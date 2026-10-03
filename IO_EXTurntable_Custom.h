//#include "IO_EXTurntable.cpp"
#include "IODevice.h"
//custom EXTurntable class to handle relative turntable movement

#define TT_CUSTOM_DIAG 2
class EXTurntable_Custom : public IODevice {
public:

  static void create(VPIN firstVpin, int nPins, I2CAddress addr) {
      if (checkNoOverlap(firstVpin, nPins, addr)) new EXTurntable_Custom(firstVpin, nPins, addr);
      
    }

private:
   // Constructor
    EXTurntable_Custom(VPIN firstVpin, int nPins, I2CAddress addr): IODevice(firstVpin, nPins) {
         _firstVpin=firstVpin;
          _nPins=nPins;
          _I2CAddress=addr;
  
          addDevice(this);  
        
    }
   // HAL entry points (these override IODevice's private virtuals)
    void _begin() override;
    void _loop(unsigned long currentMicros) override;
    int  _read(VPIN vpin) override;
    void _writeAnalogue(VPIN vpin, int value, uint8_t activity, uint16_t duration) override;
    void _display() override;

    // Internal state
    VPIN        _firstVpin;
    int         _nPins;
    I2CAddress  _I2CAddress;
    uint8_t     _stepperStatus;

    // custom additions
    long        _lastPosition;     // for relative movement
  };

// Turn = 0,             // Rotate turntable, maintain phase
// Turn_PInvert = 1,     // Rotate turntable, invert phase
// Home = 2,             // Initiate homing
// Calibrate = 3,        // Initiate calibration sequence
// LED_On = 4,           // Turn LED on
// LED_Slow = 5,         // Set LED to a slow blink
// LED_Fast = 6,         // Set LED to a fast blink
// LED_Off = 7,          // Turn LED off
// Acc_On = 8,           // Turn accessory pin on
// Acc_Off = 9           // Turn accessory pin off
// Turn_Relative = 10,    // Rotate turntable relative to current position
// Turn_Relative_PInvert = 11, // Rotate turntable relative to current position
void EXTurntable_Custom::_writeAnalogue(VPIN vpin, int value, uint8_t activity, uint16_t duration)
{
if (activity>=10 && activity<=11)
{
    value+= _lastPosition;
    if(value<0)
    {
        value=0;
    }

activity = (activity==10)?0:1; // convert to Turn or Turn_PInvert

}
 if (_deviceState == DEVSTATE_FAILED) return;
 if(activity<2)
 {
    _lastPosition = value;
 }
 
  uint8_t stepsMSB = value >> 8;
  uint8_t stepsLSB = value & 0xFF;
#if  TT_CUSTOM_DIAG >= 1
  DIAG(F("EX-Turntable WriteAnalogue VPIN:%u Value:%d Activity:%d Duration:%d"),
    vpin, value, activity, duration);
  DIAG(F("I2CManager write I2C Address:%s stepsMSB:%d stepsLSB:%d activity:%d"),
    _I2CAddress.toString(), stepsMSB, stepsLSB, activity);
#endif
  _stepperStatus = 1;     // Tell the device driver Turntable-EX is busy
  I2CManager.write(_I2CAddress, 3, stepsMSB, stepsLSB, activity);
}
void EXTurntable_Custom::_begin(){
  I2CManager.begin();
  if (I2CManager.exists(_I2CAddress)) {
//#ifdef DIAG_IO
    _display();
//#endif
  } else {
    DIAG(F("EX-Turntable_Custom I2C:%s device not found"), _I2CAddress.toString());
    _deviceState = DEVSTATE_FAILED;
  }
}

void EXTurntable_Custom::_display() {
  DIAG(F("EX-Turntable_Custom I2C:%s Configured on Vpins:%u-%u %S"), _I2CAddress.toString(), (int)_firstVpin, 
    (int)_firstVpin+_nPins-1, (_deviceState==DEVSTATE_FAILED) ? F("OFFLINE") : F("Online"));
}

void EXTurntable_Custom::_loop(unsigned long currentMicros) {
  uint8_t readBuffer[1];
  I2CManager.read(_I2CAddress, readBuffer, 1);
  _stepperStatus = readBuffer[0];
 // DIAG(F("Turntable-EX returned status: %d"), _stepperStatus);
  delayUntil(currentMicros + 500000);  // Wait 500ms before checking again, turntables turn slowly
}

int EXTurntable_Custom::_read(VPIN vpin) {
  if (_deviceState == DEVSTATE_FAILED) return 0;
  // DIAG(F("_read status: %d"), _stepperStatus);
  if (_stepperStatus > 1) {
    return false;
  } else {
    return _stepperStatus;
  }
}