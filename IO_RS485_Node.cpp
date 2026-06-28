#include "IO_RS485_Node.h"
//#include "RemoteDevice.h"
//#include "RS_485_Poller.h"
///#include"RS485_IODevice.h"
//---------------------------------------------------------------------

#ifndef RS485_DEBUG
#define RS485_DEBUG 1
#endif

//unsigned long _nextPoll = 0;
//static constexpr unsigned long POLL_INTERVAL = 50 * 1000; // 50ms
//RS485_Node* RS485_Node::registry[MAX_NODES] = { nullptr };
//uint8_t RS485_Node::nodeCount = 0;

// ======================================================================
// Constructors
// ======================================================================


RS485_Node::RS485_Node(uint8_t nodeAddress,RS485BusController* bus)
:_nodeAddress(nodeAddress),_busController(bus),_count(0)
{
  //  DIAG(F("[RS485] Node %u constructed on bus %p"), _nodeAddress,_busController);
   // dont create plloer here use the iodevice to poll in loop
  //new RS485_Poller(this, 100); // poll every 50ms //// Create proxy IODevice for this remote device based on the flag or useraddin method.
_begin();
}


 RS485_Node* RS485_Node::create(uint8_t nodeAddress,RS485BusController* bus)
                        
    {
        RS485_Node* node = new RS485_Node(nodeAddress,bus);
   bus->addNode(node);
        return node;
    }



// ======================================================================
// Device registration
// ======================================================================

void RS485_Node::addDevice(IODevice* dev,I2CAddress i2c, VPIN start, uint8_t pins, const char* type) {
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
 _dev[_count].dev= dev;

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
void RS485_Node::_begin() {
   // scanI2C();

    //  new RS485_Poller(this, 50); // poll every 50ms //// Create proxy IODevice for this remote device based on the flag or useraddin method.
   // discoverDevices();      // remote scan
   // for each remote device:
       // addDevice();         // creates RS485_IODevice
   // startPoller();          // safe now
}


IODevice* RS485_Node::nextDevice()
{
    if (_count == 0)
        return nullptr;

    // Loop until we find an online device or exhaust all
    for (uint8_t i = 0; i < _count; i++)
    {
        RS485_RemoteDef& d = _dev[_nextDeviceIndex];
        // advance index for next call
        _nextDeviceIndex = (_nextDeviceIndex + 1) % _count;

        if (d.online && d.dev != nullptr)
            return d.dev;
    }

    return nullptr;  // no online devices
}



// ======================================================================
// TX direction control
// ======================================================================

// void RS485_Node::beginTx() {
//     if (!_autoMode && _txPin != 255)
//         digitalWrite(_txPin, HIGH); // TX
// }

// void RS485_Node::endTx() {
//     if (!_autoMode && _txPin != 255)
//         digitalWrite(_txPin, LOW); // RX
// }

// ======================================================================
// RS485 protocol (simple placeholder)
// ======================================================================

// void RS485_Node::sendWrite(I2CAddress i2c, uint8_t pin, uint8_t value) {
//   // Do not transmit if waiting for a reply
//     if (_waitingForReply)
//       // Queue the write
//         _writePending = true;
//         _wpI2C = i2c;
//         _wpPin = pin;
//         _wpValue = value;
//         return;

//     beginTx();
//     _serial->print("<W ");
//     _serial->print(_nodeAddress);
//     _serial->print(" ");
//     _serial->print(i2c.toString());
//     _serial->print(" ");
//     _serial->print(pin);
//     _serial->print(" ");
//     _serial->print(value);
//     _serial->print(">");
//     _serial->flush();
//     endTx();
// #if RS485_DEBUG >= 1
//     DIAG(F("[RS485] Write → Node %u Dev %s Pin %u = %u"),
//          _nodeAddress, i2c.toString(), pin, value);
//          #endif
// }

// int RS485_Node::requestRead(RS485_RemoteDef* d, uint8_t pinIndex) {
//     beginTx();
//     _serial->write(0xAB);              // read request
//     _serial->write(_nodeAddress);
//     _serial->write(d->i2c);
//     _serial->write(pinIndex);
//     _serial->write(0x56);
//     _serial->flush();
//     endTx();

//     // For now: no real return path
// #if RS485_DEBUG >= 2
//     DIAG(F("[RS485] Node %u read request → Dev 0x%02X pin %u"),
//          _nodeAddress, d->i2c, pinIndex);
// #endif

//     return 0; // placeholder
// }

// ======================================================================
// Public API used by proxy IODevice
// ======================================================================

// int RS485_Node::read(VPIN vpin) {
//     RS485_RemoteDef* d = find(vpin);
//     if (!d) return 0;
//     uint8_t idx = vpin - d->start;
//     return requestRead(d, idx);
// }

// void RS485_Node::write(VPIN vpin, int value) {
//     RS485_RemoteDef* d = find(vpin);
//     if (!d) return;
//     uint8_t idx = vpin - d->start;
//     sendWrite(d->i2c, idx, value);
// }

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


// void RS485_Node::poll(unsigned long now) {
//  handleIncoming();

//    // if (now < _nextPoll)
//     //    return;

//  // If waiting for reply, check timeout
//     if (_waitingForReply) {
//         if (millis() > _replyDeadline) {
//             #if RS485_DEBUG >= 1
//             DIAG(F("[RS485] Node %u: reply TIMEOUT"), _nodeAddress);
//             #endif
//             _waitingForReply = false;
//             _inPacket = false; // reset parser state
//             _rxPos = 0;
//         } else {
//             return; // still waiting
//         }
//     }

//    // _nextPoll = now + POLL_INTERVAL;

//     // Poll each registered remote device
 
//     // Send poll for next device
//    // RS485_RemoteDef& d = _dev[_pollIndex];


//         for (uint8_t i = 0; i< _count; i++) {
//        RS485_RemoteDef& d = _dev[i];

//         if (!d.online) {
//         // Skip devices not found during scan
//         continue;
//     }
//               beginTx();
//         _serial->print("<P ");
//         _serial->print(_nodeAddress);
//         _serial->print(" ");
//         _serial->print(d.i2c.toString());
//         _serial->print(">");
//         _serial->flush();
//         endTx();

//    _waitingForReply = true;
//     _replyDeadline = millis() + 50; // 50ms timeout
// _lastPollSent = micros();

//       // Advance to next device
//     _pollIndex++;
//     if (_pollIndex >= _count)
//         _pollIndex = 0;
//             // For now: no real return path
//             #if RS485_DEBUG >= 2
//             DIAG(F("[RS485] Poll → Node %u Dev %s"),
//                  _nodeAddress, d.i2c.toString());
//                  #endif
//        // }
//        // handleIncoming(); move to start
//         } 
// }

// void RS485_Node::handleIncoming() {
// // Ignore echo for 300 microseconds after sending poll
// if (_waitingForReply && (micros() - _lastPollSent) < 300) {
//     // Don't read anything yet
//     return;
// }

//     while (_serial->available()) {
//         char c = _serial->read();

//         if (c == '<') {
//             _inPacket = true;
//             _rxPos = 0;
//             continue;
//         }

//         if (c == '>') {
//     _rxBuf[_rxPos] = 0;
//     _inPacket = false;
// #if RS485_DEBUG >= 2
//     DIAG(F("[RS485] RAW RX: <%s>"), _rxBuf);  
// #endif
//     processPacket(_rxBuf);
//     continue;
// }


//         if (!_inPacket)
//             continue;

//         if (c == '>') {
//             _rxBuf[_rxPos] = 0;
//             _inPacket = false;
//             processPacket(_rxBuf);
//             continue;
//         }

//         if (_rxPos < sizeof(_rxBuf) - 1)
//             _rxBuf[_rxPos++] = c;
//     }
// }
// void RS485_Node::processPacket(const char* p) {

//     // Ignore poll echoes (pure serial mode)
//     if (p[0] == 'P') {
//         // DO NOT clear _waitingForReply here
//         return;
//     }

//     // Expect: R node i2c mask
//     int node, i2c;
//     unsigned int rawMask;
//     if (sscanf(p, "R %d %d %d", &node, &i2c, &rawMask) != 3){
//         #if RS485_DEBUG >= 1
//       DIAG(F("[RS485] UNKNOWN PACKET: <%s>"), p);
//       #endif
//         return;
//     }
// // If reply is for us
//     if (node == _nodeAddress) {

//         // If we were waiting, this is the reply
//         if (_waitingForReply) {
//             #if RS485_DEBUG >= 3
//             DIAG(F("[RS485] REPLY: <%s>"), p);
//             #endif
//             _waitingForReply = false;
//         } else {
//             // Late reply — still show it
//             #if RS485_DEBUG >= 1
//             DIAG(F("[RS485] LATE REPLY: <%s>"), p);
//             #endif
//         }
//     }
//     if (node != _nodeAddress)
//         return;

//     // Now we have a REAL reply → clear waiting flag
//     _waitingForReply = false;

//     // Handle queued writes
//     if (_writePending) {
//         _writePending = false;
//         sendWrite(_wpI2C, _wpPin, _wpValue);
//     }

//     // Find device
//     RS485_RemoteDef* d = nullptr;
//     for (uint8_t i = 0; i < _count; i++) {
//         if (_dev[i].i2c == I2CAddress(i2c)) {
//             d = &_dev[i];
//             break;
//         }
//     }
//     if (!d) return;

// if (rawMask == (unsigned int)-1) {
//     // Device is offline
//      if (d->online) {
//         DIAG(F("[RS485] Node %u: device I2C %s went OFFLINE"),
//              _nodeAddress, I2CAddress(i2c).toString());
//     }
//             d->online=false;  
//     return;
// }

// if (rawMask == d->lastMask) {
//     return;   // no change → skip VPIN updates
// }




// d->lastMask = rawMask;

// }

uint16_t RS485_Node::getCurrentMask(IODevice* device) {

    for (int i = 0; i < _count; i++) {
        if (_dev[i].dev == device) {
            return _dev[i].lastMask;
        }
    }

    return 0;   // or 0xFFFF if you prefer "all high"
}

 //void RS485_Node::scanI2C() {
//     DIAG(F("[RS485] Node %u: scanning all remote I2C addresses..."), _nodeAddress);

//     char buf[32];
//     uint8_t pos;
//     bool inPacket;

//     bool foundAny = false;

//     for (uint8_t addr = 0; addr < 128; addr++) {

// //farm this out to the buscontroller

//         // // --- Send discovery request for this I2C address ---
//         // beginTx();
//         // _serial->print("<D ");
//         // _serial->print(_nodeAddress);
//         // _serial->print(" ");
//         // _serial->print(addr);
//         // _serial->print(">");
//         // _serial->flush();
//         // endTx();

//         // --- Wait for <RD,i2c,true|false> ---
//         unsigned long deadline = millis() + 20;
//         pos = 0;
//         inPacket = false;

//         bool gotRD = false;
//         uint8_t rdI2C = 0;
//         bool alive = false;

//         while (millis() < deadline) {
//             while (_serial->available()) {
//                 char c = _serial->read();

//                 if (c == '<') {
//                     inPacket = true;
//                     pos = 0;
//                     continue;
//                 }

//                 if (c == '>') {
//                     buf[pos] = 0;
//                     inPacket = false;

//                     // Expect: RD,i2c,true|false
//                     if (strncmp(buf, "RD", 2) == 0) {
//                         int i2c;
//                         char aliveStr[8];

//                         if (sscanf(buf, "RD %d %7s", &i2c, aliveStr) == 2) {
//                             rdI2C = (uint8_t)i2c;
//                             alive = (strcmp(aliveStr, "true") == 0 || strcmp(aliveStr, "1") == 0);
//                             gotRD = true;
//                         }
//                     }
//                     break;
//                 }

//                 if (inPacket && pos < sizeof(buf) - 1)
//                     buf[pos++] = c;
//             }

//             if (gotRD) break;
//         }

//         // --- Only log TRUE replies ---
//         if (gotRD && alive) {
//             // Mark matching device as alive
// for (uint8_t i = 0; i < _count; i++) {
//     if (_dev[i].i2c == I2CAddress(rdI2C)) {
//         _dev[i].online = true;
//     }
// }
//             DIAG(F("[RS485] Node %u I2C 0X%02X present"),_nodeAddress,(uint32_t)rdI2C );
//             foundAny = true;
//         }
//     }

//     if (!foundAny) {
//         DIAG(F("[RS485] No I2C devices found on Node: %u"),_nodeAddress);
//     } else {
//         DIAG(F("[RS485] Node %u: I2C scan complete"), _nodeAddress);
//     }
//}




