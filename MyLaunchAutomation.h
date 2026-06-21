// SEQUENCE(99)
//    SIGNAL(25,26,27)
//    RED(25)   // indicate launch not ready
//    AFTER(30) // user presses and releases launch button
//    UNJOIN    // separate the programming track from main
//    DELAY(2000)
//    AMBER(25) // Show amber, user may place loco
//    AFTER(30) // user places loco on track and presses “launch” again
//    READ_LOCO // identify the loco
//    GREEN(25) // show green light to user
//    JOIN      // connect prog track to main
//    START(12) // send loco off along route 12
//    FOLLOW(99) // keep doing this for another launch

//    AUTOMATION(11,"LAUNCH")
//    FOLLOW(99)
//    DONE