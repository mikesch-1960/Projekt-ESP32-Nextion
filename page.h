/*
v check for MAX_COMPONENT_ITEMS  or  set MAX_COMPONENT_ITEMS dynamic
x sync seconds at the first call of update_time() to a change of seconds
v NEX_cfg.fmtTime[0] must exist for nullptr format strings
- build a format string for empty strings similar to boolean format strings
  OR: use boolean format strings for this porposes in a other way
*/

/*-----------------------------------------------------------------------------------------
  page init event list format:    CompName=DataType=FormatString^...
    comp.ext=N|C|Q{,s:e}|R{,s:e}|[I|G|S]{,i}|M{,i}|T{smd}{=fmtStr|?truetxt:falsetxt}^...

  If no format string is specified, a default format matching the value to be displayed is used (%d or %s).
  A range specifier forces the incoming value into the specified range. This can be used to define a picture for a status like signal quality.
    Example: Q,2:4 means that the quality (0..100) is now in range of 2..4. This value can be assigned to picture component to display the quality.
  IP addresses and the mac address can have an index to only use one element of the array. The index must match the size of the array!
    Example: I,3 shows the last number from the ip address.

  group time: (Updates like defined behind the data type in the update period specifier)
    T{s-ecound|m-minute|d-ay} specifies the update period. If none is specified m is the default. So if You want to show seconds you have to specify this!
      Format string must be a valid timestamp format. See https://cplusplus.com/reference/ctime/strftime/ or
      https://help.gnome.org/users/gthumb/unstable/gthumb-date-formats.html.de in german.

  group wifi: (Updates like defined in the Pg_upd.wifi.INTERVAL setting)  ### define the intervall on top of the page.h
    C-onnected. Values are 0 or 1
      Can have a boolean Format string like C=?truetxt:falsetxt
    R-SSI. In dBa. Allways negative.
      Can have a range specifier in the range from 0 to 254: R,s:e.
    Q-uality. The dBa value as a percentage value.
      Can have a range specifier in the range from 0 to 254: Q,s:e.
    I-p,  G-ateway address.   S-ubnetmask.
      Can have an index specifier in the range from 0 to 3: I,3.
    M-ac address.
      Can have an index specifier in the range from 0 to 5: M,5.
    N-ame: SSID of the connected wifi or 'not connected'. ### use boolean format string also for wifi name? (...N=?%s:OFFLINE^)

  group mqtt: (Updated when a mqtt message was received for that topic)
    # for MQTT. Must have a topic behind the # specifier.
    Needs more exact description!
    nam1.txt=#0_userdata/example=%s^nam2.val=#/data=%d^
**---------------------------------------------------------------------------------------*/

#define NOT_APPLIED   255
#define isApplied(idxOrRng)     (idxOrRng < NOT_APPLIED)

#define DT_TIME       'T'
#define DT_RSSI       'R'
#define DT_QUAL       'Q'
#define DT_IPV4       'I'
#define DT_GATW       'G'
#define DT_SUBN       'S'
#define DT_MAC        'M'
#define DT_CONN       'C'
#define DT_SSID       'N'
#define DT_MQTT       '#'

/*
  When this is defined, the format of the 'page init' message is heavily checked for errors.
  This can be switched off if no errors were found and no more changes are made to the HMI file.
  Then it is no longer checked for errors!
*/
#define VALIDATE_PI_MSG  // comment out to disable intensive error checking
#ifdef VALIDATE_PI_MSG
  const char INDEX_TYPES[]     = { DT_IPV4, DT_GATW, DT_SUBN, DT_MAC, '\0' };  // Can have index specifier
  const char RANGE_TYPES[]     = { DT_RSSI, DT_QUAL, '\0' };                 // Can have range specifier
  const char BOOLEAN_TYPES[]   = { DT_CONN, '\0' };     // Can use a special format string with one text for true and an other for false.

  const char WIFI_TYPES[]      = {
                                   DT_IPV4, DT_GATW, DT_SUBN, DT_MAC    // INDEX_TYPES
                                 , DT_RSSI, DT_QUAL                   // RANGE_TYPES
                                 , DT_CONN                            // BOOLEAN_TYPES
                                 , DT_SSID
                                 , '\0'
                                 };
  const char ALL_DATA_TYPES[]  = {
                                   DT_IPV4, DT_GATW, DT_SUBN, DT_MAC,  DT_RSSI, DT_QUAL,  DT_CONN,  DT_SSID   // WIFI_TYPES
                                 , DT_TIME, DT_MQTT
                                 , '\0'
                                 };

  #define COMP_IS_IGNORED       "! Component is ignored."
#endif  // ifdef VALIDATE_PI_MSG

// Value of these data types remain unchanged as long as the Wi-Fi connection status is unchanged
const char FIXED_WIFI_TYPES[]  = { DT_SSID, DT_IPV4, DT_GATW, DT_SUBN, DT_MAC, '\0' };


struct __attribute__ ((packed)) comp_t {    // one packed struct for every token in the compStr
  char* ptrName = nullptr;  // pointer to the name of the component
  //### perhaps: replace pointer by byte or int offset from ptrName (saves 2 or 3 byte per component)
  char* ptrFmt  = nullptr;  // pointer to the format the data should use
  char  cType   = '\0';     // type of data the component should show

  union {
    struct __attribute__ ((packed)) {     // for wifi data types that can have an index(s) or range(s to e)
      uint8_t s;     // the index or the range start
      uint8_t e;     // the range end
    };
    struct __attribute__ ((packed)) {     // for wifi data type C with a boolean format
      // The ':' in the boolean format string will be replaced by \0 and the offset points to the text behind that
      uint8_t falseOffset;
    };
    struct __attribute__ ((packed)) {     // for time data types time
      char timeUpdPeriod;   // d-ay, m-inute, s-econd; only for data type T-ime; T cant have index or range!
    };
    struct __attribute__ ((packed)) {     // for mqtt data types
      uint8_t topicOffset;   // offset from the ptrName to the topic of a MQTT data type component ( char* ptrTopic = comp.ptrName+(size_t)comp.topicOffset )
      uint8_t fullTopicIdx;  // For components with a relative topic: The index of component with the absolute topic to use for
    };
  };
};    // struct comp_t


struct {    // PG_upd struct for holding nesessery informations about the current page on the display
  // Holds the hole string comming from the 'page init' message which lists all components to be updated on the actual page
  // The string is changed a lot in the splitCompStr() function and is also divided into several strings.
  char compStr[MAXLEN_COMPONENT_LIST+1] = {0};

  comp_t compList[MAX_COMPONENT_ITEMS];    // for every component in the compStr splitCompStr() generates an element in this list
  int compCount = 0;

  // These values are calculated at the end of splitCompStr()
  // int timeStartIdx = 0;  // because of the sort order of the components, this is allways 0!
  int     wifiStartIdx;                       // Index of the first wifi component in the ordered component list
  int     mqttStartIdx;                       // Index of the first mqtt component in the ordered component list

  /*
    This enum determines the sort order of the components. The sort order is very important because the update
    functions do use this sorting to update only the components that are required for the respective function.
    Never change the sort order without adjusting the while loops of the update functions!
  */
  enum DataGroup { grTIME, grWIFI_DYN, grWIFI_FIX, grMQTT };

  // Used for sorting the componentlist by data type groups
  DataGroup getDataTypeGroup(comp_t* comp) {
    const char t = comp->cType;
    return t==DT_TIME ? grTIME : (t==DT_MQTT ? grMQTT : (strchr(FIXED_WIFI_TYPES, t) ? grWIFI_FIX : grWIFI_DYN) );
  }


  void reset() {
    memset(compStr, 0, MAXLEN_COMPONENT_LIST);
    time.reset();
    wifi.reset();
    compCount = 0;
  }


  // writes the componentname to the buffer 'name' and expandes the extension if necessary
  void getCompFullname(const comp_t* comp, char* name) {
    strcpy(name, comp->ptrName);

    char* p = strchr(name, '.');    // Globol variables in the ´Program.s´ code has no extension
    if (!p) return;

    p++;
    if (strcmp(p, "t") == 0)
      strcpy(p, "txt");
    else
    if (strcmp(p, "v") == 0)
      strcpy(p, "val");
  }


  bool isBoolFmt(const comp_t* comp) {
    return comp->ptrFmt && comp->ptrFmt[0]=='?';
  }


  char* getBooltextPtr(const comp_t* comp, int val) {
    if (val)
      return comp->ptrFmt + 1;
    else
      return comp->ptrFmt + (size_t)comp->falseOffset;
  }


  // returns the index to the buffer with defined format macros, or -1 if comp do not have a format macro
  int isFmtMacro(const comp_t* comp) {
    if (comp->ptrFmt && comp->ptrFmt[0]=='$' && strlen(comp->ptrFmt)>1) {
      return atoi(comp->ptrFmt+1);
    } else
      return -1;
  }


  bool topicEqual(const int compIdx, char* searchTopic, char* topicBuffer) {
    readAbsoluteTopic(compIdx, topicBuffer);
    return topicBuffer && strcmp(topicBuffer, searchTopic) == 0;
  }


  void readAbsoluteTopic(const int compIdx, char* topicBuff) {
    topicBuff[0] = '\0';
    char* compTopic = compList[compIdx].ptrName + compList[compIdx].topicOffset;

    if (compTopic[0] == '/') {    // a other topic at the same level as a previous full topic in a component before the current
      int idx = compIdx-1;

      // if the fullTopicIdx allready known, use it to go directly to the component with the corresponding absolute topic
      if (compList[compIdx].fullTopicIdx < MAX_COMPONENT_ITEMS){
        idx = compList[compIdx].fullTopicIdx;
//log_v("### readAbsoluteTopic: use abs idx %d for comp %s", idx, compList[compIdx].ptrName);
      }

      // for (int idx=compIdx-1; idx >= mqttStartIdx; idx--) {
      for (idx; idx >= mqttStartIdx; idx--) {
        if ((compList[idx].ptrName + compList[idx].topicOffset)[0] != '/') {    // is it the corresponding absolute topic?
          if (compList[compIdx].fullTopicIdx == MAX_COMPONENT_ITEMS) {
            compList[compIdx].fullTopicIdx = idx; // store the corresponding absolute topic to use it directly next time
//log_v("### readAbsoluteTopic: store abs idx %d for comp %s", idx, compList[compIdx].ptrName);
          }

          strncpy(topicBuff, (compList[idx].ptrName + compList[idx].topicOffset), 256);   // copy the absolute topic to the result buffer

          char* prevLevel = strrchr(topicBuff, '/');    // find the last backslash in the result
          if (prevLevel)
            // exchange the last part of the absolute topic by the relative topic of the current component
            strncpy(prevLevel, compTopic, 256-strlen(topicBuff));

//log_v("---### comp=%s  topic=%s", compList[compIdx].ptrName, topicBuff);

          break;   // exit the loop
        }
      }
    } else {
      strncpy(topicBuff, compTopic, 256);
    }
//log_v("    get full topic from %s is '%s'", compList[compIdx].ptrName, topic);
  }   // readAbsoluteTopic()


  // -----------------------------------------------------------------
  struct {    // time struct - for components of data types T
    tm* timeinfo  = &NTP.timeinfo;

    struct {
      unsigned long loopMs = 0;
      int min   = 9999;
      int day   = 9999;
    }
    last;

    bool newMin()  { return (timeinfo->tm_min  != last.min); }
    bool newDay()  { return (timeinfo->tm_mday != last.day); }

    void reset() {
      last.loopMs = 0;
      last.min    = 9999;
      last.day    = 9999;
    }

    bool fresh() { return last.min == 9999; }

    void storeCurrent() {
      last.min = timeinfo->tm_min;
      last.day = timeinfo->tm_mday;
    }

    bool next() {
      if (millis() >= last.loopMs || fresh()) {
        if (last.loopMs > 0)
          last.loopMs += 1000;    // for _time, we are going exatly 1000ms from the last
        else
          last.loopMs = millis() + 1000;   // only for the first call we are using millis() as the base
        return true;
      }
      return false;
    }
  }
  time;

  // ----------  special wifi data  ----------
  struct {    // wifi struct - for components of wifi data type
    // Use an odd millisecond specification to prevent many values from being updated at the same time
    const unsigned long INTERVAL = 9832;

    struct {  // value not changing while connected to WiFi
      char ssid[32+1] = {0};    // (N)
      IPAddress ipv4  ;         // (I)
      IPAddress gatewy;         // (G)
      IPAddress snmask;         // (S)
      uint8_t   mac[6];         // (M)
    }
    fixed;

    struct {    // the last values updated to the components
      unsigned long loopMs = 0;
      int rssi = 999;   // in dBa;  (R)
      int conn = 9;     // WiFI 1=connected   (C)
    }
    last;

    struct {    // current value; is loaded before updating the components
      int rssi;
      int conn;
    }
    curr;

    bool newConn()  { return (curr.conn != last.conn); }
    bool newRSSI()  { return (curr.rssi > last.rssi+5) || (curr.rssi < last.rssi-5); }  // differet to last +-5
    //bool newRSSI()  { return curr.rssi != last.rssi; }

    void reset() {
      last.loopMs  = 0;
      last.rssi    = 999;
      last.conn    = 999;
      memset(fixed.ssid, 0, 32);
      fixed.ipv4   = {0, 0, 0, 0};
      fixed.gatewy = {0, 0, 0, 0};
      fixed.snmask = {0, 0, 0, 0};
      memset(fixed.mac, 0, 6);
    }

    bool fresh() {
      return (last.rssi == 999) || ((int)(WiFi.status() == WL_CONNECTED) != last.conn);
    }

    void storeCurrent() {
      last.rssi   = curr.rssi;
      last.conn   = curr.conn;
      last.loopMs = millis() + INTERVAL;  // ### time.last.loopMs + 200 instead of millis() to prevent many values from being updated at the same time?
    }

    bool next() {
      // Update the fixed values if page is new or wifi connection changed
      if (fresh()) {
        if (WiFi.isConnected()) {
          strncpy(fixed.ssid, WiFi.SSID().c_str(), 32);
          fixed.ipv4   = WiFi.localIP();
          fixed.gatewy = WiFi.gatewayIP();
          fixed.snmask = WiFi.subnetMask();
          // Get MAC address for WiFi station
          esp_read_mac(fixed.mac, ESP_MAC_WIFI_STA);
        } else {
          strncpy(fixed.ssid, "unconnected", 32);  //### read value from config struct
          fixed.ipv4   = {0, 0, 0, 0};
          fixed.gatewy = {0, 0, 0, 0};
          fixed.snmask = {0, 0, 0, 0};
          memset(fixed.mac, 0, 6);
        }
      }

      if (millis() > last.loopMs || fresh()) {
        curr.conn = (WiFi.status() == WL_CONNECTED);
        curr.rssi = WiFi.RSSI();
        return true;
      }

      return false;
    }   // next()
  }
  wifi;
}
PG_upd;


/*
  Determines any specified index(s) or range(s-e).
  Returns true if the function detects an error.
*/
bool handleTypeExtras(comp_t* comp, char* ptrType) {
  // examples: Q,1:5 -> range 1 to 5, or I,3 index 3 => the comma must be at the secound place

  if (comp->cType == DT_TIME) {   // is the data type 'Time'?
    comp->timeUpdPeriod = ptrType[1]=='\0' ? 'm' : ptrType[1];   // Has the time an update period specifier? Else use m-inute as default

#ifdef VALIDATE_PI_MSG
    // test if update period is valid ('\0' means that no update period was specified, then 'm' is used as the default)
    if (!strchr("smd\0", ptrType[1])) {
      log_e("14) The time update period specifier '%c' for component '%s' is invalid! Use:'s','m' or 'd'%s",
          ptrType[1], comp->ptrName, COMP_IS_IGNORED);
      return true;
    }
#endif
    return false;
  }

  if (comp->cType == DT_MQTT) {   // is the data type 'MQTT'?
    comp->topicOffset = (uint8_t)(ptrType - comp->ptrName + 1);   // with this offset ptrName+topicOffset points to the topic

#ifdef VALIDATE_PI_MSG
    // test if a topic is specified
    if (strlen(ptrType) <= 1) {
      log_e("15) Empty topic for MQTT type of component '%s' is not allowed%s",
          comp->ptrName, COMP_IS_IGNORED);
      return true;
    }
#endif
    return false;
  }

  if (PG_upd.isBoolFmt(comp)) {     // a boolean format specifier is used for this component
    char* falseText = strchr(comp->ptrFmt, ':');
    if (falseText) {
      falseText[0] = '\0';
      comp->falseOffset = (uint8_t)(falseText- comp->ptrFmt) + 1;
    }
  }

  char* comma = strstr(ptrType, ",");

  if (!comma) {       // no comma means no index or range
    return false;     // no comma found or data type is T!
  }

  char* colon = strstr(ptrType, ":");

#ifdef VALIDATE_PI_MSG
  // test whether the data type allows an index specification
  if (!colon && !strchr(INDEX_TYPES, comp->cType)) {
    log_e("1) The data type '%c' for component '%s' do not allow an index specification%s",
        comp->cType, comp->ptrName, COMP_IS_IGNORED);
    return true;
  }

  // test whether the data type allows a range specification
  if (colon && !strchr(RANGE_TYPES, comp->cType)) {
    log_e("2) The data type '%c' for component '%s' do not allow a range specification%s",
        comp->cType, comp->ptrName, COMP_IS_IGNORED);
    return true;
  }

  // comma not at the second positon or colon not behind a number after the comma or no value after the colon
  if ((comma != ptrType+1) || (colon && (colon < comma+2 || colon[1] == '\0'))) {
    log_e("3) Index or range specification in data type '%s' for component '%s' has invalid format%s",
        ptrType, comp->ptrName, COMP_IS_IGNORED);
    return true;
  }
#endif

  comma[0] = '\0';
  comma++;

  char num[5] = {0};
  int i;

  if (colon) {
    colon[0] = '\0';
    colon++;

    strcpy(num, colon);
    i = atoi(num);
#ifdef VALIDATE_PI_MSG
    for (int n = 0; n < strlen(num); n++)
      if (num[n]<'0' || num[n]>'9') {
        log_e("4) Range-end value for data type '%s' of component '%s' is not a number%s",
            ptrType, comp->ptrName, COMP_IS_IGNORED);
        return true;
      }

    // check for valid range end
    if (i < 1 || i > 254) {
      log_e("5) Range-end value for data type '%s' of component '%s' must be <= 254%s",
          ptrType, comp->ptrName, COMP_IS_IGNORED);
      return true;
    }
#endif
    comp->e = i;
  }

  strcpy(num, comma);
  i = atoi(num);

#ifdef VALIDATE_PI_MSG
  for (int n = 0; n < strlen(num); n++)
    if (num[n]<'0' || num[n]>'9') {
      log_e("6) %s value for data type '%s' of component '%s' is not a number%s",
          colon ? "Range-start" : "Index", ptrType, comp->ptrName, COMP_IS_IGNORED);
      return true;
    }

  // check for valid range start. The max. range end is 254, so the start max is 253!
  if (colon && (i > 253 || i < 0 || i >= comp->e)) {
    log_e("7) Range-start value for data type '%s' of component '%s' is invalid%s",
        ptrType, comp->ptrName, COMP_IS_IGNORED);
    return true;
  }

  // check for valid index
  uint8_t maxIdx = (comp->cType == 'M') ? 5 : 3;   // mac do have 6 numbers while ip's have 4
  if (!colon && (i < 0 || i > maxIdx)) {
    log_e("8) Index value for data type '%s' of component '%s' must be <= %d%s",
        ptrType, comp->ptrName, maxIdx, COMP_IS_IGNORED);
    return true;
  }
#endif
  comp->s = i;

  return false;
}


/*
  To sort the componentlist, the data types are grouped.
  First all time components, then the WLAN components that change during an existing WLAN connection.
  Followed by the WLAN components that do not change during an existing WLAN connection and finally
  the MQTT components. (The order of the components is determined by enum DataGroup!)
  The function is called as a callback for the qsort() at the end of the splitCompStr() function.
*/
int compareByType(const void *elem1, const void *elem2) {
  return PG_upd.getDataTypeGroup((comp_t*)elem1) > PG_upd.getDataTypeGroup((comp_t*)elem2) ? 1 : -1;
}


/*
  Separates the 'page init' message into the components and collects all importent informations for every component.
  The compStr is splitted into many seperate strings, by setting the split characters to `\0`!
*/
void splitCompStr() {
  #define COMP_SPLIT    "^"   // for separationg components
  #define INNER_SPLIT   '='   // char to split data type from format string

  if (strlen(PG_upd.compStr) == 0) return;

  char* ptrType = nullptr;
  char *endStr;
  char *tok = strtok_r(PG_upd.compStr, COMP_SPLIT, &endStr);
  while (tok != NULL) {
    if (PG_upd.compCount == MAX_COMPONENT_ITEMS) {
      log_e("20) Components behind %s are too much. A maximum of %d are allowed. All following components are ignored!",
          PG_upd.compList[PG_upd.compCount-1].ptrName, MAX_COMPONENT_ITEMS);
      break;
    }

    comp_t* comp = &PG_upd.compList[PG_upd.compCount];

    comp->ptrName = tok;
    comp->ptrFmt  = nullptr;

    ptrType = strchr(tok, INNER_SPLIT);

    if (nullptr != ptrType) {  // INNER_SPLIT behind compname found
      ptrType[0] = '\0';
      ptrType++;
      comp->cType = ptrType[0];
      if (comp->cType == DT_TIME) {
        comp->timeUpdPeriod = ptrType[1] == '\0' ? 'm' : ptrType[1];
      } else {
        // NOT_APPLIED means not used! Real values will be detected in handleTypeExtras() called later
        comp->s = NOT_APPLIED;   comp->e = NOT_APPLIED;
      }

      char* ptr = strchr(ptrType, INNER_SPLIT);
      // the format string is not mandatory
      if (nullptr != ptr) {  // INNER_SPLIT behind data type found
        ptr[0] = '\0';
        ptr++;
        comp->ptrFmt = ptr;
      }
    }

#ifdef VALIDATE_PI_MSG
    bool hasErr = false;

    // do the componentname have an extension?
    //### falls für globale Variablen verwendet, ist das kein Fehler. Die haben keine extension und sind nur vom Typ Integer!
    char* ext = strchr(comp->ptrName, '.');
    if (!ext || ext[1] == '\0') {
      log_e("9) Missing extension in componentname '%s'%s",
          comp->ptrName, COMP_IS_IGNORED);
      hasErr = true;
    }

    // do the format string contain a format specifier? '$' means a format macro and '?' means a boolean format, wich do not have a format specifier
    if (comp->ptrFmt && comp->ptrFmt[0] != '$' && comp->ptrFmt[0] != '?' && !strchr(comp->ptrFmt, '%') ) {
      log_e("10) Missing format specifier in format string '%s' for component '%s'%s",
          comp->ptrFmt, comp->ptrName, COMP_IS_IGNORED);
      hasErr = true;
    }

    // do the component use a format macro?
    int n = PG_upd.isFmtMacro(comp);
    if (n >= 0) {   // component uses a format macro
      if (n >= MAX_FMT_MACROS) {   // do the macro point to a valid index
        log_e("16) The format macro index %d(max.=%d) for component '%s' is out of range%s",
            n, MAX_FMT_MACROS-1, comp->ptrName, COMP_IS_IGNORED);
        hasErr = true;
      }
      else
      if (!NEX_cfg.fmtTime[n] || strlen(NEX_cfg.fmtTime[n]) < 2) {  // do the macro point to a valid format buffer in the config structure
        log_e("19) The format macro at index %d for component '%s' is empty or invalid%s",
            n, comp->ptrName, COMP_IS_IGNORED);
        hasErr = true;
      }
    }

    // is the boolean format valid?
    if (PG_upd.isBoolFmt(comp) && !strchr(comp->ptrFmt, ':')) {
      log_e("17) The boolean format for component '%s' is missing the ':' for the false text part%s",
          comp->ptrName, COMP_IS_IGNORED);
      hasErr = true;
    }

    // has a data type?
    if (ptrType == nullptr || ptrType[0] == '\0') {
      log_e("11) Missing type of data for component '%s'%s",
          comp->ptrName, COMP_IS_IGNORED);
      hasErr = true;
    }

    // is the given data type allowed?
    if (!strchr(ALL_DATA_TYPES, comp->cType)) {
      log_e("12) Invalide type of data '%c' for component '%s'%s",
          comp->cType, comp->ptrName, COMP_IS_IGNORED);
      hasErr = true;
    }

    // format string is boolean format but only allowed for boolean data types!
    if (!strchr(BOOLEAN_TYPES, comp->cType) && comp->ptrFmt && comp->ptrFmt[0] == '?') {
      log_e("13) Boolean format for component '%s' is not allowed for data type '%c'%s",
          comp->ptrName, comp->cType, COMP_IS_IGNORED);
      hasErr = true;
    }

    hasErr |=
#endif
    handleTypeExtras(comp, ptrType);

    //----- next token
    tok = strtok_r(NULL, COMP_SPLIT, &endStr);

#ifdef VALIDATE_PI_MSG
    if (!hasErr) {
      // does the the component name allready exist
      char nameNew[NEX_MAX_NAMELEN+1];
      char nameIdx[NEX_MAX_NAMELEN+1];
      PG_upd.getCompFullname(comp, nameNew);

      for (int n=PG_upd.compCount-1; n>=0; n--) {
        PG_upd.getCompFullname(&PG_upd.compList[n], nameIdx);
        if (strcmp(nameNew, nameIdx) == 0) {
          log_e("18) The component '%s' do allready exist at index %d%s",
            comp->ptrName, n, COMP_IS_IGNORED);
          hasErr = true;
          break;
        }
      }
    }

    if (!hasErr)
#endif
    PG_upd.compCount++;
  }   // while token

  qsort(PG_upd.compList, PG_upd.compCount, sizeof(comp_t), compareByType);

  PG_upd.wifiStartIdx = MAX_COMPONENT_ITEMS;
  PG_upd.mqttStartIdx = MAX_COMPONENT_ITEMS;

  // Because of the sort order, we can find the starting index for each group. Time components start at 0!
  for (int i=0; i<PG_upd.compCount; i++) {
    if (PG_upd.wifiStartIdx == MAX_COMPONENT_ITEMS &&
        PG_upd.compList[i].cType != DT_TIME &&
        PG_upd.compList[i].cType != DT_MQTT
    ) {   // must be a wifi component
      PG_upd.wifiStartIdx = i;
    }

    if (PG_upd.mqttStartIdx == MAX_COMPONENT_ITEMS &&
        PG_upd.compList[i].cType == DT_MQTT
    ) {
      PG_upd.mqttStartIdx = i;
      break;
    }
  }


#pragma region comp list
  //### log the list of components
  log_d("-----------  start componentlist  ---------------");
  char numS[4];
  char numE[4];
  for (int i=0; i<PG_upd.compCount; i++) {
    const comp_t* comp = &PG_upd.compList[i];

    if
    (
#ifdef VALIDATE_PI_MSG
      (strchr(INDEX_TYPES, comp->cType) || strchr(RANGE_TYPES, comp->cType)) &&
#endif
      comp->s < NOT_APPLIED
    )
     sprintf(numS, "%u", comp->s);
    else
      strcpy(numS, "---");

    if
    (
#ifdef VALIDATE_PI_MSG
      strchr(RANGE_TYPES, comp->cType) &&
#endif
      comp->e < NOT_APPLIED
    )
     sprintf(numE, "%u", comp->e);
    else
      strcpy(numE, "---");

    log_d("  %2.2d: nam=%-10s type=%c%c\tfmt=%-15s s=%-3s \t e=%-3s",
        i,
        comp->ptrName,
        comp->cType, comp->cType==DT_TIME ? comp->timeUpdPeriod : ' ',
        comp->ptrFmt ? comp->ptrFmt : "nullptr",
        numS, numE
    );
  }
  log_d("number of elements: %d", PG_upd.compCount);
  log_d("-----------  end componentlist  ---------------");
#pragma endregion comp list
}   // splitCompStr()


//-----------------------------------------------------------------------------------------

void Page_init(char* pgComplist) {
  log_i("[PG] Page init.");
  PG_upd.reset();
  strncpy(PG_upd.compStr, pgComplist, MAXLEN_COMPONENT_LIST);

  splitCompStr();
  subscribePage();
};    // Page_init()


void Page_exit() {
  unsubscribePage();
  PG_upd.reset();
  log_i("[nexPG] Page exit");
};    // Page_exit()



/*
    -----  update functions  -----
*/

void Page_updateTime() {
  // name...="value..."\0
  char buff[NEX_MAX_NAMELEN + 2 + NEX_MAX_TEXTLEN + 2] = {0}; // buffer for the command to set the value of the component

  // the time components are sorted to the top of the component list
  int idx = 0;
  while (idx < PG_upd.compCount && PG_upd.compList[idx].cType == DT_TIME) {
    const comp_t* comp = &PG_upd.compList[idx];

    if (
        comp->timeUpdPeriod == 's' ||
        (PG_upd.time.newMin() && comp->timeUpdPeriod == 'm') ||
        (PG_upd.time.newDay() && comp->timeUpdPeriod == 'd')
      )
    {
      PG_upd.getCompFullname(comp, buff);

      char* fmt = comp->ptrFmt;
      if (!comp->ptrFmt || comp->ptrFmt[0] == '$') { // it is a format macro!
        fmt = &NEX_cfg.fmtTime[!comp->ptrFmt ? 0 : atoi(comp->ptrFmt+1)][0];
      }

      // until now, buff only contains the componentname
      bool isTxt = strstr(buff, ".txt");
      strcat(buff, "=");
      if (isTxt) strcat(buff, "\"");    // if component is text type, sourond the value by '"'
      NTP_asString(fmt, (buff+strlen(buff)), NEX_MAX_TEXTLEN);  // write the formated time to the buffer
      if (isTxt) strcat(buff, "\"");

      // send the command to the Nextion
      NEX_sendCommand(buff);
    }

    idx++;
  }   // while( time-component )
}   // Page_updateTime()


void Page_updateWifi() {
  // name...="value..."\0
  char buff[NEX_MAX_NAMELEN + 2 + NEX_MAX_TEXTLEN + 2] = {0}; // buffer for the command to set the value of the component

  // wifi components are sorted after the time components, possible MQTT components will follow afterwards
  int idx = PG_upd.wifiStartIdx;
  while (idx < PG_upd.compCount && PG_upd.compList[idx].cType != DT_MQTT) {
    if (!PG_upd.wifi.fresh() && strchr(FIXED_WIFI_TYPES, PG_upd.compList[idx].cType)) {
      /*
        Fixed values remain unchanged as long as the Wi-Fi connection status is unchanged. Because of the sort order,
        the loop can be aborted here, as it is ensured that the WiFi status is unchanged and only components that are
        then unchanged follow.
       */
      // idx++; continue;
      break;
    }

    const comp_t* comp = &PG_upd.compList[idx];

    PG_upd.getCompFullname(comp, buff);
    bool isTxt = strstr(buff, ".txt");

    strcat(buff, "=");
    if (isTxt) strcat(buff, "\"");    // if component is text type, sourond the value by '"'

    char defFmt[3] = "%d";
    // if comp->ptrFmt is null or empty, use a default format
    char* fmt = (!comp->ptrFmt || strlen(comp->ptrFmt)==0) ? &defFmt[0] : comp->ptrFmt;

    // log_d("  upd-wifi %d: %s(%c) fmt=%s", idx, comp->ptrName, comp->cType, fmt);

    // read the corresponding wifi data and send the uodate command to the Nextion
    switch (comp->cType) {
      //****  Following wifi values remain unchanged as long as the Wi-Fi connection status is unchanged  ****
      case DT_SSID: {   // wifi name
        defFmt[1] = 's';
        sprintf(buff+strlen(buff), fmt, PG_upd.wifi.fixed.ssid);
        break;
      }

      case DT_CONN: {   // connection status
        if (PG_upd.isBoolFmt(comp)) {   // use boolean format
          fmt = PG_upd.getBooltextPtr(comp, PG_upd.wifi.curr.conn);
          sprintf(buff+strlen(buff), "%s", fmt);
        } else {
          defFmt[1] = 'd';
          sprintf(buff+strlen(buff), fmt, PG_upd.wifi.curr.conn);
        }
        break;
      }

      case DT_IPV4: case DT_GATW: case DT_SUBN: {
        IPAddress val;
        switch (comp->cType) {
          case DT_IPV4:    val = PG_upd.wifi.fixed.ipv4;   break;
          case DT_GATW:  val = PG_upd.wifi.fixed.gatewy; break;
          case DT_SUBN:  val = PG_upd.wifi.fixed.snmask; break;
        }
        defFmt[1] = 's';
        if (isApplied(comp->s)) {       // Only one Part of the ip array should be used.
          defFmt[1] = 'd';
          sprintf(buff+strlen(buff), fmt, val[comp->s]);
        } else
          sprintf(buff+strlen(buff), fmt, val.toString().c_str());
        break;
      }

      case DT_MAC: {
        if (isApplied(comp->s)) {       // Only one Part of the ip array should be used.
          defFmt[1] = 'd';
          sprintf(buff+strlen(buff), fmt, PG_upd.wifi.fixed.mac[comp->s]);
        } else {
          sprintf(buff+strlen(buff), "%02X:%02X:%02X:%02X:%02X:%02X",
              PG_upd.wifi.fixed.mac[0],
              PG_upd.wifi.fixed.mac[1],
              PG_upd.wifi.fixed.mac[2],
              PG_upd.wifi.fixed.mac[3],
              PG_upd.wifi.fixed.mac[4],
              PG_upd.wifi.fixed.mac[5]
          );
        }
        break;
      }

      //****  Following wifi values do changed while wifi is connected  ****
      case DT_RSSI: {
        if (!PG_upd.wifi.fresh() && !PG_upd.wifi.newRSSI() ) {
          idx++; continue;
        }
        defFmt[1] = 'd';
        sprintf(buff+strlen(buff), fmt, PG_upd.wifi.curr.rssi);
        break;
      }

      case DT_QUAL: {
        if (!PG_upd.wifi.fresh() && !PG_upd.wifi.newRSSI()) {
          idx++; continue;
        }
        defFmt[1] = 'd';
        float Q = 0.0;

        if (PG_upd.wifi.curr.rssi < 0)    // rssi is zero if wifi is not connected!
          Q = (-100 / -50) * (constrain(PG_upd.wifi.curr.rssi, -100, -50) + 100);

        /*
          RSSI level less than -80 dBm may not be usable, depending on noise.
          In general, the following correlation can be applied:
            RSSI ≥ -50 dBm => quality = 100%
            RSSI ≤ -100 dBm => quality = 0%
            -100 dBm ≤ RSSI ≤ -50 dBm => quality ~= 2 x (RSSI + 100)
        */
        if (isApplied(comp->s)) {    // use a range instaed of %
            // log_d("   Q rng  fmt=%s", fmt);//###
          int   r = comp->e - comp->s + 1;
          float v = constrain(Q * r / 100, comp->s, comp->e);
          sprintf(buff+strlen(buff), fmt, (int)(v + comp->s));
        } else {
            // log_d("   Q prz  fmt=%s", fmt);//###
          sprintf(buff+strlen(buff), fmt, (int)Q);
        }
        break;
      }

    }   // switch cType

    if (isTxt) strcat(buff, "\"");

    // send the command to the Nextion
    NEX_sendCommand(buff);

    idx++;
  }   // while( wifi-component )
}   // Page_updateWifi()


// called from onMqttMessage() with received topic and payload
void Page_updateMQTT(char* topic, char* payload, const size_t& len) {
  // name...="value..."\0
  char buff[NEX_MAX_NAMELEN + 2 + NEX_MAX_TEXTLEN + 2] = {0}; // buffer for the command to set the value of the component
  char compTopic[256] = {0};    //### can we use the buffer above (check the size by MAX(buffer or topic size))

  // Find all components with the given topic
  for (int idx = PG_upd.mqttStartIdx; idx < PG_upd.compCount; idx++) {
    if (PG_upd.topicEqual(idx, topic, compTopic)) {
      const comp_t* comp = &PG_upd.compList[idx];

      PG_upd.getCompFullname(comp, buff);
      bool isTxt = strstr(buff, ".txt");
      strcat(buff, "=");
      if (isTxt) strcat(buff, "\"");    // if component is text type, sourond the value by '"'

      //### define a format string depending on the component- and the payloadtype
      //### sprintf(buff+strlen(buff), comp->ptrFmt, payload);
      // sprintf(buff+strlen(buff), "%s", payload);
      size_t bp = strlen(buff);
      strncpy(buff+bp, payload, len);
      (buff+bp+len)[0] = '\0';

      if (isTxt) strcat(buff, "\"");

      // send the command to the Nextion
      NEX_sendCommand(buff);
    }    // if (topicEqual)
  }    // for (int idx
}    // Page_updateMQTT()
