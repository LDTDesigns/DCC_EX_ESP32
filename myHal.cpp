//  Sample myHal.cpp file.
//
// To use this file, copy it to myHal.cpp and uncomment the directives and/or
// edit them to satisfy your requirements.  If you only want to use up to 
// two MCP23017 GPIO Expander modules and/or up to two PCA9685 Servo modules,
// then you don't need this file as DCC++EX configures these for free!

// Note that if the file has a .cpp extension it WILL be compiled into the build
// and the halSetup() function WILL be invoked.
//
// To prevent this, temporarily rename the file to myHal.txt or similar.
//

// The #if directive prevent compile errors for Uno and Nano by excluding the 
//  HAL directives from the build.


#if !defined(IO_NO_HAL)

// Include devices you need.
#include "IODevice.h"


//#include "IO_HALDisplay.h"  // Auxiliary display devices (LCD/OLED)
//#include "IO_HCSR04.h"    // Ultrasonic range sensor
//#include "IO_VL53L0X.h"   // Laser time-of-flight sensor
//#include "IO_DFPlayer.h"  // MP3 sound player
//#include "IO_TouchKeypad.h  // Touch keypad with 16 keys
#include "RS485_BusController.h"
#include "IO_RS485_Node.h" // custom rs485 node handler
#include"RS485_IODevice.h"
//#include "RemoteDevice.h"  // custom remote device handler
#include"IO_LocalCoilDriver.h"
#include"IO_RemoteCoilDriver.h"
#if !nanoLite
#include "IO_EXTurntable.h"   // Turntable-EX turntable controller
#endif
//#include "IO_EXFastClock.h"  // FastClock driver
//#include "IO_PCA9555.h"     // 16-bit I/O expander (NXP & Texas Instruments).

//==========================================================================
// The function halSetup() is invoked from CS if it exists within the build.
// The setup calls are included between the open and close braces "{ ... }".
// Comments (lines preceded by "//") are optional.
//==========================================================================

void halSetup() {



  PCF8574::create(240, 8, 0x39);

 //LocalCoilDriver::create(0x38,200,8,3000,75);

 
// ========================================================================
    // SECTION 1: NETWORK HARDWARE BUS INITIALIZATION
    // ========================================================================
    // Injecting our hardware dependencies at the top level makes it trivial to 
    // reconfigure our physical wiring down the road without rewriting the driver.

    // Select which of the Mega's 4 independent hardware serial ports to use.
    // (Serial2 uses Pin 16 for TX and Pin 17 for RX. This completely leaves 
    // Serial1 free to handle Wi-Fi throttles and JMRI traffic safely!)
    HardwareSerial& rs485Bus = Serial3; 
#define RS485_TX_ENABLE_PIN   2
    // Define the Digital Output Pin on the Mega that is physically wired to 
    // the MAX485's RE/DE shorted jumper pins to control transmit/receive direction.
    const uint8_t max485TogglePin = 2; 

    // Spin up the physical UART hardware bus lines at 115,200 bits per second.
    // This high speed ensures a standard text packet clears the line in < 1ms.
  //  rs485Bus.begin(19200); 

RS485BusController* Bus1 =RS485BusController::create(rs485Bus,max485TogglePin,50);

 // 1. Declare the remote network hub (Node ID = 1)
 RS485_Node* node1  =RS485_Node::create(1,Bus1);
 RS485_Node* node2 =RS485_Node::create(2,Bus1);
//RemoteCoilDriver::create(node1,0x38,200,8,3000,75);
RS485_IODevice::create(node1,0x20,232,8);
RemoteCoilDriver::create(node1,0x20,200,8,3000,75);
Bus1->init();
}

#endif
