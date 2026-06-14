#ifndef IO_RS485_NODE_H
#define IO_RS485_NODE_H

#include "IODevice.h"
#include <Arduino.h> // Needed to recognize the HardwareSerial type
/*=============================================================================
 * COMPONENT:    VirtualRegister Helper Class
 * DESCRIPTION:  Provides a lightweight, pure-RAM memory cache mapping block.
 * * WHY IT EXISTS:
 * Standard DCC-EX chip drivers (like PCF8574 or MCP23017) execute physical I2C 
 * hardware link verifications during bootup. If those chips are located across 
 * the room on a remote Nano instead of physically attached to the Master Mega's 
 * local pins, the stock drivers will log an I2C error and fail to initialize.
 * * HOW IT WORKS:
 * This helper class completely bypasses the physical hardware pins on the Mega. 
 * It allocates a raw integer array directly in the Master Mega's RAM to track 
 * pin states. 
 * * 1. When EX-Rail requests a change, it updates this local RAM table via _write().
 * 2. The companion RS485_Node wrapper intercepts that change and transmits it 
 * natively over the RS485 serial network wires.
 * 3. When background incoming responses arrive, the bits are exploded straight 
 * back into this RAM array, allowing _read() requests to be served instantly 
 * and completely non-blocking.
 *=============================================================================*/
class VirtualRegister : public IODevice {
private:
    int* _pinStates;
public:
    VirtualRegister(uint8_t numPins) : IODevice(0, numPins) {
        _pinStates = new int[numPins](); // Allocate memory array initialized to 0
    }
    virtual void _write(VPIN vpin, int value) override {
        // Just store the value directly in Master RAM
        _pinStates[vpin] = value; 
    }
    virtual int _read(VPIN vpin) override {
        // Return the value directly from Master RAM
        return _pinStates[vpin]; 
    }
};

class RS485_Node : public IODevice {
private:
    uint8_t         _nodeAddress;  
    IODevice* _subDevice;    
    VPIN            _vPinStart;    
    uint8_t         _numPins;      
    HardwareSerial* _serialPort;   // Dynamic pointer to ANY hardware serial port
    uint8_t         _txEnablePin;  // Dynamic MAX485 control pin
     bool           _isAutoMode ; // Manual mode active
    bool            _awaitingResponse;



public:
    // ========================================================================
    // UPDATED CONSTRUCTOR: Takes the serial port and DE/RE pin as parameters
    // ========================================================================
    RS485_Node(VPIN vPinStart, uint8_t numPins, IODevice* subDevice, 
               uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txEnablePin) 
        : IODevice(vPinStart, numPins) 
    {
        _vPinStart = vPinStart;
        _numPins = numPins;
        _subDevice = subDevice;
        _nodeAddress = nodeAddress;
        _serialPort = &serialPort; // Store the address of the chosen serial port
        _txEnablePin = txEnablePin;
        _awaitingResponse = false;
        _isAutoMode = false; // Manual mode active
        pinMode(_txEnablePin, OUTPUT);
        digitalWrite(_txEnablePin, LOW); 
    }

// Constructor 2: For Auto-Switching Hardware (Just pass Serial, no pin needed)
   RS485_Node(VPIN vPinStart, uint8_t numPins, IODevice* subDevice, 
               uint8_t nodeAddress, HardwareSerial& serialPort) 
        : IODevice(vPinStart, numPins) 
    {
        _vPinStart = vPinStart;
        _numPins = numPins;
        _subDevice = subDevice;
        _nodeAddress = nodeAddress;
        _serialPort = &serialPort; // Store the address of the chosen serial port  
        _awaitingResponse = false;
        _isAutoMode = true; // auto mode active
      
    }


    // Updated Factory Creator passing the port and no pin needed
    static void create(VPIN vPinStart, uint8_t numPins, IODevice* subDevice, 
                       uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txEnablePin) {
        new RS485_Node(vPinStart, numPins, subDevice, nodeAddress, serialPort, txEnablePin);
    }
  static void create(VPIN vPinStart, uint8_t numPins, IODevice* subDevice, 
                       uint8_t nodeAddress, HardwareSerial& serialPort) {
        new RS485_Node(vPinStart, numPins, subDevice, nodeAddress, serialPort);
    }



    // ========================================================================
    // OUTBOUND COMMAND PROCESSING (Uses dynamic port)
    // ========================================================================
    virtual void _write(VPIN vpin, int value) override {
        VPIN localPin = vpin - _vPinStart;
        if(!_isAutoMode){
        digitalWrite(_txEnablePin, HIGH); // Lock the physical wire
        delayMicroseconds(5); 
        }

        // Use the pointer to talk to your specific serial port smoothly
        _serialPort->print("<W ");
        _serialPort->print(_nodeAddress);
        _serialPort->print(" ");
        _serialPort->print(localPin);
        _serialPort->print(" ");
        _serialPort->print(value);
        _serialPort->println(">");
        _serialPort->flush(); 
        if(!_isAutoMode){
        digitalWrite(_txEnablePin, LOW); // Unlock the wire
        _subDevice->_write(vpin, value);    
        }       
    }

    virtual int _read(VPIN vpin) override {
        return _subDevice->_read(vpin);
    }

    // ========================================================================
    // BACKGROUND SCHEDULING (Uses dynamic port)
    // ========================================================================
    virtual void _loop(unsigned long currentMicros) override {
        
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
            
            delayUntil(currentMicros + 4000UL); // Wait 4ms for response
            return;
        }

        // Check if data is available on our specific injected serial port
        if (_awaitingResponse && _serialPort->available() > 0) {
            String response = _serialPort->readStringUntil('\n');
            
            if (response.startsWith("<R ") && response.endsWith(">")) {
                int firstSpace = response.indexOf(' ');
                int closingBrace = response.indexOf('>');
                String dataStr = response.substring(firstSpace + 1, closingBrace);
                uint8_t pinDataByte = dataStr.toInt();

                for (int i = 0; i < _numPins; i++) {
                    int bitState = (pinDataByte >> i) & 0x01;
                    _subDevice->_write(_vPinStart + i, bitState);
                }
            }
            _awaitingResponse = false; 
        }

        _awaitingResponse = false;
        delayUntil(currentMicros + 5000UL); // Padding interval
    }
};

#endif