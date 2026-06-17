#ifndef REMOTE_DEVICE_H
#define REMOTE_DEVICE_H

#include <stdint.h>
#include "IODevice.h"

class RS485_Node;

class RemoteDevice : public IODevice {
public:
    RemoteDevice(
        VPIN firstVpin,
        uint8_t pinCount,
        RS485_Node* parent,
        uint8_t i2cAddress
    );

    void _begin() override;
    int  _read(VPIN vpin) override;
    void _write(VPIN vpin, int state) override;
    void _display() override;

private:
    RS485_Node* _parent;
    uint8_t _i2cAddress;
};

#endif
