// Battery Monitoring

// Variables
const int analogInPin = A0;      // Analog input pin
int sensorValue;           // Analog output of the sensor
const float calibration = 0.27;  // Check Battery voltage using a multimeter & add/subtract the value
int bat_percentage;
unsigned long prevTimeBat = 0;
const unsigned long pollingBat = 1000;