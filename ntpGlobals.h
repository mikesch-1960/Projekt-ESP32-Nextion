#pragma once

struct {
  time_t now;           // this is the epoch
  tm timeinfo;          // the structure tm holds time information in a more convenient way
}
NTP;

//### used for testing purposes only
/*
  void tmpNTPsetTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  NTP.timeinfo.tm_year  = yr - 1900;    // Set date
  NTP.timeinfo.tm_mon   = month-1;
  NTP.timeinfo.tm_mday  = mday;
  NTP.timeinfo.tm_hour  = hr;           // Set time
  NTP.timeinfo.tm_min   = minute;
  NTP.timeinfo.tm_sec   = sec;
  NTP.timeinfo.tm_isdst = isDst;        // 1 or 0

  time_t t = mktime(&NTP.timeinfo);
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}
*/