#pragma once
#include "IODevice.h"
#include "IO_RS485_Node.h"
#include"IODevice.h"

class RS485_IODevice : public IODevice {
private:
    uint8_t* _states = nullptr;
 RS485_Node* _owner = nullptr;
    I2CAddress _i2c;     
public:
    RS485_IODevice(RS485_Node* owner, I2CAddress i2c, VPIN firstVpin, int nPins);
    void updateInput(VPIN vpin, int value);

protected:
    int _read(VPIN vpin) override;
    void _write(VPIN vpin, int value) override;
    void _display() override;
};
