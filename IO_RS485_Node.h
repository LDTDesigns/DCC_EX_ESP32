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
    VPIN getStartVpin() const { return _vPinStart; }
    int getPinCount() const { return _numPins; }

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
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "<P,%d,%d>", node, _i2cAddress);
        serial->write((uint8_t*)buf, len);
        if (!autoMode) { serial->flush(); digitalWrite(txPin, LOW); }
    }
    
    virtual void decodeResponse(int rxByte) override {
        for (int i = 0; i < 8; i++) _cache->_write(_vPinStart + i, (rxByte >> i) & 0x01);
    }

    virtual bool verifyWriteSuccess(VPIN vpin, int expectedValue, uint8_t expectedActivity) override {
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

    // Per‑node virtual VPIN map (I2C → VirtualRegister)
    VirtualRegister* _virtualMap[128];

    // Outbox Transaction Tracker
    WriteTransaction _outbox;

public:
    RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txPin = 255) : IODevice(0,0) {
        _nodeAddress = nodeAddress; _serial = &serialPort; _txPin = txPin;
        _autoMode = (txPin == 255); _count = 0; _pollIndex = 0; _waiting = false; _bufIdx = 0;

        for (int i = 0; i < 128; i++) _virtualMap[i] = nullptr;

        _outbox.type = WRITE_NONE;
        _outbox.awaitingConfirmation = false;
        _outbox.retryCount = 0;

        Serial.print("RS485_Node constructed at ");
        Serial.println((uint32_t)this, HEX);

        addDevice(this);
    }

    void registerComponent(NetworkComponent* comp) {
        if (_count < MAX_COMPONENTS_PER_NODE) {
            _registry[_count++] = comp;

            uint8_t i2c = comp->getAddress();
            if (_virtualMap[i2c] == nullptr) {
                _virtualMap[i2c] = new VirtualRegister(comp->getStartVpin(), comp->getPinCount());
            }
        }
    }

    virtual void _write(VPIN vpin, int state) override {
        for (int i = 0; i < _count; i++) {
            if (_registry[i]->handlesVPin(vpin)) {
                _registry[i]->packWrite(vpin, state, _serial, _nodeAddress, _txPin, _autoMode);

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

    virtual void _writeAnalogue(VPIN vpin, int value, uint8_t activity, uint16_t duration) override {
        for (int i = 0; i < _count; i++) {
            if (_registry[i]->handlesVPin(vpin)) {
                _registry[i]->packWriteAnalogue(vpin, value, activity, _serial, _nodeAddress, _txPin, _autoMode);

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

    virtual int _read(VPIN vpin) override {
        for (int i = 0; i < _count; i++) {
            if (_registry[i]->handlesVPin(vpin)) {
                uint8_t i2c = _registry[i]->getAddress();
                VirtualRegister* vr = _virtualMap[i2c];
                if (vr) return vr->_read(vpin);
            }
        }
        return 0;
    }

    void _loop(unsigned long currentMicros) override {

        while (_serial->available()) {
            char c = _serial->read();

            if (c == '<') {
                _bufIdx = 0;
                Serial.println("[MEGA] Start of frame '<'");
            }

            if (_bufIdx < (int)(sizeof(_buf) - 1)) {
                _buf[_bufIdx++] = c;
                _buf[_bufIdx] = '\0';
            }

            if (c == '>') {
                Serial.print("[MEGA] Frame complete: ");
                Serial.println(_buf);

                int rNode = 0, rI2C = 0, rData = 0;

                int parsed = sscanf(_buf, "<R,%d,%d,%d", &rNode, &rI2C, &rData);

                if (parsed == 3) {
                    Serial.print("[MEGA] Parsed OK → Node=");
                    Serial.print(rNode);
                    Serial.print(" I2C=");
                    Serial.print(rI2C);
                    Serial.print(" Data=");
                    Serial.println(rData);

                    if (rNode == _nodeAddress) {

                        for (int i = 0; i < _count; i++) {
                            if (_registry[i]->getAddress() == rI2C) {

                                Serial.println("[MEGA] decodeResponse() called");
                                _registry[i]->decodeResponse(rData);

                                // Update EX‑RAIL VPINs
                                VirtualRegister* vr = _virtualMap[rI2C];
                                if (vr) {
                                    uint16_t start = vr->getStartPin();
                                    uint8_t count = vr->getPinCount();
                                    for (uint8_t bit = 0; bit < count; bit++) {
                                        uint8_t value = (rData >> bit) & 0x01;
                                        vr->_write(start + bit, value);
                                    }
                                }

                                // Write confirmation
                                if (_outbox.awaitingConfirmation &&
                                    _outbox.i2cAddress == rI2C) {

                                    Serial.println("[MEGA] Write confirmation check");

                                    if (_registry[i]->verifyWriteSuccess(
                                            _outbox.vpin,
                                            _outbox.value,
                                            _outbox.activity)) {

                                        Serial.println("[MEGA] Write confirmed");
                                        _outbox.awaitingConfirmation = false;
                                        _outbox.type = WRITE_NONE;

                                    } else if (_outbox.retryCount < MAX_WRITE_RETRIES) {

                                        Serial.println("[MEGA] Write failed → retrying");
                                        _outbox.retryCount++;

                                        if (_outbox.type == WRITE_BINARY)
                                            _registry[i]->packWrite(_outbox.vpin, _outbox.value, _serial, _nodeAddress, _txPin, _autoMode);
                                        else
                                            _registry[i]->packWriteAnalogue(_outbox.vpin, _outbox.value, _outbox.activity, _serial, _nodeAddress, _txPin, _autoMode);

                                    } else {
                                        Serial.println("[MEGA] Max retries reached → dropping");
                                        _outbox.awaitingConfirmation = false;
                                        _outbox.type = WRITE_NONE;
                                    }
                                }
                                break;
                            }
                        }
                    }
                } else {
                    Serial.print("[MEGA] PARSE FAIL (");
                    Serial.print(parsed);
                    Serial.println(" fields)");
                }

                Serial.println("[MEGA] Clearing _waiting and advancing poll index");
                _waiting = false;
                _pollIndex = (_pollIndex + 1) % _count;
            }
        }

        if (_waiting && (currentMicros - _lastPoll > 60000UL)) {
            Serial.println("[MEGA] TIMEOUT → advancing poll index");
            _waiting = false;
            _pollIndex = (_pollIndex + 1) % _count;
        }

        if (!_waiting) {
            Serial.print("[MEGA] Sending poll to index ");
            Serial.println(_pollIndex);

            _registry[_pollIndex]->packPoll(_serial, _nodeAddress, _txPin, _autoMode);

            _waiting = true;
            _lastPoll = currentMicros;
            _bufIdx = 0;
        }
    }
};

#endif // RS485_NODE_H
