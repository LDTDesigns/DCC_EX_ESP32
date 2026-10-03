#pragma once
#include"RS485_BusController.h"

#include"IO_RS485_Node.h"

#include "IODevice.h"
class RS485_Node;

class RS485_IODevice : public IODevice {
private:
    uint8_t* _states = nullptr;
 RS485_Node* _owner = nullptr;
    I2CAddress _i2c;   
    uint16_t _value [16];  
    unsigned long _lastPollMicros = 0;
const unsigned long _pollIntervalMicros = 10000;   // 10ms
uint16_t _lastProcessedMask;
void _OutputToDevice(VPIN vpin, int value);

public:
    RS485_IODevice(RS485_Node* owner, I2CAddress i2c, VPIN firstVpin, int nPins);
    void updateInput(VPIN vpin, int value);
 static void create(RS485_Node* owner, I2CAddress i2c, VPIN firstVpin, int nPins);

protected:
    int _read(VPIN vpin) override;
    void _write(VPIN vpin, int value) override;
    void _display() override;
       void _loop(unsigned long now) ;
bool _inverted = false;  // default to non-inverted logic
void _ProcessMask(uint16_t mask) ;
};