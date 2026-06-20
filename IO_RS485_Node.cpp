#include "IO_RS485_Node.h"
//#include "RemoteDevice.h"
#include "RS_485_Poller.h"
#include"RS485_IODevice.h"
// ---------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------

#ifndef RS485_DEBUG
#define RS485_DEBUG 1
#endif

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
    new RS485_Poller(this, 100); // poll every 50ms //// Create proxy IODevice for this remote device based on the flag or useraddin method.
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
#if RS485_DEBUG >=1
    DIAG(F("[RS485] Node %u constructed in MANUAL mode (DE pin=%u)"),
         _nodeAddress, _txPin);
         #endif
         new RS485_Poller(this, 100); // poll every 50ms//// Create proxy IODevice for this remote device based on the flag or useraddin method.
}

// ======================================================================
// Device registration
// ======================================================================

void RS485_Node::addDevice(I2CAddress i2c, VPIN start, uint8_t pins, const char* type) {
    if (_count >= MAX_RS485_DEVICES) {
#if RS485_DEBUG >= 1
        DIAG(F("[RS485] Node %u: device registry FULL"), _nodeAddress);
#endif
        return;
    }

    _dev[_count].i2c      = i2c;
_dev[_count].start    = start;
_dev[_count].pins     = pins;
_dev[_count].type     = type;
_dev[_count].lastMask = 0xFFFF;   // correct initial value
 _dev[_count].dev = new RS485_IODevice(this, i2c, start, pins);

   // _dev[_count++] = { i2c, start, pins, type, 0xffff}; // Initialize lastMask 
   // new RS485_IODevice(this, i2c, start, pins); 
#if RS485_DEBUG >= 0
    DIAG(F("[RS485] Node %u added %s I2C %s VPIN %u-%u"),
         _nodeAddress,
         type,
         i2c.toString(),
         start,
         start + pins - 1);
         
         #endif

         _count++;

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

void RS485_Node::sendWrite(I2CAddress i2c, uint8_t pin, uint8_t value) {
  // Do not transmit if waiting for a reply
    if (_waitingForReply)
      // Queue the write
        _writePending = true;
        _wpI2C = i2c;
        _wpPin = pin;
        _wpValue = value;
        return;

    beginTx();
    _serial->print("<W ");
    _serial->print(_nodeAddress);
    _serial->print(" ");
    _serial->print(i2c.toString());
    _serial->print(" ");
    _serial->print(pin);
    _serial->print(" ");
    _serial->print(value);
    _serial->print(">");
    _serial->flush();
    endTx();
#if RS485_DEBUG >= 2
    DIAG(F("[RS485] Write → Node %u Dev %s Pin %u = %u"),
         _nodeAddress, i2c.toString(), pin, value);
         #endif
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
#if RS485_DEBUG >= 2
    DIAG(F("[RS485] Node %u read request → Dev 0x%02X pin %u"),
         _nodeAddress, d->i2c, pinIndex);
#endif

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
    sendWrite(d->i2c, idx, value);
}

// void RS485_IODevice::_write(VPIN vpin, int value) {
//     int index = vpin - _firstVpin;

//     // store the new state
//     _pinStates[index] = value;

//     // notify EXRAIL / callbacks
//     IONotifyCallback::notify(vpin, value);
// }


// ======================================================================
// Poll (incoming frames later)
// ======================================================================


void RS485_Node::poll(unsigned long now) {
 handleIncoming();

   // if (now < _nextPoll)
    //    return;

 // If waiting for reply, check timeout
    if (_waitingForReply) {
        if (millis() > _replyDeadline) {
            #if RS485_DEBUG >= 1
            DIAG(F("[RS485] Node %u: reply TIMEOUT"), _nodeAddress);
            #endif
            _waitingForReply = false;
            _inPacket = false; // reset parser state
            _rxPos = 0;
        } else {
            return; // still waiting
        }
    }

   // _nextPoll = now + POLL_INTERVAL;

    // Poll each registered remote device
 
    // Send poll for next device
   // RS485_RemoteDef& d = _dev[_pollIndex];


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

   _waitingForReply = true;
    _replyDeadline = millis() + 50; // 50ms timeout
_lastPollSent = micros();

      // Advance to next device
    _pollIndex++;
    if (_pollIndex >= _count)
        _pollIndex = 0;
            // For now: no real return path
            #if RS485_DEBUG >= 2
            DIAG(F("[RS485] Poll → Node %u Dev %s"),
                 _nodeAddress, d.i2c.toString());
                 #endif
       // }
       // handleIncoming(); move to start
        } 
}

void RS485_Node::handleIncoming() {
// Ignore echo for 300 microseconds after sending poll
if (_waitingForReply && (micros() - _lastPollSent) < 300) {
    // Don't read anything yet
    return;
}

    while (_serial->available()) {
        char c = _serial->read();

        if (c == '<') {
            _inPacket = true;
            _rxPos = 0;
            continue;
        }

        if (c == '>') {
    _rxBuf[_rxPos] = 0;
    _inPacket = false;
#if RS485_DEBUG >= 2
    DIAG(F("[RS485] RAW RX: <%s>"), _rxBuf);  
#endif
    processPacket(_rxBuf);
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

    // Ignore poll echoes (pure serial mode)
    if (p[0] == 'P') {
        // DO NOT clear _waitingForReply here
        return;
    }

    // Expect: R node i2c mask
    int node, i2c, mask;
    if (sscanf(p, "R %d %d %d", &node, &i2c, &mask) != 3){
        #if RS485_DEBUG >= 1
      DIAG(F("[RS485] UNKNOWN PACKET: <%s>"), p);
      #endif
        return;
    }
// If reply is for us
    if (node == _nodeAddress) {

        // If we were waiting, this is the reply
        if (_waitingForReply) {
            #if RS485_DEBUG >= 3
            DIAG(F("[RS485] REPLY: <%s>"), p);
            #endif
            _waitingForReply = false;
        } else {
            // Late reply — still show it
            #if RS485_DEBUG >= 1
            DIAG(F("[RS485] LATE REPLY: <%s>"), p);
            #endif
        }
    }
    if (node != _nodeAddress)
        return;

    // Now we have a REAL reply → clear waiting flag
    _waitingForReply = false;

    // Handle queued writes
    if (_writePending) {
        _writePending = false;
        sendWrite(_wpI2C, _wpPin, _wpValue);
    }

    // Find device
    RS485_RemoteDef* d = nullptr;
    for (uint8_t i = 0; i < _count; i++) {
        if (_dev[i].i2c == I2CAddress(i2c)) {
            d = &_dev[i];
            break;
        }
    }
    if (!d) return;

// Only update if mask changed
if (mask == d->lastMask) {
    return;   // no change → skip VPIN updates
}
d->lastMask = mask;


    // Apply mask → VPINs
    for (uint8_t i = 0; i < d->pins; i++) {
        int val = (mask & (1 << i)) ? 1 : 0;
      //  IODevice::write(d->start + i, val); - this might be wrong call
      d->dev->updateInput(d->start + i, val);

    }

    #if RS485_DEBUG >= 1
    DIAG(F("[RS485] Update Dev %d → mask=0x%02X"), i2c, mask);
    #endif

for (int i = 0; i < d->pins; i++) {
    VPIN v = d->start + i;
#if RS485_DEBUG >= 2
    if (!IODevice::exists(v)) {
        DIAG(F("RS485 INPUT ERROR: VPIN %u not registered!"), v);
        continue;     
    }
    #endif
#if RS485_DEBUG >= 2
    if (IODevice::exists(v)) {
        DIAG(F("RS485 INPUT : VPIN %u is registered!"), v);
        continue;     
    }
    #endif
}



}

// void RS485_Node::processPacket(const char* p) {
//    // _waitingForReply = false;

// if (_writePending) {
//     _writePending = false;
//     sendWrite(_wpI2C, _wpPin, _wpValue);
//     return;
// }

//     // Expect: R node i2c bitstring
//     if (p[0] != 'R' && p[0] != 'r')
//         return;

//     uint8_t node;
//     char i2cStr[8];
//     char bits[17];

//     int n = sscanf(p, "R %hhu %7s %16s", &node, i2cStr, bits);
//     if (n != 3)
//         return;
// if (node == _nodeAddress) {
//     _waitingForReply = false;
// }



//     if (node != _nodeAddress)
//         return;

//     // Convert i2c string to I2CAddress
//    uint8_t i2cVal = strtol(i2cStr, nullptr, 0);
// I2CAddress i2c(i2cVal);

//     // Find device
//     RS485_RemoteDef* d = nullptr;
//     for (uint8_t i = 0; i < _count; i++) {
//         if (_dev[i].i2c == i2c) {
//             d = &_dev[i];
//             break;
//         }
//     }
//     if (!d)
//         return;

//     // Update VPINs
//     for (uint8_t i = 0; i < d->pins; i++) {
//         int val = (bits[i] == '1') ? 1 : 0;
//         VPIN v = d->start + i;
//         IODevice::write(v, val);
//     }

//     DIAG(F("[RS485] Update %s → VPIN %u-%u = %s"),
//          d->i2c.toString(),
//          d->start,
//          d->start + d->pins - 1,
//          bits);
// }

