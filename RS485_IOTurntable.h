
#include "IO_EXTurntable.h"
#include"RS485_BusController.h"
class RS485_Node;


class RS485_IOTurntable:public EXTurntable {
  public:
    RS485_IOTurntable(RS485_Node* owner, VPIN firstVpin, int nPins, I2CAddress I2CAddress) : EXTurntable(firstVpin, nPins, I2CAddress) {
        _owner = owner;
_owner->addDevice(this,I2CAddress,firstVpin,nPins,"RS485_IOTurntable",IODevice::CONFIGURE_INPUT); //define as input so polling is done to keep the stepper busy status accurate
_display();
    }
   static void create(RS485_Node* owner, I2CAddress i2c, VPIN firstVpin, int nPins) {
        if (checkNoOverlap(firstVpin, nPins)) {
            new RS485_IOTurntable(owner,firstVpin, nPins, i2c);
        }
    }
   

void _loop(unsigned long currentMicros)  override{
   if (_owner)
        {
            uint16_t mask = _owner->getCurrentMask(this);

            if (mask != _lastProcessedMask)
            {
              //  _ProcessMask(mask);
                    _lastProcessedMask = mask;
                    _stepperStatus = mask; // Update the stepper status based on the mask
            }
        }

    //  uint8_t readBuffer[1];
  //I2CManager.read(_I2CAddress, readBuffer, 1);
 // _stepperStatus = readBuffer[0];
  // DIAG(F("Turntable-EX returned status: %d"), _stepperStatus);
  delayUntil(currentMicros + 500000);  // Wait 500ms before checking again, turntables turn slowly
}
void _begin() override{}
void _display() override{

    DIAG(F("RS485_IOTurntable: Created at I2C address %s with first VPIN %d and %d pins on node %u"), _I2CAddress.toString(), _firstVpin, _nPins, _owner->getNodeAddress());
    }   


// writeAnalogue to send the steps and activity to Turntable-EX.
// Sends 3 bytes containing the MSB and LSB of the step count, and activity.
// value contains the steps, bit shifted to MSB + LSB.
// activity contains the activity flag as per this list:
// 
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
// Relative_Move = 10,    // Move relative to current position
void _writeAnalogue(VPIN vpin, int value, uint8_t activity, uint16_t duration) override {
        // Send the command to the turntable via RS485
        // This is a placeholder for the actual implementation
        DIAG(F("RS485_IOTurntable: Sending command to turntable at I2C address %s with activity %d and duration %d on node %u"), _I2CAddress.toString(), activity, duration, _owner->getNodeAddress());
        // Here you would implement the actual RS485 communication to send the command to the turntable.
        if(activity==Turn_Relative){
value=_lastTargetPosition+value;
activity=Turn;
        }
else if (activity==Turn_Relative_PInvert)
{
value=_lastTargetPosition+value;
    activity=Turn_PInvert;
        }
    if (value<0)
    {
        value=0;
    }

    _lastTargetPosition=value;  
            // For relative moves, we might want to encode the value differently
            if(_owner){
                _owner->sendAnalogueMask(_I2CAddress,vpin,value,activity,duration,RS485BusController::DEVICE_TYPE_TURNTABLE);
                _stepperStatus = 1;  // Tell the device driver Turntable-EX is busy
            }
}
    
  int  _read(VPIN vpin) override {
        // Read the status from the turntable via RS485
#ifdef DIAG_IO
        DIAG(F("RS485_IOTurntable: Reading status from turntable at I2C address %s on node %u"), _I2CAddress.toString(), _owner->getNodeAddress());
        #endif
        // Here you would implement the actual RS485 communication to read the status from the turntable.
       if(_stepperStatus>1){
        return false;
       }else{   
        return _stepperStatus; // Return the last known status
    }
}
protected:
    RS485_Node* _owner=nullptr;
    RS485BusController* _bus;
   //uint8_t _stepperStatus=0;
   uint16_t _lastProcessedMask=0;
   uint16_t _lastTargetPosition=0;


};
