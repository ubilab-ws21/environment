/****************************************************
 * A request happended, handle it
 ****************************************************/
void handleEvent(Stream * getter) {
  if (!getter->available()) return;
  getter->readStringUntil('\n').toCharArray(command,COMMAND_MAX_SIZE);
  // Be backwards compatible to "?" command all the time
  if (command[0] == '?') {
    getter->println(F("Info:Setup done"));
    return;
  }
  // This is just a keepalive msg
  if (command[0] == '!') {
    return;
  }
  #ifdef DEBUG_DEEP
  logger.log(INFO, command);
  #endif

  newGetter = getter;

  response = "";

  JsonObject obj = docSend.to<JsonObject>();
  obj.clear();

  if (parseCommand()) {
    handleJSON();

    JsonObject object = docSend.as<JsonObject>();
    if (object.size()) {
      // NOTE: This flush causes socket broke pipe... WTF
      // getter->flush();
      response = "";
      serializeJson(docSend, response);
      response = LOG_PREFIX + response;
      getter->println(response.c_str());

      #ifdef SERIAL_LOGGER
        if ((Stream*)&Serial != getter) {
          serialLog.log(response.c_str());
        }
      #endif
      // This will be too long for the logger
      // logger.log(response.c_str());
    }
  }
  response = "";
  command[0] = '\0';
}

/****************************************************
 * Decode JSON command from string to json dictionary
 ****************************************************/
bool parseCommand() {
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(docRcv, command);
  
  // Test if parsing succeeds.
  if (error) {
    // Remove all unallowed json characters to prevent error 
    uint32_t len = strlen(command);
    if (len > 30) len = 30;
    for (size_t i = 0; i < len; i++) {
      if (command[i] == '\r' || command[i] == '\n' || command[i] == '"' || command[i] == '}' || command[i] == '{') command[i] = '_';
    }
    logger.log(ERROR, "deserializeJson() failed: %.30s", &command[0]);
    return false;
  }
  //docSend.clear();
  return true;
}

/****************************************************
 * Handle a command in form of a JSON dict
 ****************************************************/
void handleJSON() {
  // All commands look like the following:
  // {"cmd":"commandName", "payload":{<possible data>}}
  // e.g. mdns

  const char* cmd = docRcv["cmd"];
  const char* method = docRcv["method"];
  JsonObject root = docRcv.as<JsonObject>();
  if (cmd == nullptr && method == nullptr ) {
    docSend["msg"] = "Syntax error: \"{\"cmd\":<commandName>}\"";
    return;
  }
  if (cmd != nullptr) {
    /*********************** LOG LEVEL COMMAND ****************************/
    if (strcmp(cmd, CMD_LOG_LEVEL) == 0) {
      docSend["error"] = true;
      const char* level = root["level"];
      LogType newLogType = LogType::DEBUG;
      if (level == nullptr) {
        docSend["msg"] = "Not a valid Log level cmd, level missing";
        return;
      }
      if (strcmp(level, LOG_LEVEL_ALL) == 0) {
        newLogType = LogType::ALL;
      } else if (strcmp(level, LOG_LEVEL_DEBUG) == 0) {
        newLogType = LogType::DEBUG;
      } else if (strcmp(level, LOG_LEVEL_INFO) == 0) {
        newLogType = LogType::INFO;
      } else if (strcmp(level, LOG_LEVEL_WARNING) == 0) {
        newLogType = LogType::WARNING;
      } else if (strcmp(level, LOG_LEVEL_ERROR) == 0) {
        newLogType = LogType::ERROR;
      } else {
        response = "Unknown log level: ";
        response += level;
        docSend["msg"] = response;
        return;
      }
      int found = -2;
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (newGetter == (Stream*)&client[i]) {
          found = i;
          streamLog[i]->_type = newLogType;
          streamLog[i]->setLogType(newLogType);
        }
      }
      #ifdef SERIAL_LOGGER
        if (newGetter == (Stream*)&Serial) {
          found = -1;
          serialLog._type = newLogType;
          serialLog.setLogType(newLogType);
        }
      #endif
      if (found <= -2) {
        docSend["msg"] = "getter not related to logger";
        return;
      }
      response = "Log Level set to: ";
      response += level;
      docSend["msg"] = response;
      docSend["error"] = false;
    }

    /*********************** RESTART COMMAND ****************************/
    // e.g. {"cmd":"restart"}
    else if (strcmp(cmd, CMD_RESTART) == 0) {
      ESP.restart();
    }

    /*********************** factoryReset COMMAND ****************************/
    // e.g. {"cmd":"factoryReset"}
    else if (strcmp(cmd, CMD_FACTORY_RESET) == 0) {
      config.makeDefault();
      config.store();
      ESP.restart();
    }

    /*********************** basicReset COMMAND ****************************/
    // e.g. {"cmd":"basicReset"}
    else if (strcmp(cmd, CMD_BASIC_RESET) == 0) {
      config.makeDefault(false);// Do not reset name
      config.store();
      ESP.restart();
    }

    /*********************** INFO COMMAND ****************************/
    // e.g. {"cmd":"info"}
    else if (strcmp(cmd, CMD_INFO) == 0) {
      sendDeviceInfo(newGetter);
      // It is already sent, prevent sending again
      JsonObject obj = docSend.to<JsonObject>();
      obj.clear();
    }

    /*********************** MDNS COMMAND ****************************/
    // e.g. {"cmd":"mdns", "payload":{"name":"newName"}}
    else if (strcmp(cmd, CMD_MDNS) == 0) {
      docSend["error"] = true;
      const char* newName = docRcv["payload"]["name"];
      if (newName == nullptr) {
        docSend["msg"] = F("MDNS name required in payload with key name");
        return;
      }
      if (strlen(newName) < MAX_NAME_LEN) {
        config.setName((char * )newName);
      } else {
        response = F("MDNS name too long, only string of size ");
        response += MAX_NAME_LEN;
        response += F(" allowed");
        docSend["msg"] = response;
        return;
      }
      char * name = config.netConf.name;
      response = F("Set MDNS name to: ");
      response += name;
      //docSend["msg"] = snprintf( %s", name);
      docSend["msg"] = response;
      docSend["mdns_name"] = name;
      docSend["error"] = false;
      initMDNS();
      // Resubscribe to MQTT
      mqttSubscribe();
    }

    /*********************** MQTT Server COMMAND ****************************/
    // e.g. {"cmd":"mqttServer", "payload":{"server":"<ServerAddress>"}}
    else if (strcmp(cmd, CMD_MQTT_SERVER) == 0) {
      docSend["error"] = true;
      const char* newServer = docRcv["payload"]["server"];
      if (newServer == nullptr) {
        docSend["msg"] = F("MQTTServer address required in payload with key server");
        return;
      }
      if (strlen(newServer) < MAX_IP_LEN) {
        config.setMQTTServerAddress((char * )newServer);
      } else {
        response = F("MQTTServer address too long, only string of size ");
        response += MAX_IP_LEN;
        response += F(" allowed");
        docSend["msg"] = response;
        return;
      }
      char * address = config.myConf.mqttServer;
      response = F("Set MQTTServer address to: ");
      response += address;
      //docSend["msg"] = snprintf( %s", name);
      docSend["msg"] = response;
      docSend["mqtt_server"] = address;
      docSend["error"] = false;
      mqtt.disconnect();
      mqtt.init(config.myConf.mqttServer, config.netConf.name);
      mqtt.connect();
    }
    /*********************** Time Server COMMAND ****************************/
    // e.g. {"cmd":"timeServer", "payload":{"server":"<ServerAddress>"}}
    else if (strcmp(cmd, CMD_TIME_SERVER) == 0) {
      docSend["error"] = true;
      const char* newServer = docRcv["payload"]["server"];
      if (newServer == nullptr) {
        docSend["msg"] = F("StreamServer address required in payload with key server");
        return;
      }
      if (strlen(newServer) < MAX_DNS_LEN) {
        config.setTimeServerAddress((char * )newServer);
      } else {
        response = F("TimeServer address too long, only string of size ");
        response += MAX_DNS_LEN;
        response += F(" allowed");
        docSend["msg"] = response;
        return;
      }
      char * address = config.myConf.timeServer;
      response = F("Set TimeServer address to: ");
      response += address;
      //docSend["msg"] = snprintf( %s", name);
      docSend["msg"] = response;
      docSend["time_server"] = address;
      docSend["error"] = false;
      myTime.updateNTPTime();
    }
    /*********************** ADD WIFI COMMAND ****************************/
    // e.g. {"cmd":"addWifi", "payload":{"ssid":"ssidName","pwd":"pwdName"}}
    else if (strcmp(cmd, CMD_ADD_WIFI) == 0) {
      docSend["error"] = true;
      const char* newSSID = docRcv["payload"]["ssid"];
      const char* newPWD = docRcv["payload"]["pwd"];
      if (newSSID == nullptr or newPWD == nullptr) {
        docSend["msg"] = F("WiFi SSID and PWD required, for open networks, fill empty pwd");
        return;
      }
      int success = 0;
      if (strlen(newSSID) < MAX_NETWORK_LEN and strlen(newPWD) < MAX_PWD_LEN) {
        success = config.addWiFi((char * )newSSID, (char * )newPWD);
      } else {
        response = F("SSID or PWD too long, max: ");
        response += MAX_NETWORK_LEN;
        response += F(", ");
        response += MAX_PWD_LEN;
        docSend["msg"] = response;
        return;
      }
      if (success == 1)  {
        char * name = config.netConf.SSIDs[config.netConf.numAPs-1];
        char * pwd = config.netConf.PWDs[config.netConf.numAPs-1];
        response = F("New Ap, SSID: ");
        response += name;
        response += F(", PW: ");
        response += pwd;
        //docSend["msg"] = snprintf( %s", name);
        docSend["ssid"] = name;
        docSend["pwd"] = pwd;
        docSend["error"] = false;
      } else if (success == -1) {
        response = F("Wifi AP ");
        response += newSSID;
        response += F(" already in list");
      } else {
        response = F("MAX # APs reached, need to delete first");
      }

      docSend["msg"] = response;
      String ssids = "[";
      for (size_t i = 0; i < config.netConf.numAPs; i++) {
        ssids += config.netConf.SSIDs[i];
        if (i < config.netConf.numAPs-1) ssids += ", ";
      }
      ssids += "]";
      docSend["ssids"] = ssids;
    }

    /*********************** DEl WIFI COMMAND ****************************/
    // e.g. {"cmd":"delWifi", "payload":{"ssid":"ssidName"}}
    else if (strcmp(cmd, CMD_REMOVE_WIFI) == 0) {
      docSend["error"] = true;
      const char* newSSID = docRcv["payload"]["ssid"];
      if (newSSID == nullptr) {
        docSend["msg"] = F("Required SSID to remove");
        return;
      }
      bool success = false;
      if (strlen(newSSID) < MAX_NETWORK_LEN) {
        success = config.removeWiFi((char * )newSSID);
      } else {
        response = F("SSID too long, max: ");
        response += MAX_NETWORK_LEN;
        docSend["msg"] = response;
        return;
      }
      if (success)  {
        response = F("Removed SSID: ");
        response += newSSID;
        docSend["error"] = false;
      } else {
        response = F("SSID ");
        response += newSSID;
        response += F(" not found");
      }
      docSend["msg"] = response;
      String ssids = "[";
      for (size_t i = 0; i < config.netConf.numAPs; i++) {
        ssids += config.netConf.SSIDs[i];
        if (i < config.netConf.numAPs-1) ssids += ", ";
      }
      ssids += "]";
      docSend["ssids"] = ssids;
    }

    /*********************** NTP COMMAND ****************************/
    // e.g. {"cmd":"ntp"}
    else if (strcmp(cmd, CMD_NTP) == 0) {
      docSend["error"] = true;
      const char* newSSID = docRcv["payload"]["ssid"];
      if (myTime.updateNTPTime()) {
        docSend["msg"] = "Time synced";
        docSend["error"] = false;
      } else {
        docSend["msg"] = "Error syncing time";
        docSend["error"] = true;
      }
      char * timeStr = myTime.timeStr();
      docSend["current_time"] = timeStr;
    }

    /*********************** SET LED Count COMMAND ****************************/
    // e.g. {"cmd":"ledCount","num":150}
    else if (strcmp(cmd, CMD_LED_COUNT) == 0) {
      JsonVariant valueVariant = root["num"];
      if (valueVariant.isNull()) {
          docSend["msg"] = "Number leds \"num\" missing";
          return;
      }
      config.myConf.numLEDs = root["num"].as<uint16_t>();
      if (config.myConf.numLEDs > 1024) config.myConf.numLEDs = 1024;
      // Reinit LEDs
      FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, config.myConf.numLEDs);
      updated = false;
      config.store();
      ESP.restart();
    }

    /*********************** UNKNOWN COMMAND ****************************/
    else {
      response = "unknown command";
      docSend["msg"] = response;
      logger.log(WARNING, "Received unknown command");
    }
  
  /*********************** LED Stripe COMMANDs ****************************/
  } else if (method != nullptr) {
    if (strcmp(method, METHOD_TRIGGER) != 0) {
      docSend["msg"] = "We only listen to trigger msges";
      return;
    }

    const char* state = root["state"];
    const char* data = root["data"];
    if (state == nullptr || data == nullptr) {
      docSend["msg"] = "Syntax error: \"{\"method\":\"trigger\",\"state\":<state>,\"data\":<data>}\"";
      return;
    }
    // LEDs need an update
    updated = false;
    // Power 
    if (strcmp(state, TRIGGER_POWER) == 0) {
      bool on = true;
      if (strcmp(data, "off") == 0) on = false;
      config.myConf.power = on;
    }
    // Power 
    else if (strcmp(state, TRIGGER_GLOBES) == 0) {
      size_t dataLen = strlen(data);
      size_t index = 0;
      solidColorFlugplatz = parseColor(data, &index, dataLen);
      solidColorMensa = parseColor(data, &index, dataLen);
      JsonArray array = docSend.createNestedArray("color1");
      for (int i = 0; i < 3; i++) array.add(solidColorFlugplatz[i]);
      JsonArray array2 = docSend.createNestedArray("color2");
      for (int i = 0; i < 3; i++) array2.add(solidColorMensa[i]);
      setPattern(GLOBES);
    }
    // Blink 
    else if (strcmp(state, TRIGGER_BLINK) == 0) {
      size_t dataLen = strlen(data);
      size_t index = 0;
      stroboskoping = 1;
      stroboTimeMs = DEFAULT_STROBO_MS;
      int t = parseInt(&data[0], &index, dataLen);
      index++;
      if (t > 0) stroboTimeMs = t;
      config.myConf.solidColor = parseColor(&data[0], &index, dataLen);
      config.myConf.backSolidColor = parseColor(&data[0], &index, dataLen);
      
      JsonArray array = docSend.createNestedArray("blink1");
      for (int i = 0; i < 3; i++) array.add(config.myConf.solidColor[i]);
      JsonArray array2 = docSend.createNestedArray("blink2");
      for (int i = 0; i < 3; i++) array2.add(config.myConf.backSolidColor[i]);
      setPattern(BLINK);
    }
    else if (strcmp(state, TRIGGER_RGB) == 0) {
      size_t dataLen = strlen(data);
      size_t index = 0;
      config.myConf.solidColor = parseColor(&data[0], &index, dataLen);
      config.store();
      JsonArray array = docSend.createNestedArray("color");
      for (int i = 0; i < 3; i++) array.add(config.myConf.solidColor[i]);

      setPattern(SOLID);
    }
    else if (strcmp(state, TRIGGER_PATTERN) == 0) {
      bool found = false;
      for (size_t i = 0; i < patternCount; i++) {
        logger.log("%s - %s",data, patterns[i].name.c_str());
        if (strcmp(data, patterns[i].name.c_str()) == 0) {
          found = true;
          setPattern(i);
          break;
        }
      }
      if (not found) {
        docSend["msg"] = "Pattern not found";
      }
    }

    else if (strcmp(state, TRIGGER_BRIGHTNESS) == 0) {
      size_t dataLen = strlen(data);
      size_t index = 0;
      config.myConf.brightness = parseInt(data, &index, dataLen);
      docSend["brightness"] = config.myConf.brightness;
      FastLED.setBrightness(config.myConf.brightness);
      config.store();
    }

    else if (strcmp(state, TRIGGER_BRIGHTNESS_ADJUST) == 0) {
      bool up = true;
      if (strcmp(data, "-1") == 0) up = false;
      if (up) brightnessIndex++;
      else brightnessIndex--;

      // wrap around at the ends
      if (brightnessIndex < 0) brightnessIndex = brightnessCount - 1;
      else if (brightnessIndex >= brightnessCount) brightnessIndex = 0;
      setBrightness(brightnessMap[brightnessIndex]);

      docSend["brightness"] = config.myConf.brightness;
      FastLED.setBrightness(config.myConf.brightness);
      config.store();
    }
    else if (strcmp(state, TRIGGER_PATTERN_ADJUST) == 0) {
      bool up = true;
      if (strcmp(data, "-1") == 0) up = false;
      if (up) config.myConf.patternIndex++;
      else config.myConf.patternIndex--;

      // wrap around at the ends
      if (config.myConf.patternIndex < 0) config.myConf.patternIndex = patternCount - 1;
      if (config.myConf.patternIndex >= patternCount) config.myConf.patternIndex = 0;

      setPattern(config.myConf.patternIndex);

      docSend["pattern"] = config.myConf.patternIndex;
      config.store();
    }
  }
}

int parseInt(const char * arrPtr, size_t * s, size_t e) {
  int value = 0;
  int mult = 1;
  bool valid = false;
  if (arrPtr[*s] == '-') {
    mult = -1;
    *s = *s + 1;
  }
  while (*s < e && arrPtr[*s] >= '0' && arrPtr[*s] <= '9') {
    value = value*10 + ((uint8_t)(arrPtr[*s]) - '0');
    *s = *s+1;
    valid = true;
  }
  if (not valid) return -1;
  return value*mult;
}

CRGB parseColor(const char * arrPtr, size_t * s, size_t e) {
  uint8_t colorArr[3] = {0};
  size_t start = 0;
  for (size_t i = 0; i < 3; i++) {
    int c = parseInt(&arrPtr[*s], &start, e);
    if (c >= 0 && c <= 255) colorArr[i] = c;
    // Skip next char
    start++;
  }
  *s = *s + start;
  CRGB color = CRGB(colorArr[0], colorArr[1], colorArr[2]);
  return color;
}

/****************************************************
 * A mqtt msg was received for a specific 
 * topic we subscribed to. See mqttSubscribe function. 
 ****************************************************/
void mqttCallback(char* topic, byte* message, unsigned int length) {
  memcpy(&command[0], message, length);
  command[length] = '\0';
  logger.log("MQTT msg on topic: %s: %s", topic, command);

  // That's the time coming right in
  if (strcmp(TOPIC_TIME, topic) == 0) {

    char timeStr[11] = {'\0'};
    memcpy(&timeStr[0], message, min(10,(int)length));
    // String
    size_t i = 0;
    int mySecs = parseInt(&timeStr[0], &i, strlen(timeStr));
    if (mySecs >= 0) globalSec = mySecs;
    logger.log("Got new secs: %lu", globalSec);
    return;
  }
  // That's possibly a command
  if (!parseCommand()) {
    mqtt.publish(mqttTopicPubInfo, "Parse error");
    return;
  } 
  handleJSON();

  JsonObject object = docSend.as<JsonObject>();
  if (object.size()) {
      response = "";
      serializeJson(docSend, response);
      snprintf(command, COMMAND_MAX_SIZE, "{ \"method\": \"message\", \"data\": \"%s\" }", response.c_str());
      mqtt.publish(mqttTopicPubInfo, command);
      logger.log(response.c_str());
  }

  response = "";
  command[0] = '\0';
}

void setBrightness(uint8_t value){
  // don't wrap around at the ends
  if (value > 255) value = 255;
  else if (value < 0) value = 0;
  config.myConf.brightness = value;
  FastLED.setBrightness(config.myConf.brightness);
  config.store();
}

void setPattern(uint8_t value){
  // don't wrap around at the ends
  if (value < 0) value = 0;
  else if (value >= patternCount) value = patternCount - 1;
  config.myConf.patternIndex = value;
  config.store();
}