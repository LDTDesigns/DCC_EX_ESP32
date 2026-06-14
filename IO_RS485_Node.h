#ifndef IO_RS485_NODE_H
#define IO_RS485_NODE_H

#include "IODevice.h"
#include <Arduino.h> 

/*=============================================================================
 * COMPONENT:    VirtualRegister Helper Class
 *=============================================================================*/
class VirtualRegister : public IODevice {
private:
    int* _pinStates;
public:
    VirtualRegister(VPIN vPinStart, uint8_t numPins) : IODevice(vPinStart, numPins) {
        _pinStates = new int[numPins](); 
    }

    // Public accessors allowing our wrapper Node to copy our configuration maps safely
    VPIN getStartPin() { return _firstVpin; }
    uint8_t getPinCount() { return _nPins; }

    virtual void _write(VPIN vpin, int value) override {
        // Safe access: Offset global VPIN down to a local 0-(numPins-1) array index
        _pinStates[vpin - _firstVpin] = value; 
    }

    virtual int _read(VPIN vpin) override {
        // Safe access: Offset global VPIN down to a local 0-(numPins-1) array index
        return _pinStates[vpin - _firstVpin]; 
    }
};

/*=============================================================================
 * COMPONENT:    RS485_Node Driver Class
 *=============================================================================*/
class RS485_Node : public IODevice {
private:
    uint8_t           _nodeAddress;  
    VirtualRegister* _subDevice;    
    VPIN              _vPinStart;    
    uint8_t           _numPins;      
    HardwareSerial* _serialPort;   
    uint8_t           _txEnablePin;  
    bool              _isAutoMode; 
    bool              _awaitingResponse;
    unsigned long     _lastPollTime; 

public:
    // Constructor 1: Manual DE/RE Control
    RS485_Node(VirtualRegister* subDevice, uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txEnablePin) 
        : IODevice(subDevice->getStartPin(), subDevice->getPinCount()) 
    {
        _vPinStart = subDevice->getStartPin();
        _numPins = subDevice->getPinCount();
        _subDevice = subDevice;
        _nodeAddress = nodeAddress;
        _serialPort = &serialPort; 
        _txEnablePin = txEnablePin;
        _awaitingResponse = false;
        _isAutoMode = false; 
        _lastPollTime = 0;
        
        pinMode(_txEnablePin, OUTPUT);
        digitalWrite(_txEnablePin, LOW); 
    }

    // Constructor 2: Auto-Switching Hardware
    RS485_Node(VirtualRegister* subDevice, uint8_t nodeAddress, HardwareSerial& serialPort) 
        : IODevice(subDevice->getStartPin(), subDevice->getPinCount()) 
    {
        _vPinStart = subDevice->getStartPin();
        _numPins = subDevice->getPinCount();
        _subDevice = subDevice;   
        _nodeAddress = nodeAddress;
        _serialPort = &serialPort;   
        _awaitingResponse = false;
        _isAutoMode = true; 
        _lastPollTime = 0;
    }

    // ========================================================================
    // COMPILER & STRUCTURAL FIX: FOOLPROOF FACTORY CREATORS
    // ========================================================================
    // Because factory methods run inside the context of an inherited class,
    // they have authorized access to execute the protected static "addDevice()"!
    
    // Creator 1: Manual DE/RE Pin Switching
    static void create(VirtualRegister* subDevice, uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txEnablePin) {
        // 1. Fully link the RAM cache footprint into the master DCC-EX device chain
        addDevice(subDevice);

        // 2. Spawn the physical wire node wrapper monitoring that RAM cache
        RS485_Node* newNode = new RS485_Node(subDevice, nodeAddress, serialPort, txEnablePin);

        // 3. Link the wire node into the master chain so its background _loop runs
        addDevice(newNode);
    }
    
    // Creator 2: Auto-Switching Hardware
    static void create(VirtualRegister* subDevice, uint8_t nodeAddress, HardwareSerial& serialPort) {
        // 1. Fully link the RAM cache footprint into the master DCC-EX device chain
        addDevice(subDevice);

        // 2. Spawn the physical wire node wrapper monitoring that RAM cache
        RS485_Node* newNode = new RS485_Node(subDevice, nodeAddress, serialPort);

        // 3. Link the wire node into the master chain so its background _loop runs
        addDevice(newNode);
    }

    // ========================================================================
    // OUTBOUND COMMAND PROCESSING
    // ========================================================================
    virtual void _write(VPIN vpin, int value) override {
        VPIN localPin = vpin - _vPinStart;
        if(!_isAutoMode){
            digitalWrite(_txEnablePin, HIGH); 
            delayMicroseconds(5); 
        }

        _serialPort->print("<W ");
        _serialPort->print(_nodeAddress);
        _serialPort->print(" ");
        _serialPort->print(localPin);
        _serialPort->print(" ");
        _serialPort->print(value);
        _serialPort->println(">");
        _serialPort->flush(); 
        
        if(!_isAutoMode){
            digitalWrite(_txEnablePin, LOW); 
        }       
        // Update the local RAM scratchpad instantly so read loops match
        _subDevice->_write(vpin, value);    
    }

    virtual int _read(VPIN vpin) override {
        return _subDevice->_read(vpin);
    }

 
    // ========================================================================
    // BACKGROUND SCHEDULING (The Network Polling Core)
    // ========================================================================
  virtual void _loop(unsigned long currentMicros) override {
        
        // 1. TRANSMITTER TIMING BLOCK
        if (!_awaitingResponse) { 
            if(!_isAutoMode){
                digitalWrite(_txEnablePin, HIGH); 
                delayMicroseconds(5);
            }

            _serialPort->print("<P ");
            _serialPort->print(_nodeAddress);
            _serialPort->println(">");
            
            if(!_isAutoMode){
                _serialPort->flush();
                digitalWrite(_txEnablePin, LOW); 
            }
            _awaitingResponse = true;
            _lastPollTime = currentMicros; 
            
            delayUntil(currentMicros + 4000UL); 
            return;
        }

        // 2. STATE-MACHINE ACCUMULATOR BUFFER (Uncrashable)
        static char staticBuffer[40];
        static int bufferIndex = 0;

        while (_serialPort->available() > 0) {
            char inChar = (char)_serialPort->read();

            // Frame Boundary Reset: If we see an opening brace, drop stale junk
            if (inChar == '<') {
                bufferIndex = 0;
            }

            // Store character if we have safety room
            if (bufferIndex < (int)(sizeof(staticBuffer) - 1)) {
                staticBuffer[bufferIndex++] = inChar;
                staticBuffer[bufferIndex] = '\0'; // Always maintain string termination
            }

            // Execution Gate: We hit the end of a valid DCC-EX envelope packet
            if (inChar == '>') {
                int incomingVPin = 0;
                int pinState = 0;

                // Safely decode directly out of the static memory stack
                if (sscanf(staticBuffer, "<R %d %d>", &incomingVPin, &pinState) == 2) {
                    if (incomingVPin >= _vPinStart && incomingVPin < (_vPinStart + _numPins)) {
                        _subDevice->_write(incomingVPin, pinState);
                    }
                }
                
                bufferIndex = 0; // Clear string immediately for next packet line
                _awaitingResponse = false; // Valid packet processed successfully!
            }
        } 
        
        // Timeout Protection Layer: Clear locks if connection drops out entirely
        if (_awaitingResponse && (currentMicros - _lastPollTime > 60000UL)) {
            _awaitingResponse = false; 
            bufferIndex = 0; // Dump residual buffer framing debris
        }

        delayUntil(currentMicros + 4000UL); 
    }
};

#endif // IO_RS485_NODE_H