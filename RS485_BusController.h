#ifndef RS_485_BUS_CONTROLLER_H
#define RS_485_BUS_CONTROLLER_H
#include"RS485_IODevice.h"
#include"IO_RS485_Node.h"
#define MAX_NODES 16
class RS485_Node;
class RS485_IODevice;

class RS485BusController : public IODevice {
public:
    static RS485BusController* create(HardwareSerial& bus, uint8_t dePin, uint16_t intervalMs);

    void addNode(RS485_Node* node);
void init();
    protected:
    void _loop(unsigned long now) override;
void _display()override;

private:
    RS485BusController(HardwareSerial& bus, uint8_t dePin, uint16_t intervalMs);

    HardwareSerial* _bus;
    uint8_t _dePin;

struct PendingWrite {
    uint8_t node;
    I2CAddress i2c;
    uint16_t mask;
    bool valid;
};

PendingWrite _pendingWrite;


   RS485_Node* _nodes[MAX_NODES];   // fixed pointer array
    uint8_t _nodeCount;                    // how many are actually used

    size_t _nextNode = 0;

    bool _waitingReply = false;
    unsigned long _replyDeadline = 0;

    RS485_Node* _currentNode = nullptr;
    IODevice* _currentDevice = nullptr;
    void _begin()override;
    void sendPoll(RS485_Node* _currentNode,IODevice* _currentDevice);
    void sendWriteFrame(const PendingWrite& w);
    void queueWrite(uint8_t node, I2CAddress i2c, uint16_t mask);
    void handleIncoming();
    void processReply(const char* frame);
    bool replyReady();
    void routeReplyToNode();
    void logTimeout();
    void scanNode(RS485_Node* node);
    void sendDiscovery(uint8_t nodeAddr);
    void startupScan();
    uint8_t getNodeCount()const;
RS485_Node* getNodeByAddress(uint8_t addr);
    RS485_Node* nextNode();
    RS485_Node* getNode(uint8_t index);
       // --- RX buffer state ---
    static constexpr size_t RXBUF_SIZE = 64;   // plenty for <R 1 32 65535>
    char   _rxBuf[RXBUF_SIZE];
    size_t _rxPos = 0;

    // --- Parsed reply fields ---
    bool     _replyReady = false;
    uint8_t  _replyNode  = 0;
    I2CAddress _replyI2C   = 0;
    uint16_t _replyMask  = 0;
};

#endif //RS485_BUS_CONTROLLER