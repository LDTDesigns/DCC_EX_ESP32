

//use this to define each turnout
DUALCOILTURNOUT(105, 200, 201, "Yard entrance", YD_E)  // Define the "Yard entrance" turnout with turnout ID 105 using PCF8754 pin 200,201, and create alias YD_E

//===========================================================================
//use this to define each turnout
DUALCOILTURNOUT(106, 202, 203, "Yard exit", YD_EX)  // Define the "Yard exit" turnout with turnout ID 106 using PCF8754, and create alias YD_E
//============================================================================
//use this to define each turnout
DUALCOILTURNOUT(107, 204, 205, "BranchLine1", BR_LN1)  // Define the "Yard exit" turnout with turnout ID 106 using PCF8754, and create alias YD_E
//=====================================================
DUALCOILTURNOUT(108, 206, 207, "BranchLine2", BR_LN2)  // Define the "Yard exit" turnout with turnout ID 106 using PCF8754, and create alias YD_E
//====================================================
// //Next PCF MODULE
// DUALCOILTURNOUT(109, 208, 209, "BranchLine3", BR_LN3)
// //====================================================
// DUALCOILTURNOUT(110, 210, 211, "BranchLine4", BR_LN4)
// //====================================================
// DUALCOILTURNOUT(111, 212, 213, "BranchLine5", BR_LN5)
// //====================================================
// DUALCOILTURNOUT(112, 214, 215, "BranchLine6", BR_LN6)
// //====================================================
// //Next PCF MODULE
// DUALCOILTURNOUT(113, 216, 217, "BranchLine7", BR_LN7)
// //====================================================
// DUALCOILTURNOUT(114, 218, 219, "BranchLine8", BR_LN8)
// //====================================================
// DUALCOILTURNOUT(115, 220, 221, "BranchLine9", BR_LN9)
// //====================================================
// DUALCOILTURNOUT(116, 222, 223, "BranchLine10", BR_LN10)
// //====================================================
//Next PCF MODULE

//====================
//Pins 224-231 on 0x22 are configured as input in myInputDevices.h
 //TURNOUTBUTTON(throwpin,closepin,BTN1,BTN2)
TURNOUTBUTTON(232,233,YD_E,BTN1,BTN2)
TURNOUTBUTTON(234,235,YD_EX,BTN3,BTN4)
TURNOUTBUTTON(236,237,BR_LN1,BTN5,BTN6)
TURNOUTBUTTON(238,239,BR_LN2,BTN7,BTN8)