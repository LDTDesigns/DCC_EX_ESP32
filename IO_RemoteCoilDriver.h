#include "IO_BaseCoilDriver.h"
#define REMOTE_COIL_DIAG 1
class RemoteCoilDriver : public BaseCoilDriver {
public:


    RemoteCoilDriver(RS485_Node* owner,I2CAddress i2c, uint8_t firstVpin, uint8_t nPins,unsigned long chargeMs,unsigned long pulse)
        : BaseCoilDriver(i2c, firstVpin, nPins,chargeMs,pulse) 
    {

        _owner = owner;
      //  DIAG(F("[RemoteCoilDriver] CONSTRUCTOR CALLED"));
        addDevice(this);
         owner->addDevice(this,i2c,firstVpin,nPins,"RemoteCoilDriver",IODevice::CONFIGURE_OUTPUT);// register in node
       // _begin();
    
    }

static void create(RS485_Node* node, I2CAddress i2c,VPIN startPin,uint8_t nPins,unsigned long chargeMs,unsigned long pulse ){
    if (checkNoOverlap(startPin, nPins))  new RemoteCoilDriver(node, i2c,startPin,nPins,chargeMs,pulse);
}

    void outputToDevice( uint8_t value) override {
//this is the call that should pass the value to the node bus to send to the remote device
#if REMOTE_COIL_DIAG >=3
DIAG(F("[RemoteCoilDriver] outputToDevice value=%d"), value);
#endif

    //  uint8_t mask = value ? (1 << pin) : 0x00;
      _owner->sendMask(_i2c, value); // send mask to node
    }

    void _begin() override {
    //   I2CManager.begin();
// register with node
// this has been called by iodevice 
#if REMOTE_COIL_DIAG >=3
DIAG(F("[RemoteCoilDriver::_begin] exists() returned: %S"),
    I2CManager.exists(_i2c) ? F("YES") : F("NO"));
     #endif
// 
uint16_t initialMask = 0x0000;


DIAG(F("[RemoteCoilDriver] Broadcast initial mask 0x%u to node %u device %s"),
     (unsigned int)initialMask,
     (unsigned int)_owner->getNodeAddress(),
     _i2c.toString());

_owner->sendMask(_i2c, initialMask); // send initial mask to node

    // All coils OFF (active-low)
   //I2CManager.write(_i2c,1, 0x00);
// send to node
    // Mark capacitor ready at startup
    _ready = true;
    _nextReadyTime = 0;
    _display();
}

void _display() override{
        DIAG(F("[RemoteCoilDriver] I2C:%s VPINs %u-%u : Charge %lu ms : Pulse %lu ms %S"),
         _i2c.toString(),
         _firstVpin,
         _firstVpin + _nPins - 1,
         _chargeTime / 1000UL,
         _pulseDuration / 1000UL,
         (_deviceState == DEVSTATE_FAILED) ? F("OFFLINE") : F(""));
}
protected:
    RS485_Node* _owner;
    RS485BusController* _bus;
};
