// //#include "RemoteDevice.h"
// #include "IO_RS485_Node.h"
// #include "DIAG.h"

// RemoteDevice::RemoteDevice(RS485_Node* parent,
//                            uint8_t i2c,
//                            VPIN start,
//                            uint8_t pins,
//                            const char* type)
// : IODevice(start, pins),
//   _parent(parent),
//   _i2c(i2c),
//   _start(start),
//   _pins(pins),
//   _type(type)
// {
//     if (_parent) {
//         _parent->registerComponent(this);

//         DIAG(F("[RS485] %s I2C 0x%02X → VPIN %u–%u on Node %u"),
//              _type,
//              _i2c,
//              _start,
//              _start + _pins - 1,
//              _parent->getNodeAddress());
//     }
// }

// int RemoteDevice::_read(VPIN vpin) {
//     return _parent ? _parent->_read(vpin) : 0;
// }

// void RemoteDevice::_write(VPIN vpin, int value) {
//     if (_parent) _parent->_write(vpin, value);
// }

// void RemoteDevice::_display() {
//     DIAG(F("        [%s] I2C 0x%02X → VPIN %u–%u"),
//          _type,
//          _i2c,
//          _start,
//          _start + _pins - 1);
// }

