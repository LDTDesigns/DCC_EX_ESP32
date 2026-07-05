#include "IO_BaseCoilDriver.h"
#include "DIAG.h"

BaseCoilDriver::BaseCoilDriver(I2CAddress i2c, VPIN startVpin, uint8_t nPins,unsigned long chargeMs=5000,unsigned long pulse=50)
: IODevice( startVpin, nPins),
  _i2c(i2c),
  _nPins(nPins),
  _chargeTime(chargeMs*1000UL),
  _pulseDuration(pulse*1000UL),
  _nextReadyTime(0)
{
    memset(_lastValue, 0, sizeof(_lastValue));
    //addDevice(this);
}
    
//  never create from base class?

//void BaseCoilDriver::create(I2CAddress i2c,VPIN startPin,uint8_t nPins,unsigned long chargeMs,unsigned long pulse ){
 //   if (checkNoOverlap(startPin, nPins, i2c))  new BaseCoilDriver(i2c,startPin,nPins,chargeMs,pulse);
//}
  
void BaseCoilDriver::_begin(){
  DIAG(F("[BaseCoilDriver] begin not implemented"));

    
}

void BaseCoilDriver::setPin(uint8_t pin, bool on) {
    // Write a single bit to the PCF-style expander
    uint8_t mask = (1 << pin);
    uint8_t val  = on ? 0 : mask;   // active-low coil driver

    I2CManager.write(_i2c, val);
}

void BaseCoilDriver::_write(VPIN vpin, int value) {
    DIAG(F("[BaseCoilDriver] _write vpin=%u value=%d"), (unsigned)vpin, value);
    uint8_t pin = vpin - _firstVpin;
    if (pin >= _nPins) return;

    // Only fire on 0→1
    if (_lastValue[pin] == 1 && value == 1)
        return;

    _lastValue[pin] = value;

    // this signal is falling edge so we dont need to act on it
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


void BaseCoilDriver::_loop(unsigned long now) {
if (_coilActive && now >= _coilOffTime) {
        // Turn everything off (all pins LOW = safe)
        outputToDevice(0x00);
        _coilActive = false;
    }

    if (!_ready && now < _nextReadyTime)
        return;

    _ready = true;
 
    uint8_t pin;
    if (queuePop(pin)) {
        fireCoil(pin, now);
    }
}
void BaseCoilDriver::fireCoil(uint8_t pin, unsigned long now) {
// this could typically be a write method
    // Fire the coil (active-high on board)
   // this writes 
    uint8_t value = (1 << pin);   // only this pin HIGH
//call class write method
outputToDevice(value);
  //  I2CManager.write(_i2c,1, value);

     _coilActive = true;
    _coilOffTime = now + _pulseDuration;   // pulseDuration in m
    // Pulse duration
   // delayMicroseconds(_pulseDuration);

    // Turn everything off (all pins LOW = safe)
   // call to class method
  // value=0x00;
  // outputToDevice(value);
  //  I2CManager.write(_i2c,1, 0x00);



    // Start CDU recharge
    _ready = false;
    _nextReadyTime = now + _chargeTime;
}
 void BaseCoilDriver::outputToDevice(uint8_t value) {
    DIAG(F("[BaseCoilDriver] outputToDevice() not implemented value=%u"), value);
    // make this only pass the value as 8 bit mask
}


void BaseCoilDriver::_display() {
 DIAG(F("[BaseCoilDriver] display() not implemented"));
}
///////////////////////////
//Manage queue helpers
void BaseCoilDriver::queuePush(uint8_t pin) {
    uint8_t next = (_queueTail + 1) % MAX_QUEUE;
    if (next == _queueHead) {
        // Queue full — drop oldest
        _queueHead = (_queueHead + 1) % MAX_QUEUE;
    }
    _queue[_queueTail] = pin;
    _queueTail = next;
}
bool BaseCoilDriver::queuePop(uint8_t &pin) {
    if (_queueHead == _queueTail)
        return false; // empty

    pin = _queue[_queueHead];
    _queueHead = (_queueHead + 1) % MAX_QUEUE;
    return true;
}
