/**
 * @file RS485_Node.h
 * @brief Master Object-Oriented Network Matrix with Automated Write-Retry Engine
 */

#ifndef RS485_NODE_H
#define RS485_NODE_H

#include <Arduino.h>
#include "IODevice.h"
#include "VirtualRegister.h"

#define MAX_COMPONENTS_PER_NODE 4
#define MAX_WRITE_RETRIES       3  // Number of times to re-verify an unacknowledged write

enum PendingWriteType {
    WRITE_NONE,
    WRITE_BINARY,
    WRITE_ANALOGUE
};

// Structure to track unconfirmed outbox messages
struct WriteTransaction {
    PendingWriteType type;
    VPIN vpin;
    int value;
    uint8_t activity;
    uint8_t i2cAddress;
    int retryCount;
    bool awaitingConfirmation;
};

// ============================================================================
// ABSTRACT NETWORK BASE CLASS BLUEPRINT
// ============================================================================
class NetworkComponent {
protected:
    uint8_t _i2cAddress;
    VPIN _vPinStart;
    int _numPins;

public:
    NetworkComponent(uint8_t i2cAddress, VPIN vPinStart, int numPins) 
        : _i2cAddress(i2cAddress), _vPinStart(vPinStart), _numPins(numPins) {}

    virtual ~NetworkComponent() {}

    uint8_t getAddress() const { return _i2cAddress; }
    bool handlesVPin(VPIN vpin) { return (vpin >= _vPinStart && vpin < (_vPinStart + _numPins)); }

    virtual void packWrite(VPIN vpin, int state, HardwareSerial* serial, uint8_t node, uint8_t txPin, bool autoMode) = 0;
    virtual void packWriteAnalogue(VPIN vpin, int value, uint8_t activity, HardwareSerial* serial, uint8_t node, uint8_t txPin, bool autoMode) {}
    virtual void packPoll(HardwareSerial* serial, uint8_t node, uint8_t txPin, bool autoMode) = 0;
    virtual void decodeResponse(int rxByte) = 0;
    virtual bool verifyWriteSuccess(VPIN vpin, int expectedValue, uint8_t expectedActivity) { return true; }
};

// ============================================================================
// COMPONENT 1: STANDARD 8-BIT PIN EXPANDER PACKETIZER
// ============================================================================
class StandardIOExpander : public NetworkComponent {
private:
    VirtualRegister* _cache;
public:
    StandardIOExpander(uint8_t i2cAddress, VPIN vPinStart) : NetworkComponent(i2cAddress, vPinStart, 8) {
        _cache = new VirtualRegister(vPinStart, 8);
    }
    ~StandardIOExpander() { delete _cache; }

    virtual void packWrite(VPIN vpin, int state, HardwareSerial* serial, uint8_t node, uint8_t txPin, bool autoMode) override {
        _cache->_write(vpin, state);
        uint8_t bulk = 0x00;
        for (int i = 0; i < 8; i++) {
            if (_cache->_read(_vPinStart + i) == 1) bulk |= (1 << i);
        }
        if (!autoMode) { digitalWrite(txPin, HIGH); delayMicroseconds(5); }
        serial->print("<W,"); serial->print(node); serial->print(","); serial->print(_i2cAddress);
        serial->print(","); serial->print(bulk); serial->println(">");
        if (!autoMode) { serial->flush(); digitalWrite(txPin, LOW); }
    }
    
    virtual void packPoll(HardwareSerial* serial, uint8_t node, uint8_t txPin, bool autoMode) override {
        if (!autoMode) { digitalWrite(txPin, HIGH); delayMicroseconds(5); }
        serial->print("<P,"); serial->print(node); serial->print(","); serial->print(_i2cAddress); serial->println(">");
        if (!autoMode) { serial->flush(); digitalWrite(txPin, LOW); }
    }
    
    virtual void decodeResponse(int rxByte) override {
        for (int i = 0; i < 8; i++) _cache->_write(_vPinStart + i, (rxByte >> i) & 0x01);
    }

    virtual bool verifyWriteSuccess(VPIN vpin, int expectedValue, uint8_t expectedActivity) override {
        // Confirm our local cache bit state matches what we attempted to push down the wire
        return (_cache->_read(vpin) == expectedValue);
    }
};

// ============================================================================
// COMPONENT 2: GENERIC PASSTHROUGH MULTI-BYTE DEVICE (TURNTABLE / SERVO)
// ============================================================================
class RemoteI2CDevice : public NetworkComponent {
private:
    uint8_t _statusFeedbackByte;
public:
    RemoteI2CDevice(uint8_t i2cAddress, VPIN vPinStart, int numPins) 
        : NetworkComponent(i2cAddress, vPinStart, numPins), _statusFeedbackByte(0) {}

    virtual void packWrite(VPIN vpin, int state, HardwareSerial* serial, uint8_t node, uint8_t txPin, bool autoMode) override {}

    virtual void packWriteAnalogue(VPIN vpin, int value, uint8_t activity, HardwareSerial* serial, uint8_t node, uint8_t txPin, bool autoMode) override {
        uint8_t msb = value >> 8;
        uint8_t lsb = value & 0xFF;

        if (!autoMode) { digitalWrite(txPin, HIGH); delayMicroseconds(5); }
        serial->print("<T,"); serial->print(node); serial->print(","); serial->print(_i2cAddress);
        serial->print(","); serial->print(msb); serial->print(","); serial->print(lsb);
        serial->print(","); serial->print(activity); serial->println(">");
        if (!autoMode) { serial->flush(); digitalWrite(txPin, LOW); }
    }

    virtual void packPoll(HardwareSerial* serial, uint8_t node, uint8_t txPin, bool autoMode) override {
        if (!autoMode) { digitalWrite(txPin, HIGH); delayMicroseconds(5); }
        serial->print("<P,"); serial->print(node); serial->print(","); serial->print(_i2cAddress); serial->println(">");
        if (!autoMode) { serial->flush(); digitalWrite(txPin, LOW); }
    }
    
    virtual void decodeResponse(int rxByte) override {
        _statusFeedbackByte = rxByte; 
    }

    virtual bool verifyWriteSuccess(VPIN vpin, int expectedValue, uint8_t expectedActivity) override {
        // Peter Cole's code sets _stepperStatus = 1 (busy) immediately during a write.
        // If our response parser captured that the device acknowledged it's busy, the write succeeded!
        return (_statusFeedbackByte == 1);
    }
};

// ============================================================================
// THE MAIN NETWORK ROUTING MASTER ENGINE WITH RETRY LOGIC
// ============================================================================
class RS485_Node : public IODevice {
private:
    uint8_t _nodeAddress;
    HardwareSerial* _serial;
    uint8_t _txPin;
    bool _autoMode;
    NetworkComponent* _registry[MAX_COMPONENTS_PER_NODE];
    int _count;
    int _pollIndex;
    bool _waiting;
    unsigned long _lastPoll;
    char _buf[32];
    int _bufIdx;

    // Outbox Transaction Tracker
    WriteTransaction _outbox;

public:
    RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txPin = 255) : IODevice(0,0) {
        _nodeAddress = nodeAddress; _serial = &serialPort; _txPin = txPin;
        _autoMode = (txPin == 255); _count = 0; _pollIndex = 0; _waiting = false; _bufIdx = 0;
        
        _outbox.type = WRITE_NONE;
        _outbox.awaitingConfirmation = false;
        _outbox.retryCount = 0;
        
        addDevice(this);
    }

    void registerComponent(NetworkComponent* comp) {
        if (_count < MAX_COMPONENTS_PER_NODE) _registry[_count++] = comp;
    }

    // Intercept binary writes and flag verification engine
    virtual void _write(VPIN vpin, int state) override {
        for (int i = 0; i < _count; i++) {
            if (_registry[i]->handlesVPin(vpin)) {
                _registry[i]->packWrite(vpin, state, _serial, _nodeAddress, _txPin, _autoMode);
                
                // Stage into outbox tracker for loop validation
                _outbox.type = WRITE_BINARY;
                _outbox.vpin = vpin;
                _outbox.value = state;
                _outbox.i2cAddress = _registry[i]->getAddress();
                _outbox.retryCount = 0;
                _outbox.awaitingConfirmation = true;
                return;
            }
        }
    }

    // Intercept analogue writes and flag verification engine
    virtual void _writeAnalogue(VPIN vpin, int value, uint8_t activity, uint16_t duration) override {
        for (int i = 0; i < _count; i++) {
            if (_registry[i]->handlesVPin(vpin)) {
                _registry[i]->packWriteAnalogue(vpin, value, activity, _serial, _nodeAddress, _txPin, _autoMode);
                
                // Stage into outbox tracker for loop validation
                _outbox.type = WRITE_ANALOGUE;
                _outbox.vpin = vpin;
                _outbox.value = value;
                _outbox.activity = activity;
                _outbox.i2cAddress = _registry[i]->getAddress();
                _outbox.retryCount = 0;
                _outbox.awaitingConfirmation = true;
                return;
            }
        }
    }

    virtual int _read(VPIN vpin) override { return 0; }

    virtual void _loop(unsigned long currentMicros) override {
        if (_count == 0) return;

        // 1. If we aren't waiting for a polling packet response, issue the next one
        if (!_waiting) {
            _registry[_pollIndex]->packPoll(_serial, _nodeAddress, _txPin, _autoMode);
            _waiting = true; _lastPoll = currentMicros; _bufIdx = 0;
            return;
        }

        // 2. Read coming string frames across the hardware serial line
        while (_waiting && _serial->available() > 0) {
            char c = (char)_serial->read();
            if (c == '<') _bufIdx = 0;
            if (_bufIdx < (int)(sizeof(_buf) - 1)) { _buf[_bufIdx++] = c; _buf[_bufIdx] = '\0'; }
            
            if (c == '>') {
                int rNode = 0, rI2C = 0, rData = 0;
                if (sscanf(_buf, "<R,%d,%d,%d>", &rNode, &rI2C, &rData) == 3) {
                    if (rNode == _nodeAddress) {
                        for (int i = 0; i < _count; i++) {
                            if (_registry[i]->getAddress() == rI2C) { 
                                _registry[i]->decodeResponse(rData); 
                                
                                // POST-POLL CHECK: Did this component register our pending write transaction?
                                if (_outbox.awaitingConfirmation && _outbox.i2cAddress == rI2C) {
                                    if (_registry[i]->verifyWriteSuccess(_outbox.vpin, _outbox.value, _outbox.activity)) {
                                        // Write confirmed! Clear outbox pipeline safely
                                        _outbox.awaitingConfirmation = false;
                                        _outbox.type = WRITE_NONE;
                                    } else {
                                        // Validation failed. Handle Re-transmit logic
                                        if (_outbox.retryCount < MAX_WRITE_RETRIES) {
                                            _outbox.retryCount++;
                                            if (_outbox.type == WRITE_BINARY) {
                                                _registry[i]->packWrite(_outbox.vpin, _outbox.value, _serial, _nodeAddress, _txPin, _autoMode);
                                            } else if (_outbox.type == WRITE_ANALOGUE) {
                                                _registry[i]->packWriteAnalogue(_outbox.vpin, _outbox.value, _outbox.activity, _serial, _nodeAddress, _txPin, _autoMode);
                                            }
                                        } else {
                                            // Max retries exhausted, drop transaction to prevent loop blockages
                                            _outbox.awaitingConfirmation = false;
                                            _outbox.type = WRITE_NONE;
                                        }
                                    }
                                }
                                break; 
                            }
                        }
                    }
                }
                _waiting = false; _pollIndex = (_pollIndex + 1) % _count;
            }
        }

        // 3. Timeout fallback guard if a remote Nano completely fails to reply within 60ms
        if (_waiting && (currentMicros - _lastPoll > 60000UL)) { 
            _waiting = false; _pollIndex = (_pollIndex + 1) % _count; 
        }
        
        delayUntil(currentMicros + 4000UL); // 4ms background loop throttling
    }
};

#endif // RS485_NODE_H