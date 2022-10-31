#pragma once

#include <time.h> // for struct tm


// format description: https://help.gnome.org/users/gthumb/unstable/gthumb-date-formats.html.de
// or https://github.com/samsonjs/strftime#supported-specifiers

#define MAXLEN_COMPONENT_LIST   2000    // max. length auf the string comming from the page init event of the Nextion page
#define NEX_MAX_NAMELEN         20      // max. length of HMI objectnames
#define NEX_MAX_TEXTLEN         100     // max. length of text for HMI text elements
#define NEX_CmdRespTimeout      100     // timeout for waiting of a response to a command
#define NEX_SER_RX              16      // RX pin for connecting to the TX pin of the Nextion
#define NEX_SER_TX              17      // TX pin for connecting to the RX pin of the Nextion

/*
  Self defined Return Codes
*/
  // Testing messages from Nextion
#define NEX_TEST_MSG      0x24   // 0x24 id 0xFF 0xFF 0xFF
  // Messages from Nextion to this program
#define NEX_SER_HEAD      0x3C   // 0x3C  This is used in the HMI file. Do not change!

#define MAX_FMTPARAM_LEN  30      // max length of default format for _time.txt

// Values collected in 'screen init' message that is fired from Program.s code of the HMI-file
struct NEX_cfg_t {//### other name
  int32_t standbyDelay;
  int32_t longTouchTime;
  char    fmtTime[4][MAX_FMTPARAM_LEN];    // up to 4 format macros can be configured from 'screen init event'
}
NEX_cfg = { 60, 600, {"%H:%M", "%d.%m.%Y", "%I:%M %p", "%a %d. %b. %y"} };    // set the default values
