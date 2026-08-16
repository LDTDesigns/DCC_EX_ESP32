#ifndef RS_485_BUS_CONTROLLER_H
#define RS_485_BUS_CONTROLLER_H
#include"RS485_IODevice.h"
#include"IO_RS485_Node.h"
#define MAX_NODES 16
#define MAX_RETRIES 2

class RS485_Node;
class RS485_IODevice;

class RS485BusController : public IODevice {
public:
    static RS485BusController* create(HardwareSerial& bus, uint8_t dePin, uint16_t intervalMs);

    void addNode(RS485_Node* node);
void init();
 void queueWrite(uint8_t node, I2CAddress i2c, uint16_t mask, uint8_t pin=0, uint8_t profile=0, uint16_t duration=0, uint8_t deviceType=DEVICE_TYPE_DEFAULT);

enum DeviceType {
    DEVICE_TYPE_DEFAULT,
    DEVICE_TYPE_INPUT,
    DEVICE_TYPE_OUTPUT,
    DEVICE_TYPE_SERVO,
    DEVICE_TYPE_ANALOGOUTPUT,
    DEVICE_TYPE_ANALOGINPUT,
    DEVICE_TYPE_TURNTABLE
};
//using DeviceType = RS485BusController::DeviceType;

 protected:
    void _loop(unsigned long now) override;
void _display()override;
 

private:
    RS485BusController(HardwareSerial& bus, uint8_t dePin, uint16_t intervalMs);

    HardwareSerial* _bus;
    uint8_t _dePin;

   const unsigned long _intervalMs;

      enum ReplyType {
        REPLY_NONE,
        REPLY_DISCOVERY,
        REPLY_POLL,
        REPLY_WRITE,
        REPLY_ACK
    } ;


  unsigned long lastPollTime=0;

   static uint16_t nextMsgId;

   RS485_Node* _nodes[MAX_NODES];   // fixed pointer array
    uint8_t _nodeCount;                    // how many are actually used

    size_t _nextNode = 0;

 //   bool _waitingReply = false;
    unsigned long _replyDeadline = 0;

    RS485_Node* _currentNode = nullptr;
    IODevice* _currentDevice = nullptr;
    void _begin()override;
    void sendPoll(RS485_Node* _currentNode,IODevice* _currentDevice);
    void sendWriteFrame(uint8_t node, I2CAddress i2c, uint16_t mask, uint8_t pin, uint8_t profile, uint16_t duration, uint8_t deviceType, uint16_t msgId);
    void handleIncoming();
    void retryWrite();
    void handleTimeout(ReplyType expected);
    void processReply(const char* frame);
    bool replyReady();
    void routeReplyToNode();
    void pollTimeout();
    void scanNode(RS485_Node* node);
    void sendDiscovery(uint8_t nodeAddr);
    void startupScan();
    
    uint8_t getNodeCount()const;
RS485_Node* getNodeByAddress(uint8_t addr);
    RS485_Node* nextNode();
    RS485_Node* getNode(uint8_t index);

    bool pollDue();
//---fifo----
void removeFromFifo();
bool handleAck(uint16_t ackMsgId, bool ok);

struct QueuedWrite {
    uint8_t node;
    I2CAddress i2c;
    uint16_t mask;
    uint8_t pin;
    uint8_t profile;
    uint16_t duration;
    uint16_t msgId;
    uint8_t retries;
    uint8_t deviceType; // New field to indicate the type of device (input, output, servo, etc.)
    bool valid;
};


static const uint8_t FIFO_SIZE = 16;
QueuedWrite writeFifo[FIFO_SIZE];
uint8_t writeHead = 0;
uint8_t writeTail = 0;

//------------
    uint16_t allocateMsgId();
       // --- RX buffer state ---
    static constexpr size_t RXBUF_SIZE = 64;   // plenty for <R 1 32 65535>
    char   _rxBuf[RXBUF_SIZE];
    size_t _rxPos = 0;

    // --- Parsed reply fields ---
    bool     _replyReady = false;
    uint8_t  _replyNode  = 0;
    I2CAddress _replyI2C   = 0;
    int _replyMask  = 0;
    bool     _replyAlive = false;
    uint16_t _replyMsgId;
bool     _replyStatus;

 
    ReplyType expectedReply = REPLY_NONE;
    ReplyType parsedReply = REPLY_NONE ;   

    enum BusState {
    BUS_IDLE,
    BUS_TX,
    BUS_WAIT_REPLY
};

BusState busState = BUS_IDLE;

};

#endif //RS485_BUS_CONTROLLER