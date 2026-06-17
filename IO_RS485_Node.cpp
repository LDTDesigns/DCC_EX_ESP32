#include "IO_RS485_Node.h"
//#include "RemoteDevice.h"
#include "RS_485_Poller.h"
#include"RS485_IODevice.h"
// ---------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------



unsigned long _nextPoll = 0;
static constexpr unsigned long POLL_INTERVAL = 50 * 1000; // 50ms

// ======================================================================
// Constructors
// ======================================================================

// AUTO mode
RS485_Node::RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort)
: _nodeAddress(nodeAddress),
  _serial(&serialPort),
  _txPin(255),
  _autoMode(true),
  _count(0)
{
    DIAG(F("[RS485] Node %u constructed in AUTO mode"), _nodeAddress);
    new RS485_Poller(this, 50); // poll every 50ms
}

// MANUAL mode
RS485_Node::RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txPin)
: _nodeAddress(nodeAddress),
  _serial(&serialPort),
  _txPin(txPin),
  _autoMode(false),
  _count(0)
{
    pinMode(_txPin, OUTPUT);
    digitalWrite(_txPin, LOW); // RX mode

    DIAG(F("[RS485] Node %u constructed in MANUAL mode (DE pin=%u)"),
         _nodeAddress, _txPin);
         new RS485_Poller(this, 50); // poll every 50ms
}

// ======================================================================
// Device registration
// ======================================================================

void RS485_Node::addDevice(I2CAddress i2c, VPIN start, uint8_t pins, const char* type) {
    if (_count >= MAX_RS485_DEVICES) {
        DIAG(F("[RS485] Node %u: device registry FULL"), _nodeAddress);
        return;
    }

    _dev[_count++] = { i2c, start, pins, type };
    new RS485_IODevice(start, pins);

    DIAG(F("[RS485] Node %u added %s I2C %s VPIN %u-%u"),
         _nodeAddress,
         type,
         i2c.toString(),
         start,
         start + pins - 1);
}

RS485_RemoteDef* RS485_Node::find(VPIN vpin) {
    for (uint8_t i = 0; i < _count; i++) {
        VPIN s = _dev[i].start;
        VPIN e = s + _dev[i].pins - 1;
        if (vpin >= s && vpin <= e)
            return &_dev[i];
    }
    return nullptr;
}

// ======================================================================
// TX direction control
// ======================================================================

void RS485_Node::beginTx() {
    if (!_autoMode && _txPin != 255)
        digitalWrite(_txPin, HIGH); // TX
}

void RS485_Node::endTx() {
    if (!_autoMode && _txPin != 255)
        digitalWrite(_txPin, LOW); // RX
}

// ======================================================================
// RS485 protocol (simple placeholder)
// ======================================================================

void RS485_Node::sendWrite(RS485_RemoteDef* d, uint8_t pinIndex, int value) {
    beginTx();
    _serial->write(0xAA);              // start
    _serial->write(_nodeAddress);
    _serial->write(d->i2c);
    _serial->write(pinIndex);
    _serial->write(value ? 1 : 0);
    _serial->write(0x55);              // end
    _serial->flush();
    endTx();

    DIAG(F("[RS485] Node %u → Dev 0x%02X pin %u = %d"),
         _nodeAddress, d->i2c, pinIndex, value);
}

int RS485_Node::requestRead(RS485_RemoteDef* d, uint8_t pinIndex) {
    beginTx();
    _serial->write(0xAB);              // read request
    _serial->write(_nodeAddress);
    _serial->write(d->i2c);
    _serial->write(pinIndex);
    _serial->write(0x56);
    _serial->flush();
    endTx();

    // For now: no real return path
    DIAG(F("[RS485] Node %u read request → Dev 0x%02X pin %u"),
         _nodeAddress, d->i2c, pinIndex);

    return 0; // placeholder
}

// ======================================================================
// Public API used by proxy IODevice
// ======================================================================

int RS485_Node::read(VPIN vpin) {
    RS485_RemoteDef* d = find(vpin);
    if (!d) return 0;
    uint8_t idx = vpin - d->start;
    return requestRead(d, idx);
}

void RS485_Node::write(VPIN vpin, int value) {
    RS485_RemoteDef* d = find(vpin);
    if (!d) return;
    uint8_t idx = vpin - d->start;
    sendWrite(d, idx, value);
}

// ======================================================================
// Poll (incoming frames later)
// ======================================================================


void RS485_Node::poll(unsigned long now) {
    if (now < _nextPoll)
        return;

    _nextPoll = now + POLL_INTERVAL;

    // Poll each registered remote device
    for (uint8_t i = 0; i < _count; i++) {
        RS485_RemoteDef& d = _dev[i];

        for (uint8_t i = 0; i< _count; i++) {
        RS485_RemoteDef& d = _dev[i];
              beginTx();
        _serial->print("<P ");
        _serial->print(_nodeAddress);
        _serial->print(" ");
        _serial->print(d.i2c.toString());
        _serial->print(">");
        _serial->flush();
        endTx();

            // For now: no real return path
            DIAG(F("[RS485] Poll → Node %u Dev %s"),
                 _nodeAddress, d.i2c.toString());
        }
        handleIncoming();
       
}
}
void RS485_Node::handleIncoming() {

    while (_serial->available()) {
        char c = _serial->read();

        if (c == '<') {
            _inPacket = true;
            _rxPos = 0;
            continue;
        }

        if (!_inPacket)
            continue;

        if (c == '>') {
            _rxBuf[_rxPos] = 0;
            _inPacket = false;
            processPacket(_rxBuf);
            continue;
        }

        if (_rxPos < sizeof(_rxBuf) - 1)
            _rxBuf[_rxPos++] = c;
    }
}

void RS485_Node::processPacket(const char* p) {
    // Expect: R node i2c bitstring
    if (p[0] != 'R' && p[0] != 'r')
        return;

    uint8_t node;
    char i2cStr[8];
    char bits[17];

    int n = sscanf(p, "R %hhu %7s %16s", &node, i2cStr, bits);
    if (n != 3)
        return;

    if (node != _nodeAddress)
        return;

    // Convert i2c string to I2CAddress
   uint8_t i2cVal = strtol(i2cStr, nullptr, 0);
I2CAddress i2c(i2cVal);

    // Find device
    RS485_RemoteDef* d = nullptr;
    for (uint8_t i = 0; i < _count; i++) {
        if (_dev[i].i2c == i2c) {
            d = &_dev[i];
            break;
        }
    }
    if (!d)
        return;

    // Update VPINs
    for (uint8_t i = 0; i < d->pins; i++) {
        int val = (bits[i] == '1') ? 1 : 0;
        VPIN v = d->start + i;
        IODevice::write(v, val);
    }

    DIAG(F("[RS485] Update %s → VPIN %u-%u = %s"),
         d->i2c.toString(),
         d->start,
         d->start + d->pins - 1,
         bits);
}
