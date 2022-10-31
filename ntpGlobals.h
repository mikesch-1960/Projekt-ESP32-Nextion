#pragma once

// usefull?: sntp_set_sync_interval(uint32_t interval_ms)


struct {
  time_t now;           // this is the epoch
  tm timeinfo;          // the structure tm holds time information in a more convenient way
}
NTP_store;

//### used for testing purposes only
/*
  void tmpNTPsetTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  NTP_store.timeinfo.tm_year  = yr - 1900;    // Set date
  NTP_store.timeinfo.tm_mon   = month-1;
  NTP_store.timeinfo.tm_mday  = mday;
  NTP_store.timeinfo.tm_hour  = hr;           // Set time
  NTP_store.timeinfo.tm_min   = minute;
  NTP_store.timeinfo.tm_sec   = sec;
  NTP_store.timeinfo.tm_isdst = isDst;        // 1 or 0

  time_t t = mktime(&NTP_store.timeinfo);
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}
*/