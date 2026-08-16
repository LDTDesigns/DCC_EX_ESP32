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
    unsigned long lastSeen = 0; // Track when the device was last seen
    IODevice::ConfigTypeEnum configType = IODevice::CONFIGURE_INPUT; // Default to input, can be set when adding the device

};

#define MAX_RS485_DEVICES 8
//#define MAX_NODES 16
#define DEVICE_TIMEOUT 5000 // 5 seconds timeout for device to be considered offline
class RS485_Node {
public:
 //static RS485_Node* registry[MAX_NODES]; 

 static RS485_Node* create(uint8_t nodeAddress,RS485BusController* bus);

    uint8_t getNodeAddress() const { return _nodeAddress; }
 uint16_t  getCurrentMask(IODevice* dev);
    // Called by proxy IODevice
  //  int  read(VPIN vpin);//not used in this implementation, reading is handled by the bus controller
    //void write(VPIN vpin, int value);//not used in this implementation, writing is handled by the bus controller

    // Called from loop()
   // void poll(unsigned long now);//not used in this implementation, polling is handled by the bus controller
void sendMask(I2CAddress i2c, uint8_t mask);
void sendAnalogueMask(I2CAddress i2c, uint8_t pin, int value, uint8_t profile=0, uint16_t duration=0, uint8_t deviceType=0);
  
    // Called by RS485BusController
 // void sendWrite(I2CAddress i2c, uint8_t pin, uint8_t value);/this is never implemented in the RS485_Node class, so we should remove it or implement it properly
  void addDevice(IODevice* dev, I2CAddress i2c, VPIN start, uint8_t pins, const char* name, IODevice::ConfigTypeEnum type = IODevice::CONFIGURE_INPUT);
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




