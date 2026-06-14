//**************************************************************************************
//Solenoid Coil turnout
//**************************************************************************************
// Define a pulse time of 50ms to activate the coil
#define PULSE 100

// Define a macro called DUALCOILTURNOUT which creates various objects and event handlers for turnouts
// This macro:
// Defines a pin turnout
// Defines an alias
// Sets the direction pin and sends the pulse for the CLOSE command
// Resets the direction pin and sends the pulse for the THROW command
#define DUALCOILTURNOUT(t, p1, p2, desc, ali) \
VIRTUAL_TURNOUT(t,  desc) \
ALIAS(ali, t) \
DONE \
ONCLOSE(t) \
RESET(p2)\
SET(p1) \
DELAY(PULSE)RESET(p1) \
DELAY(1000)RESET(p1) \
DONE \
ONTHROW(t) \
RESET(p1) \
SET(p2)DELAY(PULSE)RESET(p2) \
DELAY(PULSE)RESET(p2) \
DONE

#define TURNOUTBUTTON(throwpin,closepin,turnout,TbuttonAlias,CbuttonAlias)\
ALIAS(CbuttonAlias,closepin) \
ALIAS(TbuttonAlias,throwpin) \
DONE \
AUTOSTART \
START(throwpin) \
START(closepin) \
DONE \
SEQUENCE(throwpin) \
AT(TbuttonAlias) \
IFCLOSED(turnout) \
THROW(turnout) \
ENDIF \
FOLLOW(throwpin) \
DONE \
SEQUENCE(closepin) \
AT(CbuttonAlias) \
IFTHROWN(turnout) \
CLOSE(turnout) \
ENDIF \
FOLLOW(closepin) \
DONE 
/* AFTER(buttonAlias)\
THROW(turnout)\
DONE*/

