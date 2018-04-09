
//  TODO:  Document how this integrates with release process
// ------------------------------------------------------------------------------------------------------------
//      Build Type  'BUILD_TYPE'
//        Mandatory: must be set to one of these
// ------------------------------------------------------------------------------------------------------------
//          #define   BUILD_RELEASE   1   // release version for production
//          #define   BUILD_DVT       2   // design verification test
//          #define   BUILD_DEV       3   // working development version

    // --------------------
    // FW0058.X  12LPC15
    // --------------------
    // mandatory
    #define BUILD_DATE       "2018-04-02"
    #define BUILD_TYPE       BUILD_RELEASE
    #define MODEL_NO_STRING  "12LPC15"
    #define FW_ID_STRING     "FW0058.1"
    #define RELEASE_DATE     "2018-04-02"
    #define OPTION_DEV       DEV_INVERTER
    #define OPTION_PCB       PCB_LPC
    #define OPTION_PCB_REV   PCB_REV_LPC_1
    #define OPTION_DCIN      DCIN_12V
    #define OPTION_CAL       CAL_12LPC
    // optional
    #define OPTION_HAS_CHARGER  1
    #define OPTION_CHARGE_3STEP 1
