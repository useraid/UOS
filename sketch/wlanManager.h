// WLAN Manager

// External Libraries
#include <WiFiManager.h>
#include <ESP8266WiFiMulti.h>

// WiFi state variables
bool wifiEnabled = false;
bool apEnabled = false;
bool captivePortalEnabled = false;
bool isDirty = true; // Flag to indicate display refresh