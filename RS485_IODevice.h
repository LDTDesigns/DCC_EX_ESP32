#pragma once
#include "IODevice.h"
#include "IO_RS485_Node.h"
#include"IODevice.h"

class RS485_IODevice : public IODevice {
private:
    uint8_t* _states = nullptr;
 RS485_Node* _owner = nullptr;
    I2CAddress _i2c;   
    uint16_t _value [16];  
    unsigned long _lastPollMicros = 0;
const unsigned long _pollIntervalMicros = 100000;   // 100ms

public:
    RS485_IODevice(RS485_Node* owner, I2CAddress i2c, VPIN firstVpin, int nPins);
    void updateInput(VPIN vpin, int value);
  

protected:
    int _read(VPIN vpin) override;
    void _write(VPIN vpin, int value) override;
    void _display() override;
       void _loop(unsigned long now) ;

void _ProcessMask(uint16_t mask) ;
};