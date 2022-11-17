
#include <PubSubClient.h>

struct {
  char  server[40];
  char  port[6];
  char  username[40];
  char  password[40];
} mqtt;


/*
accuweather.0.Current.Temperature
accuweather.0.Current.Pressure
accuweather.0.Current.WeatherText
accuweather.0.Current.RelativeHumidity
*/