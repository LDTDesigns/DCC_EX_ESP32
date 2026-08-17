#include "IO_EXTurntable.h"
//custom EXTurntable class to handle relative turntable movement
class EXTurntable_Custom : public EXTurntable {
    private:
unsigned long _lastPostion = 0;
public:
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
void _writeAnalogue(VPIN vpin, int value, uint8_t activity, uint16_t duration)
{
if (activity>=10 && activity<=11)
{
    value+= _lastPostion;
    if(value<0)
    {
        value=0;
    }
_lastPostion = value;
activity = (activity==10)?0:1; // convert to Turn or Turn_PInvert

}
  if (_deviceState == DEVSTATE_FAILED) return;
  uint8_t stepsMSB = value >> 8;
  uint8_t stepsLSB = value & 0xFF;
#ifdef DIAG_IO
  DIAG(F("EX-Turntable WriteAnalogue VPIN:%u Value:%d Activity:%d Duration:%d"),
    vpin, value, activity, duration);
  DIAG(F("I2CManager write I2C Address:%d stepsMSB:%d stepsLSB:%d activity:%d"),
    _I2CAddress.toString(), stepsMSB, stepsLSB, activity);
#endif
  _stepperStatus = 1;     // Tell the device driver Turntable-EX is busy
  I2CManager.write(_I2CAddress, 3, stepsMSB, stepsLSB, activity);
}



};