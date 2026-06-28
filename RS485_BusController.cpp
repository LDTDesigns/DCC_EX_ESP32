#include"RS485_BusController.h"

#define REPLY_TIMEOUT 50
#define WRITE_REPLY_TIMEOUT 50
#define RS485_BAUD 19200
#define BUS_DIAG 4

RS485BusController* RS485BusController::create(HardwareSerial& bus, uint8_t dePin, uint16_t intervalMs) {
    RS485BusController* bc = new RS485BusController(bus, dePin, intervalMs);
    addDevice(bc);   // HAL registration
    return bc;
}

RS485BusController::RS485BusController(HardwareSerial& bus, uint8_t dePin, uint16_t intervalMs)
    : _bus(&bus), _dePin(dePin),_nodeCount(0)
{
    pinMode(_dePin, OUTPUT);
    digitalWrite(_dePin, LOW);   // receive mode
      // Initialise node table
    for (uint8_t i = 0; i < MAX_NODES; i++) {
        _nodes[i] = nullptr;
    }
}
void RS485BusController::addNode(RS485_Node* node)
{
    uint8_t addr = node->getNodeAddress();

    // 1. Enforce MAX_NODES
    if (_nodeCount >= MAX_NODES)
    {
        DIAG(F("[RS485] ERROR: Maximum nodes (%d) reached"), (int)MAX_NODES);
        return;
    }

    // 2. Duplicate guard
    for (uint8_t i = 0; i < _nodeCount; i++)
    {
        if (_nodes[i]->getNodeAddress() == addr)
        {
            DIAG(F("[RS485] ERROR: Node ID %d already exists"), (int)addr);
            return;
        }
    }

    // 3. Add node to registry
    _nodes[_nodeCount++] = node;

    DIAG(F("[RS485] Node %d added, nodeCount=%d"),
         (int)addr,
         (int)_nodeCount);
}

void RS485BusController::sendPoll( RS485_Node* node,IODevice* device){

        // --- Resolve device I2C address from the node registry ---
    I2CAddress i2c;
    bool found = false;

    for (uint8_t i = 0; i < node->_count; i++)
    {
        if (node->_dev[i].dev == device)
        {
            i2c = node->_dev[i].i2c;
            found = true;
            break;
        }
    }

    if (!found)
        return;     // device not registered on this node


    // --- Switch bus to TX mode ---
    digitalWrite(_dePin, HIGH);


    // --- Send poll frame ---
    HardwareSerial* s = _bus;

    s->print("<P ");
    s->print((int)node->_nodeAddress);      // node address
    s->print(" ");
    s->print(i2c.toString());       // device I2C address
    s->print(">");

    s->flush();                     // ensure full contiguous burst


    // --- Switch bus back to RX mode ---
    digitalWrite(_dePin, LOW);


#if BUS_DIAG >= 2
    DIAG(F("[RS485] Poll → Node %u Dev %s"),
         node->_nodeAddress, i2c.toString());
#endif
}

uint8_t RS485BusController::getNodeCount() const {
    return _nodeCount;
}

RS485_Node* RS485BusController::getNode(uint8_t index) {
    if (index >= _nodeCount) return nullptr;
    return _nodes[index];
}
void RS485BusController::handleIncoming()
{
    while (_bus->available())
    {
        char c = _bus->read();

        // --- Start of frame ---
        if (c == '<')
        {
            _rxPos = 0;
            _rxBuf[_rxPos++] = c;
            continue;
        }

        // --- Accumulate ---
        if (_rxPos < sizeof(_rxBuf) - 1)
        {
            _rxBuf[_rxPos++] = c;
        }

        // --- End of frame ---
        if (c == '>')
        {
            _rxBuf[_rxPos] = '\0';   // null‑terminate

#if RS485_DEBUG >= 2
            DIAG(F("[RS485] RX: %s"), _rxBuf);
#endif

            processReply(_rxBuf);
            _rxPos = 0;
        }
    }
}
void RS485BusController::processReply(const char* frame)
{
    char tag;
    uint8_t node;
    uint16_t rawI2C;
    uint16_t mask;

    int n = sscanf(frame, "<%c %hhu %hu %hu>", &tag, &node, &rawI2C, &mask);

    if (n != 4 || tag != 'R')
        return;

    _replyNode = node;
    _replyI2C  = I2CAddress((uint8_t) rawI2C);
    _replyMask = mask;
    _replyReady = true;
}


bool RS485BusController::replyReady() 
{
    return _replyReady;
}


void RS485BusController::routeReplyToNode()
{
    _replyReady = false;

    RS485_Node* node = getNodeByAddress(_replyNode);

    if (!node)
    {
    #ifdef BUS_DIAG >= 1
         DIAG(F("[BusController] Reply for unknown node %d"), (int)_replyNode);
         #endif
    
        return;
    }

    for (uint8_t i = 0; i < node->_count; i++)
    {
        RS485_RemoteDef& d = node->_dev[i];

        if (d.i2c == _replyI2C)
        {
            d.online = true;
            if(d.lastMask!=_replyMask)
            {
            d.lastMask = _replyMask;
            }
            // Optional: log successful routing
            #ifdef BUS_DIAG >= 1
            DIAG(F("[BusController] Node %d device $s updated mask=0x%02X"),
                 (int)_replyNode,
                 _replyI2C.toString(),
                 (int)_replyMask);
                 #endif

            return;
        }
    }
}
void RS485BusController::queueWrite(uint8_t node, I2CAddress i2c, uint16_t mask)
{
#if BUS_DIAG >= 1
    DIAG(F("[RS485] QUEUE WRITE → Node %u I2C %s Mask 0x%04X"),
         node, i2c.toString(), mask);
#endif

    _pendingWrite.node  = node;
    _pendingWrite.i2c   = i2c;
    _pendingWrite.mask  = mask;
    _pendingWrite.valid = true;
}


void RS485BusController::sendWriteFrame(const PendingWrite& w)
{
#if BUS_DIAG >= 1
    DIAG(F("[RS485] SEND WRITE → <W %u %s 0x%04X>"),
         w.node,
         w.i2c.toString(),
         w.mask);
#endif

    // --- Enable TX ---
    digitalWrite(_dePin, HIGH);
    delayMicroseconds(50);   // allow driver to settle
    _bus->print('<');
    _bus->print('W');
    _bus->print(' ');
    _bus->print(w.node);
    _bus->print(' ');
    _bus->print(w.i2c.toString());
    _bus->print(' ');
    _bus->print(w.mask, HEX);
    _bus->print('>');
     _bus->flush();           // ensure all bytes leave UART

    delayMicroseconds(50);   // allow last byte to exit transceiver

    // --- Back to RX ---
    digitalWrite(_dePin, LOW);
}


RS485_Node* RS485BusController::getNodeByAddress(uint8_t addr)
{
    for (uint8_t i = 0; i < _nodeCount; i++)
    {
        if (_nodes[i]->getNodeAddress() == addr)
            return _nodes[i];
    }
    return nullptr;
}


void RS485BusController::logTimeout() {
        for (uint8_t i = 0; i < _currentNode->_count; i++)
    {
        RS485_RemoteDef& d = _currentNode->_dev[i];

        if (d.dev == _currentDevice)
        {
            d.online = false;
            break;
        }
    }
}

  RS485_Node* RS485BusController::nextNode()
{
    if (_nodeCount == 0)
        return nullptr;

    _nextNode = (_nextNode + 1) % _nodeCount;
    return _nodes[_nextNode];
}




void RS485BusController::_loop(unsigned long now) {

    handleIncoming();   // always read bus first
  // --- Startup guard ---
    if (_currentNode == nullptr)
    {
        if (_nodeCount == 0)
            return;

        _currentNode = _nodes[0];
    }
    if (_waitingReply) {
        if (replyReady()) {
            routeReplyToNode();
            _waitingReply = false;
        }
        else if (millis() > _replyDeadline) {
            logTimeout();
            _waitingReply = false;
        }
        return;
    }
    if (!_waitingReply && _pendingWrite.valid)
{
    sendWriteFrame(_pendingWrite);
    _pendingWrite.valid = false;
    _waitingReply = true;
    _replyDeadline = millis() + WRITE_REPLY_TIMEOUT;
    return;
}

    // Not waiting → send next poll
    IODevice* dev = _currentNode->nextDevice();

if (dev == nullptr)
{
    _currentNode = nextNode();     // only when device list wraps
    dev = _currentNode->nextDevice();
}

_currentDevice = dev;

    sendPoll(_currentNode,_currentDevice);

    _replyDeadline = millis() + REPLY_TIMEOUT;
    _waitingReply  = true;
}
void RS485BusController::scanNode(RS485_Node* node)
{
    uint8_t addr = node->getNodeAddress();

    sendDiscovery(addr);

    unsigned long deadline = millis() + 50;

    // Clear online flags
    for (uint8_t i = 0; i < node->_count; i++)
        node->_dev[i].online = false;

    while (millis() < deadline)
    {
        handleIncoming();

        if (_replyReady)
        {
              // Only process replies for THIS node
            if (_replyNode == addr)
            {
                I2CAddress i2c = _replyI2C;
                bool found = false;

                // Match reply to registered devices
                for (uint8_t i = 0; i < node->_count; i++)
                {
                    if (node->_dev[i].i2c == i2c)
                    {
                        node->_dev[i].online = true;
                        DIAG(F("[BusController] Node %u device %s online"), addr, i2c.toString());
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    DIAG(F("[BusController] Node %u reports UNREGISTERED device at I2C %s"),
                         addr, i2c.toString());
                }
            }

            _replyReady = false;
        }
    }

    // Log offline registered devices

if (node->_count == 0)
{
    DIAG(F("[BusController] Node %u has NO registered devices"), addr);
}
else
{
    for (uint8_t i = 0; i < node->_count; i++)
    {
        if (!node->_dev[i].online)
        {
            DIAG(F("[BusController] Node %u device at %s offline"),
                 addr, node->_dev[i].i2c.toString());
        }
    }
}
    
}
void RS485BusController::sendDiscovery(uint8_t nodeAddr)
{
     // Log the discovery request
    DIAG(F("[BusController] Discovery → Node %u"), nodeAddr);
    digitalWrite(_dePin, HIGH);
    _bus->print("<D ");
    _bus->print((int)nodeAddr);
    _bus->print(">");

    _bus->flush();
    digitalWrite(_dePin, LOW);
}
void RS485BusController::startupScan()
{


    for (uint8_t i = 0; i < _nodeCount; i++)
    {
        RS485_Node* node = _nodes[i];
        if (!node)
            continue;

        scanNode(node);
    }
}
void RS485BusController::init()
{
    
    startupScan();
 _display();
    if (_nodeCount > 0)
        _currentNode = _nodes[0];
       
}
void RS485BusController::_begin()
{
    // Configure RS485 driver enable pin
    pinMode(_dePin, OUTPUT);
    digitalWrite(_dePin, LOW);   // receive mode

    // Start RS485 serial
    _bus->begin(RS485_BAUD);

    // Reset RX state
    _rxPos = 0;
    _replyReady = false;

    // DO NOT scan here
    // DO NOT poll here
    // Devices and nodes are not fully registered yet
}
void RS485BusController::_display()
{
  DIAG(F("[RS485BusController] Nodes: %d Baud %lu"), (int)_nodeCount,(unsigned long) RS485_BAUD );
  #if BUS_DIAG >= 0
if (_bus == &Serial)      DIAG(F("[RS485] Using Serial"));
if (_bus == &Serial1)     DIAG(F("[RS485] Using Serial1"));
if (_bus == &Serial2)     DIAG(F("[RS485] Using Serial2"));
if (_bus == &Serial3)     DIAG(F("[RS485] Using Serial3"));
#endif
}
