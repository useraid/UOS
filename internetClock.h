// Internet Clock
// External Libraries
#include <WiFiUdp.h>
#include <NTPClient.h>     
#include <TimeLib.h>

// Variables
char Time[] = "  :  :  ";
char Date[] = "  -  -20  ";
byte last_second, second_, minute_, hour_, wday, day_, month_, year_; 