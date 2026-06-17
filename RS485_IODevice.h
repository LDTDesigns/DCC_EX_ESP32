#pragma once
#include "IODevice.h"

class RS485_IODevice : public IODevice {
private:
    uint8_t* _states = nullptr;

public:
    RS485_IODevice(VPIN firstVpin, int nPins);

protected:
    int _read(VPIN vpin) override;
    void _write(VPIN vpin, int value) override;
    void _display() override;
};
