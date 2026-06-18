#include "RS485_IODevice.h"
#include <stdlib.h>

RS485_IODevice::RS485_IODevice(RS485_Node* owner, I2CAddress i2c, VPIN firstVpin, int nPins)
: IODevice(firstVpin, nPins), _owner(owner), _i2c(i2c)
{
    _states = (uint8_t*)calloc(1, (_nPins + 7) / 8);
    addDevice(this);
}

int RS485_IODevice::RS485_IODevice::_read(VPIN vpin) {
    int pin = vpin - _firstVpin;
    if (pin < 0 || pin >= _nPins) return 0;
    uint8_t mask = 1 << (pin & 7);
    return (_states[pin >> 3] & mask) ? 1 : 0;
}

// void RS485_IODevice::_write(VPIN vpin, int value) {
//     int pin = vpin - _firstVpin;
//     if (pin < 0 || pin >= _nPins) return;
//     uint8_t mask = 1 << (pin & 7);
//     if (value)
//         _states[pin >> 3] |= mask;
//     else
//         _states[pin >> 3] &= ~mask;
// }

void RS485_IODevice::_display() {
    DIAG(F("[RS485] Virtual device on VPINs %u-%u"),
         _firstVpin, _firstVpin + _nPins - 1);
}

void RS485_IODevice::_write(VPIN vpin, int value) {
    int pin = vpin - _firstVpin;

    // Update local state (so EX‑RAIL sees it immediately)
    uint8_t mask = 1 << (pin & 7);
    if (value)
        _states[pin >> 3] |= mask;
    else
        _states[pin >> 3] &= ~mask;

    // Forward write to RS485 node
   if (_owner)
        _owner->sendWrite(_i2c, pin, value);
}

