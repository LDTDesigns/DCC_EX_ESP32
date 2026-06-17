#ifndef NETWORK_COMPONENT_H
#define NETWORK_COMPONENT_H
#include "IODevice.h"

class NetworkComponent {
public:
    virtual ~NetworkComponent() {}

    virtual void _display() = 0;

    virtual uint8_t getAddress()   = 0;
    virtual VPIN    getStartVpin() = 0;
    virtual uint8_t getPinCount()  = 0;
};
#endif // NETWORK_COMPONENT_H