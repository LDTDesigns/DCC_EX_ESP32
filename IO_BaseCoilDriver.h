#pragma once
#include "IODevice.h"
#include "I2CManager.h"

class BaseCoilDriver : public IODevice {
public:
BaseCoilDriver(I2CAddress i2c, VPIN startVpin, uint8_t nPins,unsigned long  chargeMs, unsigned long pulse);


// dont need to create a base class??
//static void create(I2CAddress i2c,VPIN startVpin,uint8_t nPins,unsigned long chargeMs, unsigned long pulse);


    void _write(VPIN vpin, int value) override;
    int  _read(VPIN vpin) override { return 0; }   // coils are write-only
    void _loop(unsigned long now) override;

protected:
 I2CAddress _i2c;
     uint8_t    _nPins;
     const unsigned long _chargeTime;
const unsigned long _pulseDuration;
bool _ready;
unsigned long _nextReadyTime;
private:
   


uint8_t _lastValue[16];   // safe upper bound



//FIFO Queueing system to overcome charge times
static const uint8_t MAX_QUEUE = 16;

uint8_t _queue[MAX_QUEUE];
uint8_t _queueHead = 0;
uint8_t _queueTail = 0;

////////////////////////////////////////////////////////////////
    void setPin(uint8_t pin, bool on);
    virtual void _display() override;
virtual void _begin() override;
virtual void outputToDevice(uint8_t value);
    //fifo helpers
   void fireCoil(uint8_t pin, unsigned long now);
  void queuePush(uint8_t pin);
  bool queuePop(uint8_t &pin);
};
