struct comp_t {    // one struct for every token in the compStr
  char* namPtr      = nullptr;  // pointer to the objname in the compStr
  char* paramPtr    = nullptr;  // pointer to the parameter of the objname
  union {
    bool  hasSec      = false;    // indicates if the format has a second specifier (only for _time components)
    struct {                      // the compiled params for _wifi components
      /*  The page init string parameter for wifi components: ( {} optional )
        id          wifi data       examples
        -----------+---------------+------------------------------
        %N          wifi SSID       '%SSID is %N', '%N'
        %C          connected       'conn.: %C', '%C'
        %R          RSSI dBa
        %Q{<s,e>}   RSSI % {s..e}
        %I{[0..3]}  IP              'ip is %I', '%I', 'ip tail is %I[3]', '%I[3]'
        %G{[0..3]}  gateway
        %S{[0..3]}  subnet mask
      */
      char type;    // which wifi data
      int8_t s;       // either: the IP index or the quality range start for picture components
      int8_t e;       // the quality range end for picture components
    };
  };
};    // struct comp_t

struct {    // PG_upd struct for holding nesessery informations about the current page of the display
  // ----------  generel data  ----------
  char compStr[MAXLEN_COMPONENT_LIST+1] = {0};     // holds the string comming from the 'page init' message

  comp_t compList[50];
  int compCount = 0;

  void reset() {  // ### als normale methode und resettet dann auch touch!
    time.reset();
    wifi.reset();
    compCount = 0;
  }

  // ----------  time updates  ----------
  struct {    // time struct - for components starting with _time
    const unsigned long INTERVAL = 1000;
    bool used     = false;
    tm* timeinfo  = &NTP_store.timeinfo;

    struct {
      unsigned long loopMs = 0;
      int min   = 9999;
    }
    last;

    bool newMin()  { return (timeinfo->tm_min  != last.min); }

    void reset() {
      used         = false;
      last.loopMs  = 0;
      last.min     = 9999;
    }
    bool fresh() { return last.min == 9999; }
    void storeCurrent() {
      if (last.loopMs)
        last.loopMs += INTERVAL;    // for _time, we are going exatly 1000ms from the last
      else
        last.loopMs = millis() + INTERVAL;   // only for the first call we are using millis() as the base
      last.min = timeinfo->tm_min;
    }
    bool next() {
      return (used && (millis() > last.loopMs || fresh()) );
    }
  }
  time;

  // ----------  wifi updates  ----------
  struct {    // wifi struct - for components starting with _wifi
    const unsigned long INTERVAL = 9963;
    bool used = false;
    struct {  // value not changing while connected to WiFi
      char ssid[32+1] = {0};    // (%N)
      IPAddress ipv4  ;         // (%I)
      IPAddress gatewy;         // (%G)
      IPAddress snmask;         // (%S)
    }
    fixed;
    struct {
      unsigned long loopMs = 0;
      int rssi = 999;   // in dBa;  (%R)
      int conn = 9;     // WiFI 1=connected   (%C)
    }
    last;
    struct {
      int rssi;
      int conn;
    }
    curr;

    bool newConn()  { return (curr.conn != last.conn); }
    bool newRSSI()  { return (curr.rssi > last.rssi+5) || (curr.rssi < last.rssi-5); }  // differet to last +-5

    void reset() {
      last.loopMs  = 0;
      used         = false;
      last.rssi    = 999;
      memset(fixed.ssid, 0, 32);
      fixed.ipv4   = {0, 0, 0, 0};
      fixed.gatewy = {0, 0, 0, 0};
      fixed.snmask = {0, 0, 0, 0};
    }
    bool fresh() {
      return (last.rssi == 999) || ((int)(WiFi.status() == WL_CONNECTED) != last.conn);
    }
    void storeCurrent() {
      last.rssi   = curr.rssi;
      last.conn   = curr.conn;
      last.loopMs = millis() + INTERVAL;
    }

    bool next() {
      if (!used)    return false;

      // Update the fixed values if page is new or wifi connection changed
      if (fresh()) {
//###        if (WiFi.status() == WL_CONNECTED) {
        if (WiFi.isConnected()) {
          strncpy(fixed.ssid, wifiMgr.getWiFiSSID().c_str(), 32);
          fixed.ipv4   = WiFi.localIP();
          fixed.gatewy = WiFi.gatewayIP();
          fixed.snmask = WiFi.subnetMask();
        } else {
          strncpy(fixed.ssid, "not connected", 32);
          fixed.ipv4   = {0, 0, 0, 0};
          fixed.gatewy = {0, 0, 0, 0};
          fixed.snmask = {0, 0, 0, 0};
        }
      }
      bool ret = (millis() > last.loopMs || fresh());
      if (ret) {
        curr.conn = (WiFi.status() == WL_CONNECTED);
        curr.rssi = WiFi.RSSI();
      }
      return ret;
    }   // next
  }
  wifi;

  // struct {   //###
  //   //### cpuTemp, HAL, GPIO, ID, extTemp, Relay1 und 2
  // }
  // ESP;
}
PG_upd;

struct {    //###
  byte state = 0;

  int  downpos[2] = {0};
  unsigned long downtime = 0;

  bool up = false;
  int  uppos[2] = {0};
  unsigned long uptime = 0;

  void pressed(int x, int y) {
    state = 1;
    downpos[0] = x;
    downpos[1] = y;
    downtime = millis();
  }

  void released(int x, int y) {
    state = 0;
    uppos[0] = x;
    uppos[1] = y;
    uptime = millis();
Serial.printf(" released: dx=%1 dy=%i t=%i\n", uppos[0]-downpos[0], uppos[1]-downpos[1], uptime-downtime);
  }

  void reset() {
    state = 0;
    uppos[0] = 0;
    uppos[1] = 0;
    uptime = 0;
    downpos[0] = 0;
    downpos[1] = 0;
    downtime = 0;
  }
}
PG_touch;


/*
    -----  private functions  -----
*/

// format contains specifiers for seconds
bool timeHasSeconds(const char* fmt) {
  return
    strHas(fmt, "%S")  ||
    strHas(fmt, "%X")  ||
    strHas(fmt, "%T")  ||
    strHas(fmt, "%r")  ||
    strHas(fmt, "%c")  ||
    strHas(fmt, "%Ec") ||
    strHas(fmt, "%EX") ;
}


void wifiHandleRange(comp_t* comp) {
  char* rStart = strstr(comp->paramPtr, "<");
  char* rEnd   = strstr(comp->paramPtr, ">");
  char* comma  = strstr(comp->paramPtr, ",");

  if (
    (rStart != nullptr) && (rEnd != nullptr) && (comma != nullptr)
    && (comma > rStart+1) && (rEnd > comma+1)
  )
  {
    char num[5] = {0};
    strncpy(num, rStart+1, (comma-rStart-1));
    comp->s = atoi(num);
    strncpy(num, comma+1, (rEnd-comma-1));
    comp->e = atoi(num);
    // Serial.printf("  %s has range %d - %d\n", comp->namPtr, comp->s, comp->e);
  }

  rStart = strstr(comp->paramPtr, "[");
  rEnd   = strstr(comp->paramPtr, "]");

  if (
    (rStart != nullptr) && (rEnd != nullptr)
    && (rEnd > rStart+1)
  )
  {
    char num[5] = {0};
    strncpy(num, rStart+1, (rEnd-rStart-1));
    comp->s = atoi(num);
    // Serial.printf("  %s has index %d\n", comp->namPtr, comp->s);
  }
}   // wifiHandleRange()


void wifiParseParam(comp_t* comp) {
  comp->type  = '0';
  comp->s     = -1;
  comp->e     = -1;

  if (!comp->paramPtr) {
    Serial.printf("[PG:ERROR] wifi component '%s' has no param specifying the kind of data!\n", comp->namPtr);
    return;
  }

  wifiHandleRange(comp);

  if (strHas(comp->paramPtr, "%N")) {
    comp->type = 'N';
  } else
  if (strHas(comp->paramPtr, "%C")) {
    comp->type = 'C';
  } else
  if (strHas(comp->paramPtr, "%R")) {
    comp->type = 'R';
  } else
  if (strHas(comp->paramPtr, "%Q")) {
    comp->type = 'Q';
  } else
  if (strHas(comp->paramPtr, "%I")) {
    comp->type = 'I';
  } else
  if (strHas(comp->paramPtr, "%G")) {
    comp->type = 'G';
  } else
  if (strHas(comp->paramPtr, "%S")) {
    comp->type = 'S';
  } else {
    comp->type = '?';
    Serial.printf("[PG:ERROR] wifi component '%s' with param '%s' has an unknown data specifier!\n", comp->namPtr, comp->paramPtr);
  }

}   // wifiParseParam()


/*
  The function separates the tokens and splits the tokens into objname and its parameter.
  Parameters can be nullptr!
  Note:
   The token split chars will be changed to '\0' char, so that the pointer to the strings pointing to separated strings.
   At the end the hole string is fragmented into separated strings and it is not any more possible to search for a
   objname by using  strstr(compStr, "_compName")!=nullptr`.
*/
int splitCompStr() {
  #define TOK_SPLIT   ";"   // char to split components
  #define PROP_SPLIT  "&"   // char to split componentname from parameter

  PG_upd.reset();

  if (strlen(PG_upd.compStr) == 0) return 0;

  char *endStr;
  char *tok = strtok_r(PG_upd.compStr, TOK_SPLIT, &endStr);
  while (tok != NULL) {
    char* endTok;

    // We know the token has only two parts: objname and property
    PG_upd.compList[PG_upd.compCount].namPtr   = strtok_r(tok,  PROP_SPLIT,  &endTok);
    PG_upd.compList[PG_upd.compCount].paramPtr = strtok_r(NULL, PROP_SPLIT, &endTok);


    //----- which data has to be updated
    if (!PG_upd.time.used && strHas(PG_upd.compList[PG_upd.compCount].namPtr, "_time") ) // uses time component
      PG_upd.time.used = true;

    if (!PG_upd.wifi.used && strHas(PG_upd.compList[PG_upd.compCount].namPtr, "_wifi") ) // uses wifi component
      PG_upd.wifi.used = true;

    //----- check the parameter
    PG_upd.compList[PG_upd.compCount].hasSec =
        PG_upd.compList[PG_upd.compCount].paramPtr &&               // param defined
        PG_upd.time.used &&                                         // belongs to a time component
        timeHasSeconds(PG_upd.compList[PG_upd.compCount].paramPtr);  // format has second specifier

    if (PG_upd.wifi.used)  // belongs to a wifi component
      wifiParseParam(&PG_upd.compList[PG_upd.compCount]);


    //----- next token
    tok = strtok_r(NULL, TOK_SPLIT, &endStr);
    PG_upd.compCount++;
  }

  return PG_upd.compCount;
}   // splitCompStr()


// Searches the list for a componetname that starts with the specified string, starting with the specified startIdx
int hasCompname(const char* compName, int startIdx=0) {
  for (int i=startIdx; i<PG_upd.compCount; i++) {
    if (strHas(PG_upd.compList[i].namPtr, compName)) {
      return i;
    }
  }

  return -1;
}



/*
    -----  public functions  -----
*/

void Page_init(char* pgComplist) {
  memset(PG_upd.compStr, 0, sizeof(PG_upd.compStr));
  strncpy(PG_upd.compStr, pgComplist, MAXLEN_COMPONENT_LIST);
  Serial.println("[PG] Page init.");

  splitCompStr();

  //### log the list of components
  Serial.println("-----------  componentlist  ---------------");
  for (int i=0; i<PG_upd.compCount; i++) {
    Serial.printf("  %d: obj='%s' prop='%s'\n", i, PG_upd.compList[i].namPtr, PG_upd.compList[i].paramPtr?PG_upd.compList[i].paramPtr:"null");
  }
  Serial.println("-----------  componentlist  ---------------");
};


//###
// void Page_addComps(char* pgComplist) {
// }
// void Page_removeComps(char* pgComplist) {
// }


void Page_exit() {
  memset(PG_upd.compStr, 0, MAXLEN_COMPONENT_LIST);
  PG_upd.reset();

  Serial.println("[nexPG] Page exit");
};


/*
    -----  update functions  -----
*/

bool Page_updateTime() {
  char buff[NEX_MAX_NAMELEN+1 + NEX_MAX_TEXTLEN+1] = {0}; // buffer for the command to set the value of the component
  char fmt[MAX_FMTPARAM_LEN+1] = {0};                      // buffer for formats of the value

  // Serial.printf("%s\n", __func__);

  int idx = -1;
  do {
    idx++;
    idx = hasCompname("_time", idx);     // get all components starting with this name

    if (idx >= 0) {   // found component starting with _time
      // because this function is called every second, the second has definitely changed
      if ( !PG_upd.time.newMin() && !PG_upd.compList[idx].hasSec )
        continue;   // only second has changed, but the component has no second format secified

      // set default formats for the standard time components if none is defined in the parameter
      if (nullptr == PG_upd.compList[idx].paramPtr) {  // no param defined
        if (strcmp("_time.txt", PG_upd.compList[idx].namPtr) == 0) {
          strncpy(fmt, NEX_cfg.fmtTime[0], MAX_FMTPARAM_LEN);   // use the first default format
        }
        else
          // For all non standard _timeXYZ components paramPtr MUST be set in the 'page init' event
          // But at this point it is nullptr. Therefore we fill in one here to prevent errors
          strncpy(fmt, "99", MAX_FMTPARAM_LEN);  // only use numbers as it could also be a non-text component
      }
      else {  // component param is defined
        if (PG_upd.compList[idx].paramPtr[0] == '#') {  // first char is '#', use the format-macro of the specified number (#0-#3)
          int nr = atoi(PG_upd.compList[idx].paramPtr+1);
          if (nr >= 0 && nr <=3 && NEX_cfg.fmtTime[nr])
            strncpy(fmt, NEX_cfg.fmtTime[nr], MAX_FMTPARAM_LEN);  // copy from the format-macro
        }
        else
          strncpy(fmt, PG_upd.compList[idx].paramPtr, MAX_FMTPARAM_LEN);  // use param as format specifier for _time
      }

      // now it is sure that a format is defined

      // Is it the first update? Then check the format for existing second specifier.
      // This may be necessary if the format comes from a format macro, which is not checked for seconds in assignConfig()
      if (PG_upd.time.fresh() && !PG_upd.compList[idx].hasSec) {
        PG_upd.compList[idx].hasSec = timeHasSeconds(fmt);
      }

      bool isTxt = strHas(PG_upd.compList[idx].namPtr, ".txt");   // is it of type text?

      sprintf(buff, "%s=", PG_upd.compList[idx].namPtr);
      if (isTxt) strcat(buff, "\"");    // if component is text type, sourond the vale by '"'
      NTP_asString(fmt, (buff+strlen(buff)), NEX_MAX_TEXTLEN);
      if (isTxt) strcat(buff, "\"");

      NEX_sendCommand(buff, false);   // send the command to set the components value
    }   // if a component was found
  } while (idx >= 0 && idx < PG_upd.compCount);  // while component starts with basename found
};    // Page_updateTime()


bool Page_updateWifi() {
  // Serial.printf("----- %s start -----\n", __func__);

  char buff[NEX_MAX_NAMELEN+1 + NEX_MAX_TEXTLEN+1] = {0}; // buffer for the command to set the value of the component
  char fmt[MAX_FMTPARAM_LEN+1] = {0};                      // buffer for formats of the value

  int idx = -1;
  do {
    idx++;
    idx = hasCompname("_wifi", idx);     // get all components starting with this name

    if (idx >= 0) {   // found component starting with the searched string
      // this values are not changing while connection status is unchanged
      if (!PG_upd.wifi.fresh() &&
          (
            PG_upd.compList[idx].type == 'N' ||
            PG_upd.compList[idx].type == 'I' ||
            PG_upd.compList[idx].type == 'G' ||
            PG_upd.compList[idx].type == 'S'
          )
      )
      {
        continue;
      }

      bool isTxt  = strHas(PG_upd.compList[idx].namPtr, ".txt");   // is it of type text?

      sprintf(buff, "%s=", PG_upd.compList[idx].namPtr);
      if (isTxt) strcat(buff, "\"");

      // Handel the type of data the component wants to show
      switch (PG_upd.compList[idx].type) {

        // the component has a parameter with an unknown data spicifier
        case '?': {     // detected in wifiParseParam()
          if (isTxt)
            strcat(buff, "ERR");
          else
            strcat(buff, "999");
          break;
        }

        // the component has no parameter with a data spicifier
        case '0': {     // detected in wifiParseParam()
          if (isTxt)
            strcat(buff, "NUL");
          else
            strcat(buff, "990");
          break;
        }

        // Following values changing while connected to wifi
        case 'N': {
          strcat(buff, PG_upd.wifi.fixed.ssid);
          break;
        }
        case 'I': {
          if ( (PG_upd.compList[idx].s >= 0) && (PG_upd.compList[idx].s <=3) ) {
            // Only one Part of the ip should be used.
            sprintf(buff+strlen(buff), "%d",PG_upd.wifi.fixed.ipv4[PG_upd.compList[idx].s]);
          } else
            strcat(buff, PG_upd.wifi.fixed.ipv4.toString().c_str());
          break;
        }
        case 'G': {
          // Only one Part of the gateway ip should be used.
          if ( (PG_upd.compList[idx].s >= 0) && (PG_upd.compList[idx].s <=3) ) {
            sprintf(buff+strlen(buff), "%d",PG_upd.wifi.fixed.gatewy[PG_upd.compList[idx].s]);
          } else
            strcat(buff, PG_upd.wifi.fixed.gatewy.toString().c_str());
          break;
        }
        case 'S': {
          // Only one Part of the subnet mask should be used.
          if ( (PG_upd.compList[idx].s >= 0) && (PG_upd.compList[idx].s <=3) ) {
            sprintf(buff+strlen(buff), "%d",PG_upd.wifi.fixed.snmask[PG_upd.compList[idx].s]);
          } else
            strcat(buff, PG_upd.wifi.fixed.snmask.toString().c_str());
          break;
        }
  Serial.println(WiFi.macAddress());
/*### M = MAC
  WiFi.BSSID(*bssid);
  Serial.println(WiFi.macAddress());
  const uint8_t bssid[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  30:AE:A4:07:0D:64 als Text
  %M[0..5] Teilwert als Zahl
*/
        // Following values changing while connected to wifi
        case 'C': {
          if (!PG_upd.wifi.newConn() )  continue;
          sprintf(buff+strlen(buff), "%d", PG_upd.wifi.curr.conn);
          break;
        }
        case 'R': {
          if (!PG_upd.wifi.newRSSI() )  continue;
          sprintf(buff+strlen(buff), "%d", PG_upd.wifi.curr.rssi);
          break;
        }
        case 'Q': {
          if (!PG_upd.wifi.newRSSI())  continue;

          float Q = 0.0;
          // rssi is zero if wifi not connected!
          if (PG_upd.wifi.curr.rssi < 0)
            Q = (-100 / -50) * (constrain(PG_upd.wifi.curr.rssi, -100, -50) + 100);

          /*
            RSSI level less than -80 dBm may not be usable, depending on noise.
            In general, the following correlation can be applied:
              RSSI ≥ -50 dBm => quality = 100%
              RSSI ≤ -100 dBm => quality = 0%
              -100 dBm ≤ RSSI ≤ -50 dBm => quality ~= 2 x (RSSI + 100)
          */
          if ( (PG_upd.compList[idx].s >= 0) && (PG_upd.compList[idx].e >= 0)) {    // use a range instaed of %
            int   r = PG_upd.compList[idx].e - PG_upd.compList[idx].s + 1;
            float v = constrain(Q * r / 100, PG_upd.compList[idx].s, PG_upd.compList[idx].e);
            sprintf(buff+strlen(buff), "%d", (int)v + PG_upd.compList[idx].s);
          } else {
            sprintf(buff+strlen(buff), "%d", (int)Q);
          }
          break;
        }
        default: { strcat(buff, "99"); }     // should never happen
      }   // switch changing wifi data

      if (isTxt) strcat(buff, "\"");
      NEX_sendCommand(buff, false);   // send the command to set the components value
    }   // if a component was found
  } while (idx >= 0 && idx < PG_upd.compCount);  // while component starts with basename found

  // Serial.printf("----- %s done -----\n\n", __func__);
};    // Page_updateWifi()
