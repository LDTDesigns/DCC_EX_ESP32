/**
 * @file VirtualRegister.h
 * @brief Standalone Virtual Memory Pin Cache for DCC-EX RS485 Nodes
 */

#ifndef VIRTUAL_REGISTER_H
#define VIRTUAL_REGISTER_H

#include <Arduino.h>

class VirtualRegister {
private:
    uint16_t _startPin;
    uint8_t _pinCount;
    uint8_t* _pinStates; // Dynamic RAM array to hold individual bit states

public:
    /**
     * @brief Constructor to initialize a block of virtual memory pins
     * @param startPin The first VPIN in this contiguous block (e.g., 232)
     * @param pinCount The number of sequential pins to manage (e.g., 8)
     */
    VirtualRegister(uint16_t startPin, uint8_t pinCount) {
        _startPin = startPin;
        _pinCount = pinCount;
        
        // Allocate exactly enough memory bytes to store the states
        _pinStates = new uint8_t[_pinCount];
        
        // Default all pins to 0 (LOW / Off) on layout boot
        for (uint8_t i = 0; i < _pinCount; i++) {
            _pinStates[i] = 0;
        }
    }

    // Destructor to safely release RAM if the object is ever destroyed
    ~VirtualRegister() {
        delete[] _pinStates;
    }

    // Accessor methods to verify layout parameters
    uint16_t getStartPin() const { return _startPin; }
    uint8_t getPinCount() const { return _pinCount; }

    /**
     * @brief Writes a state (0 or 1) to a specific virtual pin
     */
    void _write(uint16_t vpin, int state) {
        if (vpin >= _startPin && vpin < (_startPin + _pinCount)) {
            uint8_t offset = vpin - _startPin;
            _pinStates[offset] = (state == 0) ? 0 : 1;
        }
    }

    /**
     * @brief Reads the current cached state of a specific virtual pin
     * @return 1 if HIGH, 0 if LOW, or 0 if out of valid bounds
     */
    int _read(uint16_t vpin) {
        if (vpin >= _startPin && vpin < (_startPin + _pinCount)) {
            uint8_t offset = vpin - _startPin;
            return _pinStates[offset];
        }
        return 0; 
    }
};

#endif // VIRTUAL_REGISTER_H