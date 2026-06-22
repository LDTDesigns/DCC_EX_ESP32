/**************************************************************************************************
 * This is an example automation file to control EX-Turntable using recommended techniques.
 ************************************************************************************************** 
 * INSTRUCTIONS
 ************************************************************************************************** 
 * To use this example file as the starting point for your layout, there are two options:
 * 
 * 1. If you don't have an existing "myAutomation.h" file, simply rename "myEX-Turntable.example.h" to
 *    "myAutomation.h".
 * 2. If you have an existing "myAutomation.h" file, rename "myEX-Turntable.example.h" to "myEX-Turntable.h",
 *    and then include it by adding the line below at the end of your existing "myAutomation.h", on a
 *    line of its own:
 * 
 *    #include "myEX-Turntable.h"
 * 
 * Note that there are further instructions in the documentation at https://dcc-ex.com/.
 *************************************************************************************************/

/**************************************************************************************************
 * The MOVETT() command below will automatically move your turntable to the defined step position on
 * start up.
 * 
 * If you do not wish this to occur, simply comment the line out.
 * 
 * NOTE: If you are including this file at the end of an existing "myAutomation.h" file, you will likely
 * need to move this line to the beginning of your existing "myAutomation.h" file in order for it to
 * be effective.
 *************************************************************************************************/
//#include "MyLaunchAutomation.h"

AUTOSTART
MOVETT(600, 114, Turn)
// START(1)
// START(33)
// START(34)
// START(35)
// START(36)
DONE

// For Conductor level users who wish to just use EX-Turntable, you don't need to understand this
// and can move to defining the turntable positions below. You must, however, ensure this remains
// before any position definitions or you will get compile errors when uploading.
//
// Definition of the EX_TURNTABLE macro to correctly create the ROUTEs required for each position.
// This includes RESERVE()/FREE() to protect any automation activities.
//
#define EX_TURNTABLE(route_id, reserve_id, vpin, steps, activity, desc) \
  ROUTE(route_id, desc) \
    RESERVE(reserve_id) \
    MOVETT(vpin,steps,LED_Fast) \
    MOVETT(vpin, steps, activity) \
    WAITFOR(vpin) \
    FREE(reserve_id) \
    MOVETT(vpin,steps,LED_Off) \
    DONE

/**************************************************************************************************
 * TURNTABLE POSITION DEFINITIONS
 *************************************************************************************************/
// EX_TURNTABLE(route_id, reserve_id, vpin, steps, activity, desc)
//
// route_id = A unique number for each defined route, the route is what appears in throttles
// reserve_id = A unique reservation number (0 - 255) to ensure nothing interferes with automation
// vpin = The Vpin defined for the Turntable-EX device driver, default is 600
// steps = The target step position
// activity = The activity performed for this ROUTE (Note do not enclose in quotes "")
// desc = Description that will appear in throttles (Must use quotes "")
//
EX_TURNTABLE(TTRoute1, Turntable, 600, 114, Turn, "TurnTable Position 1")
EX_TURNTABLE(TTRoute2, Turntable, 600, 227, Turn, "Turntable Position 2")
EX_TURNTABLE(TTRoute3, Turntable, 600, 341, Turn, "TurnTable Position 3")
EX_TURNTABLE(TTRoute4, Turntable, 600, 2159, Turn, "Turntable Position 4")
EX_TURNTABLE(TTRoute5, Turntable, 600, 2273, Turn, "Turntable Position 5")
EX_TURNTABLE(TTRoute6, Turntable, 600, 2386, Turn, "Turntable Position 6")
EX_TURNTABLE(TTRoute7, Turntable, 600, 0, Home, "Home Turntable")

// Pre-defined aliases to ensure unique IDs are used.
// Turntable reserve ID, valid is 0 - 255
ALIAS(Turntable, 255)

// Turntable ROUTE ID reservations, using <? TTRouteX> for uniqueness:
ALIAS(TTRoute1)
ALIAS(TTRoute2)
ALIAS(TTRoute3)
ALIAS(TTRoute4)
ALIAS(TTRoute5)
ALIAS(TTRoute6)
ALIAS(TTRoute7)
ALIAS(TTRoute8)
ALIAS(TTRoute9)
ALIAS(TTRoute10)
ALIAS(TTRoute11)
ALIAS(TTRoute12)
ALIAS(TTRoute13)
ALIAS(TTRoute14)
ALIAS(TTRoute15)
ALIAS(TTRoute16)
ALIAS(TTRoute17)
ALIAS(TTRoute18)
ALIAS(TTRoute19)
ALIAS(TTRoute20)
ALIAS(TTRoute21)
ALIAS(TTRoute22)
ALIAS(TTRoute23)
ALIAS(TTRoute24)
ALIAS(TTRoute25)
ALIAS(TTRoute26)
ALIAS(TTRoute27)
ALIAS(TTRoute28)
ALIAS(TTRoute29)
ALIAS(TTRoute30)

//*****************************************************************************
//add 2 buttons
//**********************************************






AUTOMATION(500, "Districts A MAIN _ B PROG Default")// Reset Default back to DCC Main & PROG
 SET_TRACK(A,MAIN) PRINT("Default Districts Tracks MAIN A & PROG B")
 SET_TRACK(B,PROG)
 DONE
AUTOMATION(501, "District A MAIN")   // Alternate DCC Main track A
 SET_TRACK(A, MAIN) PRINT("District A MAIN")
 DONE
AUTOMATION(502, "District A PROG")   // Alternate DCC PROG track A
 SET_TRACK(A, PROG) PRINT("District A PROG")
 DONE
AUTOMATION(503, "District A DC (Loco Id=1)")     // Alternate DC track A with loco ID 1
 SETLOCO(1)
 SET_TRACK(A,DC) PRINT("District A DC (Loco Id=1)")
 DONE
AUTOMATION(504, "District A DCX (Loco Id=1)")    // Alternate DCX track A Changed to Opposite Polarity
 SETLOCO(1)
 SET_TRACK(A,DCX) PRINT("District A DCX Opposite Polarity") // Track A Opposite Polarity DC
 DONE
AUTOMATION(505, "District A NONE")    // A Track disabled
 SET_TRACK(A, NONE) PRINT ("District A disabled")
 DONE

 AUTOMATION(506, "District B MAIN")   // Alternate DCC Main track B
 SET_TRACK(B, MAIN) PRINT("District B MAIN")
 DONE
AUTOMATION(507, "District B PROG")   // Alternate DCC PROG track B
 SET_TRACK(B, PROG) PRINT("District B PROG")
 DONE
AUTOMATION(508, "District B DC (LocoId=2)")     // Alternate DC track B with loco ID 2
 SETLOCO(2)
 SET_TRACK(A,DC) PRINT("District B DC (Loco Id=2)")
 DONE

AUTOMATION(509, "District B DCX (Loco Id=2)")    // Alternate DCX track B Changed to Opposite Polarity
 SETLOCO(2)
 SET_TRACK(B,DCX) PRINT("District B DCX Opposite Polarity") // Track B Opposite Polarity DC
 DONE

 AUTOMATION(510, "District B NONE")    // B Track disabled
 SET_TRACK(B, NONE) PRINT ("District B disabled")
 DONE


ALIAS(BUTTON1,52)
ALIAS(BUTTON2,53)

ALIAS (SENSOR_1,54)
SIGNAL(22,26,27)

//add sensor to expansion module 0x23 at pin 200
//ALIAS(MAIN_SENSOR,206)
//ALIAS(SIDE_SENSOR,207)

 //add signal
 //SIGNALH(200,201,202) 
 // define signal on pin 22(red)
 //ALIAS(MAIN_SIGNAL,200)
 //SIGNALH(203,204,205) 
 // define signal on pin 25(red)
 //ALIAS(SIDE_SIGNAL,203)

//add servos
//SERVO(vpin, position, profile)
//SERVO2(vpin, position, duration)
//SERVO_SIGNAL(vpin, redpos, amberpos, greenpos)
//SERVO_SIGNAL(100, 105, 120, 140)
 //SERVO2(100,105,2000)

 //AUTOMATION(1,"Traffic light 1")

//START(21)
//START(22)
// START(33)
// START(34)
//DONE





//  SEQUENCE(11)
//  AMBER(MAIN_SIGNAL)
// DELAY(500)
// GREEN(MAIN_SIGNAL)
// GREEN(100)
// DELAY(5000)
// AMBER(MAIN_SIGNAL)
// AMBER(100)
// DELAY(1000)
// RED(MAIN_SIGNAL)
// RED(100)
// DELAY(1000)
// RETURN
 

// SEQUENCE(12)
//  AMBER(SIDE_SIGNAL)
// DELAY(500)
// GREEN(SIDE_SIGNAL)
// DELAY(5000)
// AMBER(SIDE_SIGNAL)
// DELAY(1000)
// RED(SIDE_SIGNAL)
// DELAY(1000)
// RETURN



// SEQUENCE(21)
// AT(MAIN_SENSOR)
// RESERVE(5)
// CALL(11)
// FREE(5)
// FOLLOW(21)
// DONE

// SEQUENCE(22)
// AT(SIDE_SENSOR)
// RESERVE(5)
// CALL(12)
// FREE(5)
// FOLLOW(22)
// DONE

// SEQUENCE(33)
// IFCLOSED(YD_E)
// AT(BUTTON1)
// THROW(YD_E)
// ENDIF
// FOLLOW(33)


// SEQUENCE(34)
// IFTHROWN(YD_E)
// AT(BUTTON2)
// CLOSE(YD_E)
// ENDIF
// FOLLOW(34)

// SEQUENCE(35)
// IF(SENSOR_1)
// IFGREEN(22)
// RED(22)
// ENDIF
// ELSE
// IFRED(22)
// GREEN(22)
// ENDIF
// ENDIF
// FOLLOW(35)
#include "myMacro.h"
#include"myTurnoutDefinitions.h"
#include"myInputDevices.h"
#include"myTurnOutToggleButtonAutomation.h"
#include "MyLaunchAutomation.h"