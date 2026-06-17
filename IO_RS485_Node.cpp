#include "IO_RS485_Node.h"
#include "RemoteDevice.h"
// ---------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------

// AUTO mode
RS485_Node::RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort)
: IODevice(0, 0)
{
    _nodeAddress = nodeAddress;
    _serial      = &serialPort;
    _txPin       = 255;
    _autoMode    = true;

    _count = 0;
    for (int i = 0; i < MAX_COMPONENTS_PER_NODE; i++) {
        _registry[i] = nullptr;
    }

    DIAG(F("[RS485] Node %u constructed on AUTO mode"), _nodeAddress);

    addDevice(this);
}

// MANUAL mode
RS485_Node::RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txPin)
: IODevice(0, 0)
{
    _nodeAddress = nodeAddress;
    _serial      = &serialPort;
    _txPin       = txPin;
    _autoMode    = false;

    pinMode(_txPin, OUTPUT);
    digitalWrite(_txPin, LOW);   // start in RX

    _count = 0;
    for (int i = 0; i < MAX_COMPONENTS_PER_NODE; i++) {
        _registry[i] = nullptr;
    }

    DIAG(F("[RS485] Node %u constructed on MANUAL mode (DE pin=%u)"),
         _nodeAddress, _txPin);

    addDevice(this);
}

// ---------------------------------------------------------------------
// Component registration
// ---------------------------------------------------------------------

void RS485_Node::registerComponent(RemoteDevice* comp) {
    if (_count >= MAX_COMPONENTS_PER_NODE || !comp) {
        DIAG(F("[RS485] Node %u: registry full or null component"), _nodeAddress);
        return;
    }

    _registry[_count++] = comp;

    // Light-weight confirmation
    DIAG(F("[RS485] Node %u: component registered I2C 0x%02X VPIN %u–%u"),
         _nodeAddress,
         comp->getAddress(),
         comp->getStartVpin(),
         comp->getStartVpin() + comp->getPinCount() - 1);
}

// ---------------------------------------------------------------------
// IODevice interface (stubs – your existing logic can drop in here)
// ---------------------------------------------------------------------

int RS485_Node::_read(VPIN vpin) {
    // Your existing virtual register / cache logic goes here
    // For now, just return 0
    return 0;
}

void RS485_Node::_write(VPIN vpin, int value) {
    // Your existing packet send / outbox logic goes here
    // Respect _autoMode and _txPin for direction control
    if (!_autoMode) {
        digitalWrite(_txPin, HIGH);
        delayMicroseconds(5);
    }

    // ... pack + send frame via *_serial ...

    if (!_autoMode) {
        _serial->flush();
        digitalWrite(_txPin, LOW);
    }
}

void RS485_Node::_loop(unsigned long now) {
    // Your existing poll / RX state machine logic goes here
    // This is just a placeholder to keep the class complete
}

// ---------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------

void RS485_Node::_display() {

    const char* portName = "Serial?";
    if (_serial == &Serial)  portName = "Serial";
    if (_serial == &Serial1) portName = "Serial1";
    if (_serial == &Serial2) portName = "Serial2";
    if (_serial == &Serial3) portName = "Serial3";

    DIAG(F("[RS485] Node %u on %s (%s)"),
         _nodeAddress,
         portName,
         _autoMode ? "AUTO" : "MANUAL");

    if (!_autoMode) {
        DIAG(F("        Direction pin: %u"), _txPin);
    }

    DIAG(F("        %d components registered"), _count);

    for (int i = 0; i < _count; i++) {
        if (_registry[i]) {
            _registry[i]->_display();
        }
    }
}

