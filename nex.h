#pragma once

#include "nexGlobals.h"

// forward deklarations
bool NEX_sendCommand(const char* cmd, bool waitResponse, bool showLog=true); //### showLog kann später weg

#include "page.h"

// called in the setup function
void NEX_begin(uint baudrate) {
  log_i("[NEX] begin...");
  Serial2.begin(baudrate, SERIAL_8N1, NEX_SER_RX, NEX_SER_TX);

  NEX_sendCommand("", false);        // send empty command
  // Restart the nextion, which will run the Program.s part of the HMI file.
  NEX_sendCommand("rest", false);    // Restart the screen
}   // init()


// Converts an little endian integer (4 bytes) from the nextion display to an uint32_t
int32_t hmiIntToInt32(byte arr[], uint8_t start) {
  // numeric value sent in 4 byte 32-bit little endian order
  return int32_t(arr[start+0]) | int32_t(arr[start+1]<<8) | int32_t(arr[start+2]<<16) | int32_t(arr[start+3]<<24);
}


bool NEX_readPayload(uint8_t payload[], uint64_t buffLen, uint64_t timeout) {
  uint8_t buff[1];
  int cntBytes = 0, cntFF = 0;
  uint64_t msStart = millis() + timeout;

  while (millis() <= msStart) {
    if (Serial2.available()) {
      Serial2.readBytes(buff, 1);

      payload[cntBytes] = buff[0];//### check for buffer overflow
      cntBytes++;

      if (cntBytes >= buffLen-1) {
        payload[cntBytes-1] = 0;
        log_w("\nWARN: buffer overflow! max=%llu buf=\"%s\"", buffLen, payload);
        // Nextions response on an overflow error
        payload[0] = 0x24; payload[1] = 0xFF; payload[2] = 0xFF; payload[3] = 0xFF;
        payload[4] = 0x00;
        return false;
      }

      msStart = millis() + 50;    // give serial some time for next value

      if (buff[0] == 0xFF) {
        cntFF++;
        if (cntFF == 3) {  // found the tail of the nextion message
          payload[cntBytes] = '\0';
          return true;
        }   // end of message reached
      }   // read value is FF
      else
        cntFF = 0;
    }   // if Serial2.available()
  }   // while in time

  return false;
}


bool NEX_sendCommand(const char* cmd, bool waitResponse, bool showLog) { //### showLog kann später weg
  if (showLog) log_d("[NEX] Send '%s' --> ", cmd);

  Serial2.print(cmd);
  Serial2.write(0xFF);
  Serial2.write(0xFF);
  Serial2.write(0xFF);

  uint8_t resp[11];
  if (waitResponse && NEX_readPayload(resp, 10, NEX_CmdRespTimeout)) {
    // log_d("%s(%X)", resp[0] == 1 ? "OK" : "failed", resp[0]);
    return resp[0] == 1;
  }

  // log_d("none %s", waitResponse?"received!":"expected");
  return false;
}   // sendCommand()


void assignConfig(char cfgStr[]) {
  log_d("    assign config...");

  #define CFG_SPLIT ";"
  #define VAL_SPLIT "&"

  char *endStr;
  char *tok = strtok_r(cfgStr, CFG_SPLIT, &endStr);
  while (tok != NULL) {
    char *endTok;
    // We know there is at least one value for the specified name
    char *tokNam = strtok_r(tok,  VAL_SPLIT, &endTok);
    char *tokVal = strtok_r(NULL, VAL_SPLIT, &endTok);

    if (tokVal != NULL) { // value specified? If not, use the default, what is allready assigned
      log_d("     '%s' = '%s'", tokNam, (strcmp("WifiPWD", tokNam) != 0) ? tokVal : "*******");

      if (strcmp("standbyDelay", tokNam) == 0) {
        NEX_cfg.standbyDelay = atoi(tokVal);
        //### if (standby inactive) reset standbytime to new value - time
      }
      else
      if (strcmp("longTouchTime", tokNam) == 0) {
        NEX_cfg.longTouchTime = atoi(tokVal);
      } else
      if (strHas(tokNam, "fmtTime")) {
        int idx = atoi(tokNam+7);
        if (idx >= 0 && idx <=3)
          strncpy(NEX_cfg.fmtTime[idx], tokVal, MAX_FMTPARAM_LEN);
        else
          log_w("WARNING: invalid format-makro name fmtTime%d in token '%s'", idx, tok);
      } else
      {
        log_w("WARNING: unhandled setup token '%s'", tok);
      }
    }   // if tokVal

    tok = strtok_r(NULL, CFG_SPLIT, &endStr);
  }   // while tok
}   // assignConfig()


void NEX_handleMsg(uint8_t payload[]) {
  int cntBytes = strlen( (char*)payload );
  char *dta = nullptr;
  // All Nexion return codes: https://nextion.tech/instruction-set/#s7

  // nextion response to a command that was not handled by NEX_SendCommand()
  if (cntBytes==4 && payload[0]>=0x00 && payload[0]<=0x23) {    // response to a command are in range 0 - 0x23
    if (payload[0] != 0x01)   // command responses is 'OK'
      log_d("--> Cmd failed: 0x%X", payload[0]);
    // else    // command responses is 'failed'
    //   log_d("--> Cmd Ok: 0x%X", payload[0]);
    return;
  };

  switch (payload[0]) {

    case 0x00: { // 00 00 00 FF FF FF - Returned when Nextion has started or reset
      log_i("[Nex] Screen started or reseted!");
      break;
    }   // case 0x00


    case 0x88: {  // 88 FF FF FF - Returned when Nextion has powered up and is now initialized successfully
      log_i("[Nex] Screen Ready!");
      break;
    }


    case 0x70: { // 70 [txt data] FF FF FF - Returned when using get command for a string.
      payload[cntBytes-3] = '\0';   // remove message tail
      dta = (char*)payload+1;
      log_d("[Nex] Received string data '%s'!", dta);
      break;
    }   // case 0x70


    case 0x71: {  // 71 [32 bit num value] FF FF FF - Returned when get command to return a number
      int32_t num = hmiIntToInt32(payload, 1);
      log_d("[Nex] Received numeric data %d!", num);
      break;
    }


    case 0x66: {  // 66 pg FF FF FF - Returned when sendme (send page) is set to 1
      uint8_t pg = (uint8_t)payload[1];
      // log_d("[Nex] Received page %d from sendme!", pg);
      break;
    }


    case 0x67: {  // 68 x x y y s FF FF FF - Returned when sendxy (touchevents) is set to 1
      // coordinates are in 16 bit big endian order
      int16_t x = (int16_t)(payload[1]<<8) | (int16_t)payload[2];
      int16_t y = (int16_t)(payload[3]<<8) | (int16_t)payload[4];
      bool     s = payload[5] == 1;
      if (s) {
        PG_touch.pressed(x, y);
      } else {
        PG_touch.released(x, y);
      }
      log_d("[TOUCH] Received %s at %d,%d from sendXY!", s?"pressed":"released", x, y);
      break;
    }


    case NEX_SER_HEAD: {  // 3C Id1 Id2 data1..n FF FF FF
      char msgId[3] = {(char)payload[1], (char)payload[2], 0};

      if (strcmp("SR", msgId) == 0) {   // screen init - called in 'Program.s' of HMI-file
        payload[cntBytes-3] = '\0';     // remove message tail
        dta = (char*)payload + 3;       // skip header
        log_i("[NEX] screen init.");

        assignConfig(dta);
      } else


      if (strcmp("PI", msgId) == 0) {   // page init - called on page preinitialize
        payload[cntBytes-3] = '\0';     // remove message tail
        dta = (char*)payload + 3;       // skip header
        Page_init(dta);
      } else


      if (strcmp("PE", msgId) == 0) {   // page enter - called on page postinitialize
        // nothing to do
        // log_d("[NEX] msg: enter page");
      } else


      if (strcmp("PL", msgId) == 0) {   // page leave - called on page exit
        Page_exit();
      } else


      if (strcmp("CM", msgId) == 0) {   // custom message - called by buttons of a page
        switch ((char)payload[3]) {    // id of that message
          case 'B': {
            wifiMgr.reboot();
            delay(200);
            break;
          }
          // $$$ here can be implemented more with other ids!
          default:
            log_w("[NEX] Unhandled custom message '%s'", payload);
        }
      } else


      if (strcmp("TT", msgId) == 0) {   //### Temporary tests - used on page `pgTest1`
        switch ((char)payload[3]) {    // id of that message
          case '1':   { NEX_sendCommand("t0.txt=\"I'am ESP\"", true);  break; }
          case '2':   { NEX_sendCommand("t0.txt=\"Ich bin's\"", true);  break; }
          case '3':   {
            if (WiFi.status() == WL_CONNECTED) {
              log_d("[WIFI] Disconnect...");
              wifiMgr.disconnect();
            }
            else {
              log_d("[WIFI] Reconnect...");
              // WiFi.mode(WIFI_off);
              WiFi.reconnect();
            }
            break; }
          case 'W':   {
            if (WiFi.status() != WL_CONNECTED) {  // WifFi not connected
              NEX_sendCommand("t0.txt=\"AP for Config started...\"", true);
              // starting an AP to reconfigur the connection settings
              wifiMgr.setConfigPortalBlocking(false);
              wifiMgr.startConfigPortal();
            }
            else {
              // starting an webserver on an existing WiFi connection to reconfigur the connection settings
              NEX_sendCommand("t0.txt=\"Open ESP ip to config...\"", true);
              wifiMgr.startWebPortal();
            }

            break;
          }
          default:
            log_d("[NEX]### Unhandled temporary tests '%s'", payload);
        }
      } else
      {}

      break;
    }   // case NEX_SER_HEAD

    default: {
      log_w("[NEX] Unknown cmd '0x%X:\n\t", payload[0]);
      for (byte i = 0; i < cntBytes; i++) {
        Serial.print(" ");
        Serial.print(payload[i], HEX);
      }
      Serial.println("'");
    }
  }   // switch payload[0]
}   // NEX_handleMsg()


int NEX_getVar(char* varname) {
  size_t len = NEX_MAX_TEXTLEN+4;
  char buff[len+1] = {0};

  sprintf(buff, "get %s", varname);
  NEX_sendCommand(buff, false, false);

  memset(buff, 0, len);
  if (NEX_readPayload((uint8_t*)buff, len, 100)) {
    uint8_t typeId = strHas(varname, ".txt") ? 0x70 : 0x71;
    if (buff[0] == typeId) {  // returned data off var

    } else
      NEX_handleMsg((uint8_t*)buff);
  }
}   // NEX_getInt()
