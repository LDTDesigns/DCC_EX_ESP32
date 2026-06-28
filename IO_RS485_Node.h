#ifndef RS485_NODE_H
#define RS485_NODE_H
#pragma once
#include <Arduino.h>
#include "DIAG.h"
#include "I2CManager.h"
#include"IODevice.h"
#include "RS485_BusController.h"
class RS485_IODevice;   // ⭐ forward declaration
class RS485BusController;

typedef uint16_t VPIN;

struct RS485_RemoteDef {
    I2CAddress  i2c;
    VPIN     start;
    uint8_t  pins;
    const char* type;
    uint16_t lastMask; // Store the last known state of the device  
    IODevice* dev = nullptr; 
    bool online =false;

};

#define MAX_RS485_DEVICES 8
//#define MAX_NODES 16

class RS485_Node {
public:
 //static RS485_Node* registry[MAX_NODES]; 

 static RS485_Node* create(uint8_t nodeAddress,RS485BusController* bus);

    uint8_t getNodeAddress() const { return _nodeAddress; }
 uint16_t  getCurrentMask(IODevice* dev);
    // Called by proxy IODevice
    int  read(VPIN vpin);
    void write(VPIN vpin, int value);

    // Called from loop()
    void poll(unsigned long now);

  void sendWrite(I2CAddress i2c, uint8_t pin, uint8_t value);
  void addDevice(IODevice* dev, I2CAddress i2c, VPIN start, uint8_t pins, const char* type);
  IODevice* nextDevice();

  private:
RS485_Node(uint8_t nodeAddress,RS485BusController* bus);
 friend class RS485BusController; 
uint8_t _nodeAddress;
RS485BusController* _busController;
RS485_RemoteDef _dev[MAX_RS485_DEVICES];
uint8_t         _count;
//static uint8_t nodeCount;
uint8_t _nextDeviceIndex = 0;

// Helpers
RS485_RemoteDef* find(VPIN vpin);

//void scanI2C();
    
void _begin();

};


#endif




