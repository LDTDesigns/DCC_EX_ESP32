#include "RS485_IODevice.h"
#include <stdlib.h>

#ifndef RS485_DEBUG
#define RS485_DEBUG 0
#endif

RS485_IODevice::RS485_IODevice(RS485_Node* owner, I2CAddress i2c, VPIN firstVpin, int nPins)
: IODevice(firstVpin, nPins), _owner(owner), _i2c(i2c)
{
    _states = (uint8_t*)calloc(1, (_nPins + 7) / 8);
   addDevice(this); //see if this prevents duplicate registration of the device in the IODevice registry
    _display();
}

int RS485_IODevice::RS485_IODevice::_read(VPIN vpin)  {
    int pin = vpin - _firstVpin;
    return _value[pin];
}


void RS485_IODevice::_display() {
    DIAG(F("[RS485] Virtual device on VPINs %u-%u"),
         _firstVpin, _firstVpin + _nPins - 1);
}

void RS485_IODevice::_write(VPIN vpin, int value) {

#if RS485_DEBUG >= 1
    DIAG(F("[RS485_IODevice::_write] vpin=%u pin=%d value=%d first=%u"), (unsigned)vpin, vpin, value, (unsigned)_firstVpin);
#endif  

    // Update local state (so EX‑RAIL sees it immediately)
  // stop here if this was a read in from device we dont want to send back
#if RS485_DEBUG >= 1
    DIAG(F("[RS485_IODevice::_write] vpin=%u pin=%d value=%d first=%u"), (unsigned)vpin, vpin, value, (unsigned)_firstVpin);
    // Forward write to RS485 node
    #endif
   if (_owner)
        _owner->sendWrite(_i2c, vpin, value);
}
void RS485_IODevice::updateInput(VPIN vpin, int value) {

    int pin = vpin - _firstVpin;
    _value[pin] = value;  // Update local state (so EX‑RAIL sees it immediately)

    #if RS485_DEBUG>=1
       DIAG(F("[RS485_IODevice::updateInput] vpin=%u value=%d"), vpin, value);
   
#endif
   // IODevice::write(vpin, value);
    // Notify EXRAIL / Sensors / Automation


  //  IONotifyCallback::invokeAll(vpin, value);
}
void RS485_IODevice::_loop(unsigned long now) {
    #if RS485_DEBUG>= 3
       DIAG(F("[RS485_IODevice::loop]"));   
       #endif
    // This function is called from the main loop to handle any pending tasks for the RS485_IODevice.
    // For example, it could check for incoming data from the RS485 bus and update the device state accordingly.
// if the poll timer is ready



    // // Only poll when interval has elapsed
     if (now - _lastPollMicros >= _pollIntervalMicros) {

        _lastPollMicros = now;

         if (_owner) {
        uint16_t mask= _owner->getCurrentMask(this);
        _ProcessMask(mask);

    //         //go and get the mask for this device from the RS485 node
    // process mask for this device
    //     
     }
  delayUntil(now + 5000);

}
}
void RS485_IODevice::_ProcessMask(uint16_t mask) {
    #if RS485_DEBUG >= 1
    DIAG(F("[RS485_IODevice::_ProcessMask] mask=0x%04X"), (unsigned int)mask);
#endif
   // take each bit of mask and send it to write to the corresponding VPINs of this device.  The mask is a 16-bit value where each bit represents the state of a pin (1 for HIGH, 0 for LOW). The first pin corresponds to the least significant bit (LSB) of the mask.
  for (uint8_t i = 0; i < _nPins; i++) {

        int newValue = (mask >> i) & 1;   // extract bit i
        int oldValue = _value[i];         // cached state
#if RS485_DEBUG >= 3
        DIAG(F("  pin=%u old=%d new=%d"), i, oldValue, newValue);
#endif
        if (newValue != oldValue) {
            #if RS485_DEBUG >= 1
            DIAG(F("  CHANGE pin=%u vpin=%u %d→%d"),
                 i, _firstVpin + i, oldValue, newValue);
#endif
            _value[i] = newValue;         // update cache
          //  updateInput(_firstVpin + i, newValue);  // notify EX‑CS  // not needed function done inline here
        }
    }
   
}
