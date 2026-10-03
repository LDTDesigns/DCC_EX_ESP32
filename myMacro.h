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
VIRTUAL_TURNOUT(t,desc) \
    ALIAS(ali,t) \
    DONE \
    ONCLOSE(t) \
    RESET(p2) \
    SET(p1) \
    DONE \
    ONTHROW(t) \
    RESET(p1) \
    SET(p2) \
    DONE






/* // VIRTUAL_TURNOUT(t,  desc) \
// ALIAS(ali, t) \
// DONE \
// ONCLOSE(t) \
//     RESET(p2)\
//     SET(p1) \
//     DELAY(PULSE) \
//     RESET(p1) \
//     DONE \
// ONTHROW(t) \
//     RESET(p1) \
//     SET(p2) \
//     DELAY(PULSE) \
//     RESET(p2) \
//     DONE */









#define TURNOUTBUTTON(throwbtn,closebtn,turnout,TbuttonAlias,CbuttonAlias)\
ALIAS(CbuttonAlias,closebtn) \
ALIAS(TbuttonAlias,throwbtn) \
DONE \
SEQUENCE(throwbtn) \
    AT(TbuttonAlias) \
    IFCLOSED(turnout) \
        THROW(turnout) \
    ENDIF \
    FOLLOW(throwbtn) \
SEQUENCE(closebtn) \
    AT(CbuttonAlias) \
    IFTHROWN(turnout) \
        CLOSE(turnout) \
    ENDIF \
    FOLLOW(closebtn)\
    AUTOSTART \
START(throwbtn) \
START(closebtn) \
DONE
/* AFTER(buttonAlias)\
THROW(turnout)\
DONE*/

