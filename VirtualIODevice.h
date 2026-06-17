#ifndef VIRTUAL_IODEVICE_H
#define VIRTUAL_IODEVICE_H

#include "IODevice.h"
#include "VirtualRegister.h"

class VirtualIODevice : public IODevice {
private:
    VirtualRegister* _reg;

public:
    VirtualIODevice(VPIN start, int count, VirtualRegister* reg)
        : IODevice(start, count), _reg(reg)
    {
        addDevice(this);
    }

    void _display() override {
        DIAG(F("RS485 Device VPINs:%u-%u"),
             (unsigned)_firstVpin,
             (unsigned)(_firstVpin + _nPins - 1));
    }

    int _read(VPIN vpin) override {
        return _reg->_read(vpin);
    }

    void _write(VPIN vpin, int state) override {
        _reg->_write(vpin, state);
    }
};

#endif

