#include "IO_RemoteDevice.h"
#include "IO_RS485_Node.h"

RemoteDevice::RemoteDevice(VPIN firstVpin, uint8_t pinCount,
                           RS485_Node* parent, uint8_t i2cAddress)
: IODevice(firstVpin, pinCount),
  _parent(parent)
  {
   _I2CAddress = i2cAddress; 
    IODevice::addDevice(this);
    _display();
}

void RemoteDevice::_begin() {
    // Nothing needed
}

int RemoteDevice::_read(VPIN vpin) {
    return _parent->readRemote(_i2cAddress, vpin);
}

void RemoteDevice::_write(VPIN vpin, int state) {
    _parent->writeRemote(_i2cAddress, vpin, state);
}

 void RemoteDevice::_display() {
    DIAG(F("RS485 Device VPINs %u-%u configured on addr %s"),
         _firstVpin,
         _firstVpin + _nPins - 1,
         _I2CAddress.toString());
}



