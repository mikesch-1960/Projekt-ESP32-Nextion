/*
  DiesesProgramm soll auf möglichst einfache weise eine Verbindung zu einem Nextion Display herstellen und Daten aus unterschiedlichen Systemen
  die der ESP bedienen kann darstellen und/oder steuern. Bis zu diesem Zeitpunkt (10.2022) habe ich nur das im NSPanel(EU) von Sonoff eingebaute
  Nextion Display. Für mein "Smart Home" benutze ich den ioBroker. Um darin Daten zu lesen und zu steuern kann über einen MQTT Adapter oder den
  REST-API Adapter auf den ioBroker zugegriffen werden.
  Alles was in diesem Programm entwickelt wird, wird zwar mit einer möglicht großen allgemeinen Gültigkeit getan ist aber nur auf den mir zur
  Verfügung stehenden Hardware getestet.
  Die Funktionen sind nicht in Klassen gekapselt, weil ich dabei auf große Probleme gestoßen bin da immer wieder Abhängigkeiten der einzelnen
  Klassen auftraten.
  Die zugehörige HMI Datei habe ich mit dem Nextion Editor V1.63.3 (https://nextion.tech/editor_guide/) erstellt. Das Display kann mit dem Simulator
  des Editors ohne vorhandenes Display voll getestet werden, wenn die zweite serielle Schnittstelle des ESP über einen FTDI Adapter mit einen zweiten
  USB Port des PCs verbunden wird. Im Nextion Editor wird dann der Debuger gestartet und darin unten 'User MCU Input' gewählt. Dort muss dann noch die Com Schnittstelle und dir Baudrate eingestellt werden.

  In dieser Version: -----
  - Komunikation zwischen ESP und Display
  - Mit WifiManager und NTP
  - Die im 'page init event' gemeldeten _timeXYZ Komponenten werden automatisch beim Minuten-/oder Sekundenwechsel aktualisiert, abhängig davon ob im Format ein Sekunden Bezeichner enthalten ist.
  - _date entfernt! Es wird nur noch _timeXYZ.ext verwendet. Bei date muss das entspr. Format gesetzt werden.
  - Die im 'page init event' gemeldeten _wifiXYZ Komponenten werden automatisch aktualisiert
  - Format Makros: 'Program.s' kann im 'config event' bis zu 4 Formate (fmtTime0-3) für _time Komponenten definieren. Diese werden im 'page init event' mit #0-#3 addressiert. Die Angabe muss die einzige im param sein. Z.B. `_timeM1&#1;` use fmtTime1
  - Baudrate zum Nextion Display wurde auf die höchst mögliche gesetzt (921600), dass muss in der HMI Datei im 'Program.s' Skript auch so eingestellt sein.

  Todos: -----
  - LongTouch und swipe im ESP steuern
  - dim Werte aus dem Nextion im EEPROM merken damit diese bei Veränderungen einen Neustart des Screens überleben
  - Wenn Wifi ausfällt: Reconnect ohne Config Portal wenn schon mal ok war. Wann Config Portal?
  - Testen ob die zusätzl. Parameter des WiFiMgr automatisch gespeichert werden und einen Restart überleben
  - Async WifiManager

  Ideen: -----
  - Im Screen sendxy aktivieren und hier die dadurch gesendeten touch Ereignisse für longPress und swipe auswerten.
    Hier wird dann ein Kommando abgesetzt, das entsprechende globale Variablen im Screen setzt, die dort dann beim release ausgewertet werden können.
  - Die Liste der zu aktualisierenden Komponenten auch während die Seite angezeigt wird ändern können (void Page_add/removeComps(char* pgComplist)
  - Bei den Parametern zu den _wifi Koponenten eine 'echte' Formatangabe wie '%R dBa' machen können.

  Interessante Infos
  zu WiFI:
    connect to wifi (absolutly nonblocking) via FreeRTOS
    https://www.youtube.com/watch?v=YSGPcm-qxDA

  zum Nextion Display:
  Official Nextion page:  https://nextion.tech/instruction-set/
  Unofficial Nextion/TJC User Forum:  https://unofficialnextion.com/
  Tips, Tricks, and Traps for Nextion Devices
    https://github.com/krizkontrolz/Home-Assistant-nextion_handler/tree/main/Tips_and_Tricks
  A Full Instruction Set (with hidden ones):
    https://github.com/UNUF/nxt-doc/blob/main/Protocols/Full%20Instruction%20Set.md#readme


  MQTT Referenz:
  https://github.com/marvinroger/async-mqtt-client/blob/develop/examples/FullyFeatured-ESP32/FullyFeatured-ESP32.ino

  Collection of links for Nexion displays:
  sleep mode erklärt
  https://nextion.tech/2021/08/02/the-sunday-blog-energy-efficient-design-with-nextion-hmi-portable-and-wearable-designs/
  all commands and more
  https://nextion.tech/instruction-set/#s3

*/

#define strHas(str, what)     strstr(str, what)!=nullptr

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wifiMgr;
WiFiManagerParameter wmcp_ntp_server("ntpserver", "NTP server:", "0.europe.pool.ntp.org", 51);
WiFiManagerParameter wmcp_ntp_tz("ntptimezone", "NTP timezone config:", "CET-1CEST,M3.5.0/02,M10.5.0/03", 61);

WiFiManagerParameter wmcp_nex_dimHi("nexDimHigh:", "Nextion:\nDim norm.:", "60", 4);
// wmcp_nex_dimHi.setValue("80", 4);//### how to change the value in case config from Nextion


/*
  NSPanel specific IO-Pins: https://www.kickstarter.com/projects/sonoffnspanel/sonoff-nspanel-smart-scene-wall-switch/posts/3328417
    Pin   IO              Corresponding function
    7    SENSOR_CAPN     NTC, temperature sensing
    16    GPIO27          KEY2
    17    MTMS            KEY1
    23    GPIO0           for flashing firmware
    41    U0TXD           ESP_TX, connect to ESP
    40    U0RXD           ESP_RX
    24    GPIO4           T_RST, screen reset, high valid
    25    GPIO16          NEX_TX, connect to screen
    27    GPIO17          NEX_RX, connect to screen
    38    GPIO19          RELAY2, secound relay of the NSPanel
    39    GPIO22          RELAY1, first relay of the NSPanel
    42    GPIO24          BUZZER, control passive buzzer
*/
#define   NEX_SER           Serial2
#define   ESP_SER           Serial
#define   PIN_ESP_RESET     23        // GPIO bound to the EN/RES pin to progarmmaticaly restart the ESP


#include "ntpGlobals.h"
#include "ntp.h"

#include "nexGlobals.h"
#include "nex.h"


void saveParamsCallback () {
  Serial.println("Get Params:");
  Serial.print(wmcp_ntp_server.getID());
  Serial.print(" : ");
  Serial.println(wmcp_ntp_server.getValue());
}


void setup() {
  pinMode(PIN_ESP_RESET, OUTPUT);
  digitalWrite(PIN_ESP_RESET, HIGH);

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  ESP_SER.begin(115200);
  ESP_SER.println();  ESP_SER.println();

  ESP_SER.println("[ESP] NEX setup...");
  NEX_begin(921600 /*### 0=detect */);    // use the fastest possible value! Must be set in Program.s with baud=92160

  /* https://github.com/tzapu/WiFiManager/blob/master/examples/Super/OnDemandConfigPortal/OnDemandConfigPortal.ino */

  // wifiMgr.resetSettings();     // reset settings - wipe credentials for testing
  wifiMgr.setDebugOutput(false);
  wifiMgr.setConfigPortalBlocking(false);
  wifiMgr.addParameter(&wmcp_ntp_server);
  wifiMgr.addParameter(&wmcp_ntp_tz);
  wifiMgr.setSaveParamsCallback(saveParamsCallback);

/*
  #define WM_MDNS // use MDNS
  #define WM_DEBUG_PORT // set debug port eg. Serial1
  #define WM_DEBUG_LEVEL // set debug level 0-4
  DEBUG_ERROR
  DEBUG_NOTIFY
  DEBUG_VERBOSE
  DEBUG_DEV
  DEBUG_MAX
  setShowDnsFields
  getConfigPortalActive
  getWebPortalActive
  getWiFiPass
  getWiFiSSID
  getLastConxResult
  getWLStatusString
  .setSTAStaticIPConfig(IPAddress(192,168,0,99), IPAddress(192,168,0,1), IPAddress(255,255,255,0)); // optional DNS 4th argument
  .setConfigPortalTimeout(60);  // seconds
  wifiMgr.setShowStaticFields(true);
  //to preload autoconnect with credentials
  wifiMgr.preloadWiFi("ssid","password");
  wifiMgr.reboot();
  // sets timeout for which to attempt connecting, useful if you get a lot of failed connects
  void          setConnectTimeout(unsigned long seconds);
  sets number of retries for autoconnect, force retry after wait failure exit
  void          setConnectRetries(uint8_t numRetries); // default 1
  debug output the softap config
  void          debugSoftAPConfig();
  String        getWiFiSSID(bool persistent = true);
  String        getConfigPortalSSID();
  int           getRSSIasQuality(int RSSI);
*/

  // set dark mode via invert class
  wifiMgr.setDarkMode(true);
  wifiMgr.setWiFiAutoReconnect(true);
  wifiMgr.setConnectTimeout(5); // seconds
  wifiMgr.setHostname("NextionNS");

  // automatically connect using saved credentials if they exist.
  // If connection fails it starts an access point with the specified name
  //### if (wifiMgr.autoConnect("Nextion_AP", "michi")) {
  if (wifiMgr.autoConnect("Nextion_AP")) {
    Serial.printf(
      "[WM] connected to WiFi %s (%s) %ddBa (%d%%)...\n",
        wifiMgr.getWiFiSSID().c_str(), WiFi.localIP().toString().c_str(),
        WiFi.RSSI(), wifiMgr.getRSSIasQuality(WiFi.RSSI())
    );
    NTP_begin((char*)wmcp_ntp_server.getValue(), (char*)wmcp_ntp_tz.getValue());
  }
  else {
    Serial.printf("[WM] Configportal running... %s \n", wifiMgr.getConfigPortalSSID().c_str());  //### show AP and IP
    //### Inform Nextion
  }

  ESP_SER.println("[ESP] Setup Done! Entering loop...");
}


uint8_t payload[MAXLEN_COMPONENT_LIST+1+3+3] = {0}; // plus space for '\0', message-head and message-tail

void loop() {
  wifiMgr.process();


  if (NEX_readPayload(payload, MAXLEN_COMPONENT_LIST, 10)) {
        NEX_handleMsg(payload);
  }   // Nextion Serial available


  if (ESP_SER.available()) {
    ESP_SER.setTimeout(200);

    int len = ESP_SER.readBytes((char *)payload, sizeof(payload)-1);
    ESP_SER.printf("[ESP] got message: '%s' len=%d\n", payload, len);

    if (len > 2) {   // send ESP message as command to nextion
      payload[len] = '\0';
      while (len > 0 && (uint8_t)payload[len] <= 0x20)     payload[len--] = '\0';   // remove linebreaks and spaces from the end
      NEX_sendCommand((const char *)payload, 200);
    } else
    {
      // on any input that is only one character do
      if ((char)payload[0]=='B')   wifiMgr.reboot();      // reboot ESP
      else
      if ((char)payload[0]=='R')   NEX_sendCommand("rest", false);    // reboot Screen
    }
  }   // ESP Serial available


  if (PG_upd.time.next()) {
    NTP_updTimeinfo();      // update NTP time structure to get current time
    Page_updateTime();   // update time components on current page
    PG_upd.time.storeCurrent();
  }


  if (PG_upd.wifi.next()) {
    Page_updateWifi();
    PG_upd.wifi.storeCurrent();
  }
}   // loop()