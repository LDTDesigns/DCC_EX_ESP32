#ifndef RS485_NODE_H
#define RS485_NODE_H

#include <stdint.h>
#include <Arduino.h>
#include "IODevice.h"

class RemoteDevice;

class RS485_Node {
public:
    // Auto-direction RS485 interface (no DE/RE pin)
    RS485_Node(uint8_t nodeAddress, HardwareSerial* port);

    // Manual-direction RS485 interface (DE/RE pin)
    RS485_Node(uint8_t nodeAddress, HardwareSerial* port, int deRePin);

    void addDevice(uint8_t i2cAddress, VPIN firstVpin, uint8_t pinCount);

    int  readRemote(uint8_t i2cAddress, VPIN vpin);
    void writeRemote(uint8_t i2cAddress, VPIN vpin, int state);
    void _display();

private:
    uint8_t _nodeAddress;
    HardwareSerial* _port;

    int _deRePin;          // -1 = auto-direction
    bool _autoDirection;   // true = no DE/RE control required

    struct DeviceInfo {
        uint8_t i2cAddress;
        VPIN firstVpin;
        uint8_t pinCount;
        RemoteDevice* dev;
    };

    DeviceInfo _devices[16];
    uint8_t _deviceCount = 0;

    void _setTxMode(bool enable);
};

#endif



