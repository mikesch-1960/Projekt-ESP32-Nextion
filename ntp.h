#pragma once

#include "ntpGlobals.h"
#include "time.h"


void NTP_begin(char* server, char* tzCfg) {
  // https://en.cppreference.com/w/c/locale/setlocale
  // setlocale(LC_TIME, "de_DE");     // does'nt work :-(
  setlocale(LC_TIME, "de_DE.utf8");
  configTime(0, 0, server); // 0, 0 because we will use TZ in the next line
  setenv("TZ", tzCfg, 1); // Set environment variable with your time zone
  tzset();
}


void NTP_updTimeinfo() {
  time(&NTP_store.now); // read the current time
  localtime_r(&NTP_store.now, &NTP_store.timeinfo); // update the structure tm with the current time
}


// format description: https://help.gnome.org/users/gthumb/unstable/gthumb-date-formats.html.de
// or https://cplusplus.com/reference/ctime/strftime/
// NOTE: NTP_updTimeinfo();  must be called before, to get the actual time!
size_t NTP_asString(char fmt[], char res[], size_t resLen) {
  strncpy(res, "?\0\0\0", resLen-1);
  size_t r = strftime(res, resLen-1, fmt, &NTP_store.timeinfo);

  return strlen(res);
}
