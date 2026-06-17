#ifndef RS485_NODE_H
#define RS485_NODE_H
#pragma once

#include "IODevice.h"
#include "NetworkComponent.h"
#ifndef DIAG_H_INCLUDED
#define DIAG_H_INCLUDED
#include "DIAG.h"
#endif


#define MAX_COMPONENTS_PER_NODE 8

class RemoteDevice;

class RS485_Node : public IODevice {
public:
    // AUTO mode (no DE/RE pin, auto-direction module)
    RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort);

    // MANUAL mode (DE/RE pin controlled by MCU)
    RS485_Node(uint8_t nodeAddress, HardwareSerial& serialPort, uint8_t txPin);

    void registerComponent(RemoteDevice* comp);

    uint8_t getNodeAddress() const { return _nodeAddress; }

    // IODevice interface
    int  _read(VPIN vpin) override;
    void _write(VPIN vpin, int value) override;
    void _loop(unsigned long now) override;
    void _display() override;

private:
    uint8_t         _nodeAddress;
    HardwareSerial* _serial;
    uint8_t         _txPin;
    bool            _autoMode;

    RemoteDevice* _registry[MAX_COMPONENTS_PER_NODE];
    int           _count;
};


#endif




