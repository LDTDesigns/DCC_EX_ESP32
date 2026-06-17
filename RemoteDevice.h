#pragma once

class RS485_Node;
#include "IODevice.h"

class RemoteDevice : public IODevice {
public:
    RemoteDevice(RS485_Node* parent,
                 uint8_t i2c,
                 VPIN start,
                 uint8_t pins,
                 const char* type = "RemoteDevice");

    int  _read(VPIN vpin) override;
    void _write(VPIN vpin, int value) override;
    void _display() override;

    uint8_t getAddress()   const { return _i2c; }
    VPIN    getStartVpin() const { return _start; }
    uint8_t getPinCount()  const { return _pins; }
    const char* getType()  const { return _type; }

private:
    RS485_Node* _parent;
    uint8_t     _i2c;
    VPIN        _start;
    uint8_t     _pins;
    const char* _type;
};
