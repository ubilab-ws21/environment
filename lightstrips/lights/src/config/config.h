/***************************************************
 Library for rtc time handling.

 Feel free to use the code as it is.

 Benjamin Völker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Informatik
 ****************************************************/

#ifndef CONFIG_h
#define CONFIG_h

#include <EEPROM.h>

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// Include for network config
#include "../network/src/network.h"
#include "../time/src/timeHandling.h"
#ifdef SENSOR_BOARD
#include "../sensorBoard/sensorBoard.h"
#endif


// Define standard wifis settings such as in the following example
// In external privateConfig.h. It will be included automatically (see config.cpp) 
// #define NUM_STANDARD_WIFIS 2
// const char * MY_STANDARD_WIFIS[] = {
//     "SSID1",  "PWD1",
//     "SSID2",  "PWD2"
// };


// If you cahange any of these values, the config on all devices will be bricked
#define MAX_STRING_LEN 25
#define MAX_IP_LEN 17
#define MAX_DNS_LEN 25
#define MAX_NAME_LEN MAX_STRING_LEN

#define NAME_START_ADDRESS 0

#define NO_SERVER "-"



#ifdef SENSOR_BOARD
# define EEPROM_SIZE_SENSOR sizeof(SensorBoardConfiguration)
#else
# define EEPROM_SIZE_SENSOR 0
#endif

#define EEPROM_SIZE (sizeof(NetworkConf)+sizeof(MeterConfiguration)+2)

#include "FastLED.h"

typedef struct __attribute__((__packed__)) C_RGB {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
} C_RGB_t;

// packed required to store in EEEPROM efficiently
struct __attribute__((__packed__)) MeterConfiguration {
  char mqttServer[MAX_DNS_LEN] = {'\0'};    // MQTT Server DNS name
  char timeServer[MAX_DNS_LEN] = {'\0'};    // Time Server DNS name
  uint8_t brightness = 0;
  CRGB solidColor;
  CRGB backSolidColor;
  bool power = true;
  uint16_t numLEDs = 256;
  uint8_t patternIndex = 0;
}; 

class Configuration {
  public:
    Configuration();
    void init();
    void load();
    void store();
    // bool loadStoreTo(uint32_t address, bool store, uint8_t * data, size_t size);
    void makeDefault(bool resetName=true);

    void setName(char * newName);

    int addWiFi(char * ssid, char * pwd);
    bool removeWiFi(char * ssid);
    void loadWiFi();
    void storeWiFi();
    void setMQTTServerAddress(char * serverAddress);
    void setStreamServerAddress(char * serverAddress);
    void setTimeServerAddress(char * serverAddress);

    NetworkConf netConf;
    MeterConfiguration myConf;

  private:
    void storeMyConf();
};

#endif
