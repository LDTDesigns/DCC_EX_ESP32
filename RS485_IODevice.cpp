#include "RS485_IODevice.h"

#ifndef RS485_DEBUG
#define RS485_DEBUG 1
#endif

RS485_IODevice::RS485_IODevice(RS485_Node*owner,I2CAddress i2c, VPIN firstVpin, int nPins)
    : IODevice(firstVpin, nPins), _owner(owner), _i2c(i2c)
{
    _states = (uint8_t *)calloc(1, (_nPins + 7) / 8);
    addDevice(this); // register in exrail
    owner->addDevice(this,i2c,firstVpin,nPins,"IOdevice");// register in node
    _display();
}
// this might be called by iodevice so may be wrong logic here possibly just pass back the local array until the poll has been completed

 void RS485_IODevice::create(RS485_Node* owner, I2CAddress i2c, VPIN firstVpin, int nPins)
{
if (checkNoOverlap(firstVpin,nPins))  new RS485_IODevice(owner,i2c,firstVpin,nPins);

}


int RS485_IODevice::_read(VPIN vpin)
{

 #if RS485_DEBUG >= 2
    DIAG(F("[RS485_IODevice::_read] vpin=%d"), vpin);
#endif

    int pin = vpin - _firstVpin;
    return _value[pin];
}

void RS485_IODevice::_display()
{
   
    DIAG(F("[RS485] Virtual device on VPINs %u-%u"),
         _firstVpin, _firstVpin + _nPins - 1);
      
}

void RS485_IODevice::_OutputToDevice(VPIN vpin, int value)
{
// this is called by iodevice so should be writing to the hardware not exrail
#if RS485_DEBUG >= 1
    DIAG(F("[RS485_IODevice::_write] vpin=%u pin=%d value=%d first=%u"), (unsigned)vpin, vpin, value, (unsigned)_firstVpin);
#endif

    if (_owner)
        _owner->sendWrite(_i2c, vpin, value);
}

void RS485_IODevice::updateInput(VPIN vpin, int value)
{

    int pin = vpin - _firstVpin;
    _value[pin] = value; // Update local array state

#if RS485_DEBUG >= 1
    DIAG(F("[RS485_IODevice::updateInput] vpin=%u value=%d"), vpin, value);

#endif

    // Notify EXRAIL / Sensors / Automation
  //  IONotifyCallback::invokeAll(vpin, value);
}

void RS485_IODevice::_loop(unsigned long now)
{

    // This function is called from the main loop to handle any pending tasks for the RS485_IODevice.
    // For example, it could check for incoming data from the RS485 bus and update the device state accordingly.
    // if the poll timer is ready

    // // Only poll when interval has elapsed
    if (now - _lastPollMicros >= _pollIntervalMicros)
    {
#if RS485_DEBUG >= 3
    DIAG(F("[RS485_IODevice::loop]"));
#endif
        _lastPollMicros = now;

        if (_owner)
        {
            uint16_t mask = _owner->getCurrentMask(this);

            if (mask != _lastProcessedMask)
            {
                _ProcessMask(mask);
                    _lastProcessedMask = mask;
            }
        }
    }
}
void RS485_IODevice::_ProcessMask(uint16_t mask)
{
mask = (~mask) & 0xFF;
 // lets bit flip as logic is inverted from pcf modules
#if RS485_DEBUG >= 1
    DIAG(F("[RS485_IODevice::_ProcessMask] flippedMask=%s"),String(mask,BIN).c_str());
#endif
    // take each bit of mask and send it to write to the corresponding VPINs of this device.  The mask is a 16-bit value where each bit represents the state of a pin (1 for HIGH, 0 for LOW). The first pin corresponds to the least significant bit (LSB) of the mask.
    for (uint8_t i = 0; i < _nPins; i++)
    {

        int newValue = (mask >> i) & 1; // extract bit i
        int oldValue = _value[i];       // cached state
#if RS485_DEBUG >= 3
        DIAG(F("  pin=%u old=%d new=%d"), i, oldValue, newValue);
#endif
        if (newValue != oldValue)
        {
#if RS485_DEBUG >= 1
            DIAG(F("  CHANGE pin=%u vpin=%u %d→%d"),
                 i, _firstVpin + i, oldValue, newValue);
#endif

            //  _value[i] = newValue;         // update cache
            updateInput(_firstVpin + i, newValue);
        }
    }
}
