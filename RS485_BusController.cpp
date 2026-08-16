#include"RS485_BusController.h"

#define REPLY_TIMEOUT 50
#define WRITE_REPLY_TIMEOUT 50
#define RS485_BAUD 57600
#define BUS_DIAG 2

 uint16_t RS485BusController::nextMsgId = 1;


RS485BusController* RS485BusController::create(HardwareSerial& bus, uint8_t dePin, uint16_t intervalMs) {
    RS485BusController* bc = new RS485BusController(bus, dePin, intervalMs);
    addDevice(bc);   // HAL registration
    return bc;
}

RS485BusController::RS485BusController(HardwareSerial& bus, uint8_t dePin, uint16_t intervalMs)
    : _bus(&bus), _dePin(dePin),_intervalMs(intervalMs),_nodeCount(0)
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
bool RS485BusController::pollDue()
{

    if (millis()>lastPollTime+_intervalMs){
        lastPollTime=millis();
        return true;
    }
    return false;
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
delayMicroseconds(30);
    s->print("<P ");
    s->print((int)node->_nodeAddress);      // node address
    s->print(" ");
    s->print(i2c.toString());       // device I2C address
    s->print(">");

    s->flush();                     // ensure full contiguous burst
    // --- Switch bus back to RX mode ---
    delayMicroseconds(30);
    digitalWrite(_dePin, LOW);

busState=BUS_WAIT_REPLY;
_replyDeadline=millis()+REPLY_TIMEOUT;
expectedReply=REPLY_POLL;
#if BUS_DIAG >= 3
    DIAG(F("[RS485] Poll → Node %u Dev %s"),
         node->_nodeAddress, i2c.toString());
#endif
}

uint16_t RS485BusController::allocateMsgId() {
    return  nextMsgId++;
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
    parsedReply = REPLY_NONE;
    _replyReady = false;

    // Quick header check
    char type = frame[1];

    // -----------------------------
    // 1. ACK reply: <A msgId status>
    // -----------------------------
    if (type == 'a') {
        uint16_t ackMsgId;
        char statusStr[8];

        if (sscanf(frame, "<a %u %7[^>]>", &ackMsgId, statusStr) == 2) {
            _replyMsgId = ackMsgId;
            _replyStatus = (strcmp(statusStr, "OK") == 0);
            parsedReply = REPLY_ACK;
            _replyReady = true;
            return;
        }

        if (sscanf(frame, "<a %u>", &ackMsgId) == 1) {
            _replyMsgId = ackMsgId;
            _replyStatus = true;
            parsedReply = REPLY_ACK;
            _replyReady = true;
            return;
        }

        return; // malformed ACK
    }

    // -----------------------------
    // 2. Discovery reply: <RD node i2c alive>
    // -----------------------------
    if (type == 'd') {
        uint8_t nodeaddr, rawI2C;
        char aliveStr[8];

        if (sscanf(frame, "<d %hhu %hhu %7[^>]>", &nodeaddr, &rawI2C, aliveStr) == 3) {
            _replyNode  = nodeaddr;
            _replyI2C   = I2CAddress(rawI2C);
            _replyAlive = (strcmp(aliveStr, "true") == 0);
            parsedReply = REPLY_DISCOVERY;
            _replyReady = true;
            return;
        }

        return; // malformed discovery
    }

    // -----------------------------
    // 3. Poll reply: <R node i2c mask>
    // -----------------------------
    if (type == 'r') {
        uint8_t nodeaddr, rawI2C;
        uint16_t mask;
        char tag;

        if (sscanf(frame, "<%c %hhu %hhu %u>", &tag, &nodeaddr, &rawI2C, &mask) == 4 && tag == 'r') {
            _replyNode = nodeaddr;
            _replyI2C  = I2CAddress(rawI2C);
            _replyMask = mask;
            parsedReply = REPLY_POLL;
            _replyReady = true;
            return;
        }

        return; // malformed poll
    }

    // -----------------------------
    // Unknown frame
    // -----------------------------
}


bool RS485BusController::replyReady() 
{
    return _replyReady;
}


void RS485BusController::routeReplyToNode()
{
    RS485_Node* node = getNodeByAddress(_replyNode);

    if (!node)
    {
    #if BUS_DIAG >= 1
         DIAG(F("[BusController] Reply for unknown node %d"), (int)_replyNode);
         #endif
    
        return;
    }

    for (uint8_t i = 0; i < node->_count; i++)
    {
        RS485_RemoteDef& d = node->_dev[i];

        if (d.i2c == _replyI2C)
        {
          
            if(_replyMask!= -1){
            d.lastSeen = millis();
               d.online = true;/// Update last seen timestamp -1 means node didnt find it
            }
            if(d.lastMask!=_replyMask)
            {
            d.lastMask = _replyMask;
            }
            // Optional: log successful routing
            #if BUS_DIAG >= 3
            DIAG(F("[BusController] Node %d device %s updated mask=0x%02X"),
                 (int)_replyNode,
                 _replyI2C.toString(),
                 _replyMask);
                 #endif

            return;
        }
    }
}
void RS485BusController::queueWrite(uint8_t node, I2CAddress i2c, uint16_t mask, uint8_t pin, uint8_t profile, uint16_t duration, uint8_t deviceType)
{

uint8_t nextTail = (writeTail + 1) % FIFO_SIZE;

    if (nextTail == writeHead) {
        DIAG(F("[RS485] FIFO FULL — dropping write"));
        return;
    }

    QueuedWrite &w = writeFifo[writeTail];
    w.node   = node;
    w.i2c    = i2c;
    w.mask   = mask;
    w.msgId  = allocateMsgId();
    w.retries = 0;
    w.valid  = true;
    w.pin = pin;
    w.profile = profile;
    w.duration = duration;  
    w.deviceType = deviceType;
    writeTail = nextTail;

    #if BUS_DIAG >= 2
    DIAG(F("[BusController] QUEUE WRITE → Node %u I2C %s Mask 0x%04X Pin %u Profile %u Duration %u DeviceType %u"),
         node, i2c.toString(), mask, pin, profile, duration, deviceType);
#endif

}


void RS485BusController::sendWriteFrame(uint8_t node, I2CAddress i2c, uint16_t mask, uint8_t pin, uint8_t profile, uint16_t duration, uint8_t deviceType, uint16_t msgId)
{
   // --- Enable TX ---
    digitalWrite(_dePin, HIGH);
    delayMicroseconds(30);   // allow driver to settle
       _bus->print('<');
  
    switch (deviceType) {
        case DEVICE_TYPE_DEFAULT:
            // Default behavior, no additional handling needed
            #if BUS_DIAG >= 2
    DIAG(F("[BusController] SEND WRITE → <W %u %s %u MsgID %u>"),
         node,
        i2c.toString(),
         mask,
         msgId);
#endif
            _bus->print('W');
    _bus->print(' ');
    _bus->print(node);
    _bus->print(' ');
    _bus->print((unsigned int)i2c);
    _bus->print(' ');
             _bus->print(mask, DEC);
            break;
        case DEVICE_TYPE_INPUT:
            // Handle input device specifics if needed
            break;
        case DEVICE_TYPE_OUTPUT:
            // Handle output device specifics if needed
            break;
        case DEVICE_TYPE_SERVO:
            // Handle servo device specifics if needed
            break;
        case DEVICE_TYPE_ANALOGOUTPUT:
            // Handle analog output device specifics if needed
            break;
        case DEVICE_TYPE_ANALOGINPUT:
            // Handle analog input device specifics if needed
            break;
        case DEVICE_TYPE_TURNTABLE:{

            // Handle turntable device specifics if needed
            // Take the 16 bit mask and split it into LSB and MSB,add activity
             // TURNTABLE: <T node i2c msb lsb activity>
            uint8_t lsb = mask & 0xFF;
            uint8_t msb = (mask >> 8) & 0xFF;
                        #if BUS_DIAG >= 2
    DIAG(F("[BusController] SEND WRITE → <T %u %s %u %u %u MsgID %u>"),
         node, i2c.toString(), msb, lsb,
         profile,
         msgId);
#endif
             _bus->print('T');
    _bus->print(' ');
    _bus->print(node);
    _bus->print(' ');
    _bus->print((unsigned int)i2c);
    _bus->print(' ');
            _bus->print(msb, DEC);  
            _bus->print(' ');
            _bus->print(lsb, DEC);
            _bus->print(' ');
            _bus->print(profile, DEC);

            break;
        }
        default:
            DIAG(F("[BusController] Unknown device type %u"), deviceType);
            return; // Exit if the device type is unknown
    }


    _bus->print(' ');
    _bus->print(msgId, DEC);
    _bus->print('>');
    _bus->flush();           // ensure all bytes leave UART

    delayMicroseconds(30);   // allow last byte to exit transceiver

    // --- Back to RX ---
    digitalWrite(_dePin, LOW);
    
    busState= BUS_WAIT_REPLY;
    _replyDeadline=millis()+WRITE_REPLY_TIMEOUT;
    expectedReply=REPLY_ACK;
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


void RS485BusController::pollTimeout() {
        for (uint8_t i = 0; i < _currentNode->_count; i++)
    {
        RS485_RemoteDef& d = _currentNode->_dev[i];

        if (d.dev == _currentDevice)
        {
DIAG(F("[RS485] Timeout waiting for reply from Node %u Device %s"),
             _currentNode->getNodeAddress(),
             d.i2c.toString());

            d.online = false;
          //  d.lastSeen=millis();
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

    switch (busState)
    {

    case BUS_IDLE:
        if (writeHead != writeTail)
        {
            QueuedWrite &w = writeFifo[writeHead];
            sendWriteFrame(w.node, w.i2c, w.mask,w.pin, w.profile, w.duration,w.deviceType, w.msgId);
        }
        else if (pollDue())
        {
            IODevice *dev = _currentNode->nextDevice();
            if (dev == nullptr)
            {
                _currentNode = nextNode();        // only when device list wraps
                dev = _currentNode->nextDevice(); // once weve polled all the nodes and devices, we will wrap around and start again , but add the poll deelay to avoid flooding the bus with polls
            }
            _currentDevice = dev;
            sendPoll(_currentNode, _currentDevice);
        }
        break;

    case BUS_WAIT_REPLY:
        if (_replyReady)
        {

            if (parsedReply == expectedReply && expectedReply == REPLY_ACK)
            {

                // check msg ID matches?
                bool ackMatched = handleAck(_replyMsgId, _replyStatus);
                if (ackMatched)
                {
                    writeHead = (writeHead + 1) % FIFO_SIZE;
                    routeReplyToNode();
                    // pop FIFO, route poll, etc.
                    busState = BUS_IDLE;
                    parsedReply = REPLY_NONE;
                    _replyMsgId = 0;
                    _replyStatus = false;
                    _replyReady = false;
                }

                else
                {
                    // wrong reply → ignore
                    parsedReply = REPLY_NONE;
                    _replyReady = false;
                }
            }
            else if (parsedReply == expectedReply && parsedReply == REPLY_POLL)
            {
                routeReplyToNode();
                busState = BUS_IDLE;
                parsedReply = REPLY_NONE;
                _replyReady = false;
            }

            return; // yield
        }
        // no reply yet → check timeout
        if (millis() > _replyDeadline) {
            handleTimeout(expectedReply);
            expectedReply = REPLY_NONE;
            busState = BUS_IDLE;
        }
        return; 

        case BUS_TX:
            //  if (txComplete()) {       // flush + DE low
            //     busState = BUS_WAIT_REPLY;
            //      replyDeadline = now + timeoutFor(expectedReply);
            //   }
            return; // yield
        }
    
}

void RS485BusController::scanNode(RS485_Node* node)
{
    uint8_t addr = node->getNodeAddress();

    sendDiscovery(addr);
    expectedReply=REPLY_DISCOVERY;

    unsigned long deadline = millis() + 100;

    bool nodeReplied = false;
   // const uint8_t registered[] = { 0x20, 0x38 };
   // const uint8_t regCount = sizeof(registered) / sizeof(registered[0]);
uint8_t regCount = node->_count;

    I2CAddress discovered[MAX_RS485_DEVICES + 1];   // +1 for overflow
    uint8_t discoveredCount = 0;

    // Clear online flags
    for (uint8_t i = 0; i < node->_count; i++)
        node->_dev[i].online = false;

    // Collect replies
    while (millis() < deadline)
    {
        handleIncoming();

// lets do some log here see if that next statement can be true

        if (_replyReady && parsedReply == expectedReply)
        {
            nodeReplied = true;

            if (_replyAlive)
            {
                if (discoveredCount < MAX_RS485_DEVICES + 1)
                    discovered[discoveredCount++] = _replyI2C;
            }

            _replyReady = false;   // consume this reply
        }
    }
    #if BUS_DIAG >=3
DIAG(F("  Raw discoveredCount = %u"), discoveredCount);
for (uint8_t k = 0; k < discoveredCount; k++) {
    DIAG(F("  discovered[%u] = 0x%s"), k, discovered[k].toString());
}
DIAG(F("  Registered list dump:"));
for (uint8_t i = 0; i < regCount; i++) {
    DIAG(F("    registered[%u].i2c = 0x%s"), 
         i, node->_dev[i].i2c.toString());
}
#endif

    if (!nodeReplied)
    {
        DIAG(F("  Status: NO REPLY"));
        return;
    }
  #if BUS_DIAG >=3
DIAG(F("  Status: REPLIED, %u devices discovered"), discoveredCount);
DIAG(F("  Registered devices: %u"), regCount);
 #endif
    // Check registered devices
    for (uint8_t i = 0; i < regCount; i++)
    {
        I2CAddress reg = node->_dev[i].i2c;
        bool found = false;

 
        for (uint8_t j = 0; j < discoveredCount; j++)
        {
             #if BUS_DIAG >=3
            DIAG(F("  Checking discovered device %s against registered %s"),
                 discovered[j].toString(), reg.toString());
             #endif
            if (discovered[j] == reg)
            {
                #if BUS_DIAG >=3
                DIAG(F("  Match found for registered device %s"), reg.toString());
 #endif
                found = true;
                break;
            }
          #if BUS_DIAG >=3
            DIAG(F("  No match for registered device %s against discovered %s"), reg.toString(), discovered[j].toString());
           #endif
        }

        if (found){
            DIAG(F("  Present:   0x%s"), reg.toString());
            node->_dev[i].online = true;  // mark as online
    }
        else{
            DIAG(F("  Missing:   0x%s"), reg.toString());
              node->_dev[i].online = false;  // mark as offline
        }
    }

    // Check extras
    for (uint8_t j = 0; j < discoveredCount; j++)
    {
        I2CAddress det = discovered[j];
        bool isRegistered = false;

        for (uint8_t i = 0; i < regCount; i++)
        {
            if (node->_dev[i].i2c == det)
            {
                isRegistered = true;
                break;
            }
        }

        if (!isRegistered)
            DIAG(F("  Extra:     0x%s"), det.toString());
    }

    if (discoveredCount > MAX_RS485_DEVICES)
    {
        DIAG(F("  ERROR: Too many devices detected (%u > %u)"),
             discoveredCount, MAX_RS485_DEVICES);
    }
    busState=BUS_IDLE;
}

          
bool RS485BusController::handleAck(uint16_t ackMsgId, bool ok)
{
     QueuedWrite &w = writeFifo[writeHead];
     #if BUS_DIAG >=3
  DIAG(F("[BusController] FIFO head msgId=%u, incoming ACK msgId=%u"),
         w.msgId, ackMsgId);
         #endif
    if (w.msgId != ackMsgId) {

        #if BUS_DIAG >=3
        DIAG(F("[BusController] ACK for unknown msgId %u"), ackMsgId);
#endif

        return false;   // DO NOT clear waitingReply
    }

    // ACK matches → pop FIFO
  
    return true;

}
void RS485BusController::removeFromFifo()
{
writeHead=(writeHead+1)%FIFO_SIZE;
}
void RS485BusController::handleTimeout(ReplyType expected)
{


    if (expected == REPLY_ACK) {
        retryWrite();
    } else if (expected == REPLY_POLL) {
        pollTimeout();
    } else if (expected == REPLY_DISCOVERY) {
// should never get here as handled in send discovery
    }

    expectedReply = REPLY_NONE;
    parsedReply   = REPLY_NONE;

}

void RS485BusController::retryWrite()
{
    if (writeHead == writeTail)
        return; // nothing queued

    QueuedWrite &w = writeFifo[writeHead];

    if (w.retries < MAX_RETRIES) {
        w.retries++;

        DIAG(F("[BusController] Timeout → retry %u for msgId %u"),
             w.retries, w.msgId);
  
        // Reset reply state
        _replyReady = false;
        parsedReply = REPLY_NONE;
        expectedReply = REPLY_ACK;

        // Let dispatcher send the retry on next BUS_IDLE
        busState = BUS_IDLE;
 
        return;
    }

    // ⭐ Too many retries → drop it
    DIAG(F("[RS485] Write failed permanently for msgId %u"), w.msgId);

    removeFromFifo();
    busState=BUS_IDLE;
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
    busState=BUS_WAIT_REPLY;
    _replyDeadline=millis()+WRITE_REPLY_TIMEOUT;
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

//if (_bus == &Serial3)     DIAG(F("[RS485] Using Serial3"));
#endif
}
