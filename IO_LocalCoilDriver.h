#include "IO_BaseCoilDriver.h"

class LocalCoilDriver : public BaseCoilDriver {
public:
    LocalCoilDriver(I2CAddress i2c, uint8_t firstVpin, uint8_t nPins,unsigned long chargeMs,unsigned long pulse)
        : BaseCoilDriver(i2c, firstVpin, nPins,chargeMs,pulse)
    {
        DIAG(F("[LocalCoilDriver] CONSTRUCTOR CALLED"));
        addDevice(this);
    }

static void create(I2CAddress i2c,VPIN startPin,uint8_t nPins,unsigned long chargeMs,unsigned long pulse ){
    if (checkNoOverlap(startPin, nPins, i2c))  new LocalCoilDriver(i2c,startPin,nPins,chargeMs,pulse);
}

    void outputToDevice(uint8_t value) override {
      //  uint8_t mask = value ? (1 << pin) : 0x00;
        I2CManager.write(_i2c, 1, value);
    }

    void _begin() override {
       I2CManager.begin();
DIAG(F("[LocalCoilDriver::_begin] exists() returned: %S"),
     I2CManager.exists(_i2c) ? F("YES") : F("NO"));
// 
    // All coils OFF (active-low)
    I2CManager.write(_i2c,1, 0x00);

    // Mark capacitor ready at startup
    _ready = true;
    _nextReadyTime = 0;
    _display();
}

void _display() override{
        DIAG(F("Local[CoilDriver] I2C:%s VPINs %u-%u : Charge %lu ms : Pulse %lu ms %S"),
         _i2c.toString(),
         _firstVpin,
         _firstVpin + _nPins - 1,
         _chargeTime / 1000UL,
         _pulseDuration / 1000UL,
         (_deviceState == DEVSTATE_FAILED) ? F("OFFLINE") : F(""));
}

};
