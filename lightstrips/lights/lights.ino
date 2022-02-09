extern "C" {
  #include "user_interface.h"
}
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <ESP8266mDNS.h>

#include "FastLED.h"
#include "gradientPalettes.h"
FASTLED_USING_NAMESPACE

// Arduino Updater
#include <ArduinoOTA.h>

#include <PubSubClient.h>

#include "constDefine.h"
#include "src/logger/src/multiLogger.h"
#include "src/network/src/network.h"
#include "src/config/config.h"
#include "src/time/src/timeHandling.h"
#include "src/mqtt/src/mqtt.h"

void ntpSynced(unsigned int confidence);

StreamLogger serialLog((Stream*)&Serial, &timeStr, &LOG_PREFIX_SERIAL[0], ALL);

// MultiLogger logger(&streamLog, &timeStr);
// Create singleton here
MultiLogger& logger = MultiLogger::getInstance();

Configuration config;

// This version does not support an rtc
// Rtc rtc(RTC_INT);
DST germany{ true, 7, 3, 25, 7, 10, 25, 3600};
TimeHandler myTime(config.myConf.timeServer, LOCATION_TIME_OFFSET, &ntpSynced, germany, &logger);


// Open TCP port for commands
WiFiServer server(STANDARD_TCP_PORT);

MQTT mqtt;

// TCP clients and current connected state, each client is assigned a logger
WiFiClient client[MAX_CLIENTS];
bool clientConnected[MAX_CLIENTS] = {false};

StreamLogger * streamLog[MAX_CLIENTS];

// tcp stuff is send over this client 
// Init getter to point to sth, might not work otherwise
Stream * newGetter = &Serial;

// Some timer stuff s.t. things are updated regularly and not at full speed
long lifenessUpdate = millis();
long mdnsUpdate = millis();
long tcpUpdate = millis();
long mqttUpdate = millis();

// Current CPU speed
unsigned int coreFreq = 0;

// OTA Update in progress
bool updating = false;

// Command stuff send over what ever
char command[COMMAND_MAX_SIZE] = {'\0'};
StaticJsonDocument<2*COMMAND_MAX_SIZE> docRcv;
StaticJsonDocument<2*COMMAND_MAX_SIZE> docSend;
StaticJsonDocument<COMMAND_MAX_SIZE> docSample;
String response = "";

char mqttTopicPubInfo[MAX_MQTT_PUB_TOPIC] = {'\0'};
char mqttTopicPubAction[MAX_MQTT_PUB_TOPIC] = {'\0'};

unsigned long fpsCounter = millis();

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

bool autoplayEnabled = false;

uint8_t autoPlayDurationSeconds = 10;
unsigned int autoPlayTimeout = 0;

CRGB leds[1024];


CRGB blinkColor = CRGB::Black;
CRGB backBlinkColor = CRGB::Black;
CRGB solidColorMensa = CRGB::Black;
CRGB solidColorFlugplatz = CRGB::Black;
uint8_t stroboskoping = 0;
uint16_t stroboTimeMs = DEFAULT_STROBO_MS;

int globalSec; //was used to count down first, reused as communication varaible to give the time to print.
int gameDuration; //Where saved the maximum value from which we count down.


#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// Pattern call function returns void
typedef void (*Pattern)();
// Array of functions
typedef Pattern PatternList[];
typedef struct {
  Pattern pattern;
  String name;
} PatternAndName;
typedef PatternAndName PatternAndNameList[];
// List of patterns to cycle through.  Each is defined as a separate function below.


// NOTE: The timerprint pattern is only working, if the MDNS name of this
// device is set to something containing the word "timer" (e.g. serverroom_timer)
// This makes sure, that not every light is listening to the timeleft topic

// Make sure that they are in correct order
enum PATTERN_ENUM {WAVES, PALETTETEST, PRIDE, RAINBOW, RAINBOWGLITTER, CONFETTI, SINELON, JUGGLE, BPM, FIRE, SOLID, TIME, GLOBES, BLINK};
PatternAndNameList patterns = {
  { colorwaves, "colorwaves" },                  //  0
  { palettetest, "palettetest" },                //  1
  { pride, "pride" },                             //  2
  { rainbow, "rainbow" },                         //  3
  { rainbowWithGlitter, "rainbowWithGlitter" }, //  4
  { confetti, "confetti" },                       //  5
  { sinelon, "sinelon" },                         //  6
  { juggle, "juggle" },                           //  7
  { bpm, "bpm" },                                 //  8
  { fire, "fire" },                               //  9
  { showSolidColor, "solidColor" },              // 10
  { timerprint, "timerprint" },                  // 11
  { globes, "globes" },                           // 12
  { stroboskop, "stroboskop" },                   // 13
};
const uint8_t patternCount = ARRAY_SIZE(patterns);

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

bool updated = false;

const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
int brightnessIndex = 0;


/************************ SETUP *************************/
void setup() {
  config.init();  
  config.load();

  if (config.myConf.patternIndex > patternCount) config.myConf.patternIndex = 0;

  #ifdef USE_SERIAL
    // Setup serial communication
    Serial.begin(SERIAL_SPEED);
  #endif

  // bool success = rtc.init();
  // Init the logging modules
  logger.setTimeGetter(&timeStr);
  #ifdef USE_SERIAL
  // Add Serial logger
  logger.addLogger(&serialLog);
  #endif

  // Init all loggers
  logger.init();
  // init the stream logger array
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    StreamLogger * theStreamLog = new StreamLogger(NULL, &timeStr, &LOG_PREFIX[0], INFO);
    streamLog[i] = theStreamLog;
  }
  coreFreq = ESP.getCpuFreqMHz();
  logger.log(DEBUG, "%s @ firmware %s/%s", config.netConf.name, __DATE__, __TIME__);
  logger.log(DEBUG, "Core @ %u MHz", coreFreq);


  logger.log(ALL, "Connecting WLAN");

  // Init network connection
  Network::init(&config.netConf, onWifiConnect, onWifiDisconnect, false, &logger);
  // Setup OTA updating
  setupOTA();

  // Resever enough bytes for large string
  response.reserve(2*COMMAND_MAX_SIZE);

  // Set mqtt and callbacks
  mqtt.init(config.myConf.mqttServer, config.netConf.name);
  mqtt.onConnect = &onMQTTConnect;
  mqtt.onDisconnect = &onMQTTDisconnect;
  mqtt.onMessage = &mqttCallback;

  initLEDs();

  logger.log(ALL, "Setup done");
}

void initLEDs() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, config.myConf.numLEDs);
  // TODO
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(config.myConf.brightness);
  // FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);

  fill_solid(leds, config.myConf.numLEDs, CRGB(255,0,0));  
  FastLED.show();
  delay(1000);

  FastLED.show();
  //
  if (config.myConf.power && config.myConf.patternIndex == patternCount - 1) {
      fill_solid(leds, config.myConf.numLEDs, config.myConf.solidColor);  
  } else {
      fill_solid(leds, config.myConf.numLEDs, CRGB::Black);  
  }
  FastLED.show();
}

void logFunc(const char * log, ...) {
  va_list args;
  va_start(args, log);
  vsnprintf(command, COMMAND_MAX_SIZE, log, args);
  va_end(args);
  logger.log(command);
}

/************************ Loop *************************/
void loop() {
  yield();
  // Arduino OTA
  ArduinoOTA.handle();
  // We don't do anything else while updating
  if (updating) return;
  
  Network::update();
  
  // if (!Network::connected) return;

  // MQTT loop
  mqtt.update();
  
  // Handle serial requests
  #ifdef CMD_OVER_SERIAL
  if (Serial.available()) {
    handleEvent(&Serial);
  }
  #endif

  // Handle tcp requests
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    if (client[i].available() > 0) {
      handleEvent(&client[i]);
    }
  }


  // Handle tcp clients connections
  if (millis() - tcpUpdate > TCP_UPDATE_INTERVAL) {
    tcpUpdate = millis();
    // Handle disconnect
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i] and !client[i].connected()) {
        onClientDisconnect(client[i], i);
        clientConnected[i] = false;
      }
    }

    // Handle connects
    WiFiClient newClient = server.available();
    if (newClient) {
      onClientConnect(newClient);
    }
  }
  

  // Re-advertise MDNS service service every 30s 
  if (millis() - mdnsUpdate > MDNS_UPDATE_INTERVAL) {
    mdnsUpdate = millis();
    // On long time no update, avoid multiupdate
      initMDNS();

      wifi_set_sleep_type(NONE_SLEEP_T);
      // Not required, only for esp8266
    //MDNS.addService("_elec", "_tcp", STANDARD_TCP_STREAM_PORT);
  }
    
  // Update lifeness only on idle every second
  if (millis() - lifenessUpdate > LIFENESS_UPDATE_INTERVAL) {
    lifenessUpdate = millis();
    sendStatus(true, false);
  }



  yield(); // Avoid crashes on ESP8266


  if (millis()-fpsCounter > (1000 / FRAMES_PER_SECOND)) {
    fpsCounter = millis();
    updateLEDs();
  }
}

/****************************************************
 * Update LEDs depending on the pattern 
 ****************************************************/
void updateLEDs() {
  if (updated) return;
  if (!config.myConf.power) {
    fill_solid(leds, config.myConf.numLEDs, CRGB::Black);
    FastLED.show();
    updated = true;
    return;
  }
  // TODO: On solidpattern set once and return

  // logger.log("Updating pattern");
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(65535));

  EVERY_N_MILLISECONDS( 20 ) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  // change to a new cpt-city gradient palette
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  // slowly blend the current cpt-city gradient palette to the next
  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16);
  }

  // Call the current pattern function once, updating the 'leds' array
  patterns[config.myConf.patternIndex].pattern();

  FastLED.show();
}

/****************************************************
 * Send Status info over mqtt 
 ****************************************************/
void sendStatus(bool viaLogger, bool viaMQTT) {
  JsonObject obj = docSend.to<JsonObject>();
  obj.clear();

  docSend["status"] = "fine";
  docSend["ts"] = myTime.timestamp().seconds;

  response = "";
  serializeJson(docSend, response);
  if (viaLogger) logger.log("%s", response.c_str());
  if (viaMQTT) mqtt.publish(mqttTopicPubInfo, response.c_str());
}


/****************************************************
 * Send Stream info to 
 ****************************************************/
void sendDeviceInfo(Stream * sender) {
  JsonObject obj = docSend.to<JsonObject>();
  obj.clear();
  docSend["cmd"] = "info";
  docSend["type"] = F("lightstrip");
  docSend["version"] = VERSION;
  String compiled = __DATE__;
  compiled += " ";
  compiled += __TIME__;
  docSend["compiled"] = compiled;
  docSend["sys_time"] = myTime.timeStr();
  docSend["name"] = config.netConf.name;
  docSend["ip"] = Network::localIP().toString();
  docSend["mqtt_server"] = config.myConf.mqttServer;
  docSend["time_server"] = config.myConf.timeServer;
  docSend["#leds"] = config.myConf.numLEDs;
  docSend["brightness"] = config.myConf.brightness;
  String ssids = "[";
  for (size_t i = 0; i < config.netConf.numAPs; i++) {
    ssids += config.netConf.SSIDs[i];
    if (i < config.netConf.numAPs-1) ssids += ", ";
  }
  ssids += "]";
  docSend["ssids"] = ssids;
  if (not Network::ethernet) {
    docSend["ssid"] = WiFi.SSID();
    docSend["rssi"] = WiFi.RSSI();
    docSend["bssid"] = Network::getBSSID();
  }
  response = "";
  serializeJson(docSend, response);
  response = LOG_PREFIX + response;
  sender->println(response);
}


/****************************************************
 * Time getter function can be called at any time
 * Note: static function required for time getter 
 * function. e.g. for logger class
 ****************************************************/
char * timeStr() {
  return myTime.timeStr(true);
}

/****************************************************
 * If MQTT Server connection was successfull
 ****************************************************/
void onMQTTConnect() {
  logger.log("MQTT connected to %s", mqtt.ip);
  mqttSubscribe();
  // Should also publish current state
  if (mqtt.connected()) {
    snprintf(command, COMMAND_MAX_SIZE, "{ \"method\": \"message\", \"data\": \"%s active.\" }", config.netConf.name);
    mqtt.publish(mqttTopicPubInfo, command);
  }
}

/****************************************************
 * If MQTT Server disconnected
 ****************************************************/
void onMQTTDisconnect() {
  logger.log("MQTT disconnected from %s", mqtt.ip);
}


/****************************************************
 * If ESP is connected to wifi successfully
 ****************************************************/
void onWifiConnect() {
  if (not Network::apMode) {
    logger.log(ALL, "Wifi Connected %s", Network::getBSSID());
    logger.log(ALL, "IP: %s", Network::localIP().toString().c_str());
    // Reinit mdns
    initMDNS();
    // The stuff todo if we have a network connection (and hopefully internet as well)
    // myTime.updateNTPTime(true); TODO: This breaks everything with the first 5 powermeters 8-13 WTF
    myTime.updateNTPTime();
    // timeTick.attach(5,updateTime);
    mqttUpdate = millis() + MQTT_UPDATE_INTERVAL;
    if (!mqtt.connect()) logger.log(ERROR, "Cannot connect to MQTT Server %s", mqtt.ip);
  } else {
    logger.log(ALL, "Network AP Opened");
  }

  // Start the TCP server
  server.begin();
}

/****************************************************
 * If ESP disconnected from wifi
 ****************************************************/
void onWifiDisconnect() {
  logger.log(ERROR, "Wifi Disconnected");

  if (mqtt.connected()) mqtt.disconnect();
}

/****************************************************
 * If a tcp client connects.
 * We store them in list and add logger
 ****************************************************/
void onClientConnect(WiFiClient &newClient) {
  logger.log("Client with IP %s connected on port %u", newClient.remoteIP().toString().c_str(), newClient.remotePort());
  
  // Loop over all clients and look where we can store the pointer... 
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    if (!clientConnected[i]) {
      client[i] = newClient;
      client[i].setNoDelay(true);
      client[i].setTimeout(10);
      // Set connected flag
      clientConnected[i] = true;
      streamLog[i]->_type = INFO; // This might be later reset
      streamLog[i]->_stream = (Stream*)&client[i];
      logger.addLogger(streamLog[i]);
      #ifdef SEND_INFO_ON_CLIENT_CONNECT
      sendDeviceInfo((Stream*)&client[i]);
      #endif
      return;
    }
  }
  logger.log("To much clients, could not add client");
  newClient.stop();
}

/****************************************************
 * If a tcp client disconnects.
 * We must remove the logger
 ****************************************************/
void onClientDisconnect(WiFiClient &oldClient, size_t i) {
  logger.log("Client disconnected %s port %u", oldClient.remoteIP().toString().c_str(), oldClient.remotePort());
  logger.removeLogger(streamLog[i]);
  streamLog[i]->_stream = NULL;
}


/****************************************************
 * Init the MDNs name from eeprom, only the number ist
 * stored in the eeprom, construct using prefix.
 ****************************************************/
void initMDNS() {
  char * name = config.netConf.name;
  if (strlen(name) == 0) {
    logger.log(ERROR, "Sth wrong with mdns");
    strcpy(name,"lightstripX");
  }
  // Setting up MDNs with the given Name
  logger.log("MDNS Name: %s", name);
  if (!MDNS.begin(String(name).c_str())) {             // Start the mDNS responder for esp8266.local
    logger.log(ERROR, "Setting up MDNS responder!");
  }
  MDNS.addService("_light", "_tcp", STANDARD_TCP_PORT);
}

/****************************************************
 * Subscribe to the mqtt topics we want to listen
 * and build the publish topics
 ****************************************************/
void mqttSubscribe() {
  if (!mqtt.connected()) {
    logger.log(ERROR, "Should subscribe but is not connected to mqtt server");
    return;
  }

  // Build publish topics
  snprintf(&mqttTopicPubInfo[0], MAX_MQTT_TOPIC_LEN, "%s%c%s",
      MQTT_TOPIC_BASE,
      MQTT_TOPIC_SEPARATOR,
      MQTT_TOPIC_INFO);
  
  snprintf(command, COMMAND_MAX_SIZE, "%s", config.netConf.name);
  for (size_t i = 0; i < strlen(command); i++) {
    if (command[i] == '_') command[i] = '/';
  }

  // Build publish topics
  snprintf(&mqttTopicPubAction[0], MAX_MQTT_TOPIC_LEN, "%s%c%s%c%s",
      MQTT_TOPIC_BASE,
      MQTT_TOPIC_SEPARATOR,
      MQTT_TOPIC_ACTION,
      MQTT_TOPIC_SEPARATOR,
      command);

  response = mqttTopicPubAction;
  logger.log("Subscribing to: %s", response.c_str());
  mqtt.subscribe(response.c_str());

  char *ret = strstr(config.netConf.name, "timer");
  if (ret) {
    mqtt.subscribe(TOPIC_TIME);
  } else {
    // Something special for non timers
  }
}

/****************************************************
 * Callback when NTP syncs happened
 ****************************************************/
void ntpSynced(unsigned int confidence) {
  logger.log(INFO, "NTP synced with conf: %u", confidence);
}


/****************************************************
 * Setup the OTA updates progress
 ****************************************************/
// To display only full percent updates
unsigned int oldPercent = 0;
void setupOTA() {
  // Same name as mdns
  ArduinoOTA.setHostname(config.netConf.name);
  ArduinoOTA.setPassword("energy"); 
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash(2"21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    updating = true;
    Network::allowNetworkChange = false;
    logger.log("Start updating");
    // Disconnecting all connected clients
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) {
        onClientDisconnect(client[i], i);
        clientConnected[i] = false;
      }
    }
    server.stop();
    server.close();
    mqtt.disconnect();
  });

  ArduinoOTA.onEnd([]() {
    #ifdef USE_SERIAL
    logger.log("End");
    #endif
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef USE_SERIAL
    unsigned int percent = (progress / (total / 100));
    if (percent != oldPercent) {
      Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
      oldPercent = percent;
    }
    #endif
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef USE_SERIAL
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    #endif
    // No matter what happended, simply restart
    ESP.restart();
  });
  // Enable OTA
  ArduinoOTA.begin();
}
