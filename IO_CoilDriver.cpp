#include "IO_CoilDriver.h"
#include "DIAG.h"

CoilDriver::CoilDriver(I2CAddress i2c, VPIN startVpin, uint8_t nPins,unsigned long chargeMs=5000,unsigned long pulse=50)
: IODevice( startVpin, nPins),
  _i2c(i2c),
  _nPins(nPins),
  _chargeTime(chargeMs*1000UL),
  _pulseDuration(pulse*1000UL),
  _nextReadyTime(0)
{
    memset(_lastValue, 0, sizeof(_lastValue));
    addDevice(this);
}
    
void CoilDriver::create(I2CAddress i2c,VPIN startPin,uint8_t nPins,unsigned long chargeMs,unsigned long pulse ){
    if (checkNoOverlap(startPin, nPins, i2c))  new CoilDriver(i2c,startPin,nPins,chargeMs,pulse);
}
  
void CoilDriver::_begin() {
DIAG(F("[CoilDriver::_begin] I2CAddress struct = { addr=0x%02X}"),
     _i2c);
     I2CManager.begin();
DIAG(F("[CoilDriver::_begin] exists() returned: %S"),
     I2CManager.exists(_i2c) ? F("YES") : F("NO"));

    // All coils OFF (active-low)
    I2CManager.write(_i2c,1, 0x00);

    // Mark capacitor ready at startup
    _ready = true;
    _nextReadyTime = 0;
    _display();
}

void CoilDriver::setPin(uint8_t pin, bool on) {
    // Write a single bit to the PCF-style expander
    uint8_t mask = (1 << pin);
    uint8_t val  = on ? 0 : mask;   // active-low coil driver

    I2CManager.write(_i2c, val);
}

void CoilDriver::_write(VPIN vpin, int value) {
    uint8_t pin = vpin - _firstVpin;
    if (pin >= _nPins) return;

    // Only fire on 0→1
    if (_lastValue[pin] == 1 && value == 1)
        return;

    _lastValue[pin] = value;

    if (value != 1)
        return;

    unsigned long now = micros();

    // If CDU is charging, queue the request
    if (!_ready && now < _nextReadyTime) {
        queuePush(pin);
        return;
    }

    // Fire immediately
    fireCoil(pin, now);
}


void CoilDriver::_loop(unsigned long now) {
    if (!_ready && now < _nextReadyTime)
        return;

    _ready = true;
  //I2CManager.write(_i2c,1 ,0x00);

    uint8_t pin;
    if (queuePop(pin)) {
        fireCoil(pin, now);
    }
}
void CoilDriver::fireCoil(uint8_t pin, unsigned long now) {

    // Fire the coil (active-high on board)
    uint8_t value = (1 << pin);   // only this pin HIGH
    I2CManager.write(_i2c,1, value);

    // Pulse duration
    delayMicroseconds(_pulseDuration);

    // Turn everything off (all pins LOW = safe)
    I2CManager.write(_i2c,1, 0x00);

    // Start CDU recharge
    _ready = false;
    _nextReadyTime = now + _chargeTime;
}


void CoilDriver::_display() {
    DIAG(F("[CoilDriver] I2C:%s VPINs %u-%u : Charge %lu ms : Pulse %lu ms %S"),
         _i2c.toString(),
         _firstVpin,
         _firstVpin + _nPins - 1,
         _chargeTime / 1000UL,
         _pulseDuration / 1000UL,
         (_deviceState == DEVSTATE_FAILED) ? F("OFFLINE") : F(""));
}
///////////////////////////
//Manage queue helpers
void CoilDriver::queuePush(uint8_t pin) {
    uint8_t next = (_queueTail + 1) % MAX_QUEUE;
    if (next == _queueHead) {
        // Queue full — drop oldest
        _queueHead = (_queueHead + 1) % MAX_QUEUE;
    }
    _queue[_queueTail] = pin;
    _queueTail = next;
}
bool CoilDriver::queuePop(uint8_t &pin) {
    if (_queueHead == _queueTail)
        return false; // empty

    pin = _queue[_queueHead];
    _queueHead = (_queueHead + 1) % MAX_QUEUE;
    return true;
}
