#include "IO_RS485_Node.h"
#include "IO_RemoteDevice.h"

// Auto-direction constructor
RS485_Node::RS485_Node(uint8_t nodeAddress, HardwareSerial* port)
: _nodeAddress(nodeAddress),
  _port(port),
  _deRePin(-1),
  _autoDirection(true)
{
    _display();
}

// Manual-direction constructor
RS485_Node::RS485_Node(uint8_t nodeAddress, HardwareSerial* port, int deRePin)
: _nodeAddress(nodeAddress),
  _port(port),
  _deRePin(deRePin),
  _autoDirection(false)
{
    pinMode(_deRePin, OUTPUT);
    digitalWrite(_deRePin, LOW);   // RX mode
    _display();
}

void RS485_Node::_setTxMode(bool enable) {
    if (_autoDirection) return;
    digitalWrite(_deRePin, enable ? HIGH : LOW);
}

void RS485_Node::addDevice(uint8_t i2cAddress, VPIN firstVpin, uint8_t pinCount) {
    DeviceInfo& info = _devices[_deviceCount++];

    info.i2cAddress = i2cAddress;
    info.firstVpin  = firstVpin;
    info.pinCount   = pinCount;

    info.dev = new RemoteDevice(
        firstVpin,
        pinCount,
        this,
        i2cAddress
    );
   
}

int RS485_Node::readRemote(uint8_t i2cAddress, VPIN vpin) {
    _setTxMode(true);
    // TODO: send RS485 read frame
    _setTxMode(false);

    // TODO: wait for reply
    return 0;
}

void RS485_Node::writeRemote(uint8_t i2cAddress, VPIN vpin, int state) {
    _setTxMode(true);
    // TODO: send RS485 write frame
    _setTxMode(false);
}


void RS485_Node::_display() {
    const char* portName =
        (_port == &Serial1) ? "Serial1" :
        (_port == &Serial2) ? "Serial2" :
        (_port== &Serial3) ? "Serial3" : "Serial";

    DIAG(F("RS485 Node %u on %s mode:%s Devices:%u"),
         _nodeAddress,
         portName,
         _autoDirection ? "auto" : "manual",
         _deviceCount);
}

