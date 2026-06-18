#ifndef RS485_NODE_H
#define RS485_NODE_H
#pragma once
#include <Arduino.h>
#include "DIAG.h"
#include "I2CManager.h"

typedef uint16_t VPIN;

struct RS485_RemoteDef {
    I2CAddress  i2c;
    VPIN     start;
    uint8_t  pins;
    const char* type;
};

#define MAX_RS485_DEVICES 8

class RS485_Node {
public:
    // AUTO mode (no DE pin)
    RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort);
  

    // MANUAL mode (DE pin controlled by MCU)
    RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txPin);

    void addDevice(I2CAddress i2c, VPIN start, uint8_t pins, const char* type);

    uint8_t getNodeAddress() const { return _nodeAddress; }

    // Called by proxy IODevice
    int  read(VPIN vpin);
    void write(VPIN vpin, int value);

    // Called from loop()
    void poll(unsigned long now);

  void sendWrite(I2CAddress i2c, uint8_t pin, uint8_t value);

private:
    uint8_t         _nodeAddress;
    HardwareSerial* _serial;
    uint8_t         _txPin;
    bool            _autoMode;

    RS485_RemoteDef _dev[MAX_RS485_DEVICES];
    uint8_t         _count;

    // Helpers
    void beginTx();
    void endTx();
    RS485_RemoteDef* find(VPIN vpin);
unsigned long _lastPollSent = 0;

    
bool _writePending = false;
I2CAddress _wpI2C;
uint8_t _wpPin;
uint8_t _wpValue;

    
    int  requestRead(RS485_RemoteDef* d, uint8_t pinIndex);
// Incoming packet parser state
bool        _inPacket = false;
char        _rxBuf[64];
uint8_t     _rxPos = 0;

// Internal helpers
void handleIncoming();
void processPacket(const char* p);

// Poll/Reply state tracking
bool _waitingForReply = false;        // true after sending <P ...>
unsigned long _replyDeadline = 0;     // millis() when reply must arrive
uint8_t _pollIndex = 0;               // which device to poll next


   
};


#endif




