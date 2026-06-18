#pragma once
#include "IODevice.h"
#include "IO_RS485_Node.h"

class RS485_Poller : public IODevice {
public:
    RS485_Poller(RS485_Node* node, int intervalMs)
    : IODevice(0, 0), _node(node), _interval(intervalMs) {
        addDevice(this);
    }

protected:
    void _begin() override {
        DIAG(F("[RS485] Poller running every %dms"), _interval);
    }

    void _loop(unsigned long now) override {
        if (_node)
            _node->poll(now);

        delayUntil(now + _interval * 1000UL);
    }

    void _display() override {
        DIAG(F("[RS485] Poller interval %dms"), _interval);
    }

void _write(VPIN vpin, int value) override {
        // Do nothing
    }


private:
    RS485_Node* _node;
    int _interval;
};
