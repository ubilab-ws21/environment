
// Serial Speed and DEBUG option
// #define SERIAL_SPEED 9600
#define SERIAL_SPEED 115200
// #define DEBUG_DEEP
#define SENT_LIFENESS_TO_CLIENTS


#define VERSION "0.1"

#define SEND_INFO_ON_CLIENT_CONNECT
#define SENT_LIFENESS_TO_CLIENTS

#define USE_SERIAL
#define CMD_OVER_SERIAL

// Default values
#define STANDARD_UDP_PORT 54323
#define STANDARD_TCP_PORT 54321

// Location time difference between us (Freiburg, Germany) and NTP time 
#define LOCATION_TIME_OFFSET 3600//7200 // 2 hours or (2*60*60)

#define MDNS_UPDATE_INTERVAL 30000
#define TCP_UPDATE_INTERVAL 1000
#define LIFENESS_UPDATE_INTERVAL 1000
#define MQTT_UPDATE_INTERVAL 5000

// We allow a max of 3 tcp clients for performance reasons
#define MAX_CLIENTS 3

const char * LOG_FILE = "/log.txt";

const char LOG_PREFIX_SERIAL[] = "";
const char LOG_PREFIX[] = "Info:";

#define COMMAND_MAX_SIZE 500


// Communication commands
#define CMD_RESTART "restart"
#define CMD_FACTORY_RESET "factoryReset"
#define CMD_BASIC_RESET "basicReset"
#define CMD_INFO "info"
#define CMD_MDNS "mdns"
#define CMD_NTP "ntp"
#define CMD_ADD_WIFI "addWifi"
#define CMD_REMOVE_WIFI "delWifi"
#define CMD_MQTT_SERVER "mqttServer"
#define CMD_TIME_SERVER "timeServer"
#define CMD_LOG_LEVEL "log"
#define CMD_LED_COUNT "ledCount"

#define METHOD_TRIGGER "trigger"
#define TRIGGER_POWER "power"
#define TRIGGER_GLOBES "globes"
#define TRIGGER_BLINK "blink"
#define TRIGGER_RGB "rgb"
#define TRIGGER_PATTERN "pattern"
#define TRIGGER_BRIGHTNESS "brightness"
#define TRIGGER_BRIGHTNESS_ADJUST "brightnessadjust"
#define TRIGGER_PATTERN_ADJUST "patternadjust"



#define LOG_LEVEL_ALL "all"
#define LOG_LEVEL_DEBUG "debug"
#define LOG_LEVEL_INFO "info"
#define LOG_LEVEL_WARNING "warning"
#define LOG_LEVEL_ERROR "error"


#define MQTT_TOPIC_BASE "2"
#define MQTT_TOPIC_SEPARATOR '/'
#define MQTT_TOPIC_ACTION "ledstrip"
#define MQTT_TOPIC_CMD "cmd"
#define MQTT_TOPIC_INFO "info"

#define TRUE_STRING "true"
#define FALSE_STRING "false"


const int MAX_MQTT_PUB_TOPIC = sizeof(MQTT_TOPIC_BASE) + sizeof(MQTT_TOPIC_ACTION) + 4*sizeof(MQTT_TOPIC_SEPARATOR) + 2;
#define MAX_MQTT_TOPIC_LEN MAX_MQTT_PUB_TOPIC+25

template < typename TOut, typename TIn >
TOut round2( TIn value ) {
   return static_cast<TOut>((int)(value * 100 + 0.5) / 100.0);
}


#ifdef LEDSTRIP_MATRICE
//DATA_PIN  4 → D2
//DATA_PIN  5 → D1
//DATA_PIN 12 → D6
//DATA_PIN 13 → D7
//DATA_PIN 14 → D5
#define LED_TYPE       WS2812B
#define DATA_PIN 13     // for Huzzah: Pins w/o special function:  #4,#5, #12, #13, #14; // #16 does not work :(
#define NUM_ROW 8
#define NUM_COL 32
#define NUM_LEDS ((NUM_ROW)*(NUM_COL))
// #define NUM_LEDS 256
#define MILLI_AMPS 2000
#define SUB_SUB_IP 5
#define TOPIC_PATH "2/ledstrip/timer"
#define CLIENTID "timer"
#define DEFAULT_PATTERN 11
#define FRAMES_PER_SECOND   2
#endif

#ifdef LEDSTRIP_ROOM_NORTH
#define LED_TYPE       WS2812
#define DATA_PIN 5
#define NUM_LEDS 5*LED_PER_METTER                      
#define MILLI_AMPS 10000 
#define SUB_SUB_IP 1
#define TOPIC_PATH "2/ledstrip/labroom/north"
#define CLIENTID "ledstripNorth"
#define DEFAULT_PATTERN 0
#define FRAMES_PER_SECOND   25
#endif

#ifdef LEDSTRIP_ROOM_MIDDLE
#define LED_TYPE       WS2812
#define DATA_PIN  5
#define NUM_LEDS (LED_PER_METTER>>1)+(LED_PER_METTER<<1)
#define MILLI_AMPS 5000
#define SUB_SUB_IP 2
#define TOPIC_PATH "2/ledstrip/labroom/middle"
#define CLIENTID "ledstripMiddle"
#define DEFAULT_PATTERN 0
#define FRAMES_PER_SECOND   25
#endif

#ifdef LEDSTRIP_ROOM_SOUTH
#define LED_TYPE       WS2812
#define DATA_PIN 5
#define NUM_LEDS (LED_PER_METTER>>1)+(LED_PER_METTER<<1)
#define MILLI_AMPS 5000
#define SUB_SUB_IP 3
#define TOPIC_PATH "2/ledstrip/labroom/south"
#define CLIENTID "ledstripSouth"
#define DEFAULT_PATTERN 0
#define FRAMES_PER_SECOND   25
#endif

#ifdef LEDSTRIP_SERVER
#define LED_TYPE       WS2812
#define DATA_PIN 5
#define NUM_LEDS 4*LED_PER_METTER
#define MILLI_AMPS 10000
#define SUB_SUB_IP 4
#define TOPIC_PATH "2/ledstrip/serverroom"
#define CLIENTID "ledstripServerroom"
#define DEFAULT_PATTERN 0
#define FRAMES_PER_SECOND   25
#endif

#ifdef LEDSTRIP_DOOR_SERVER
#define MILLI_AMPS 1000
#define SUB_SUB_IP 4
#define TOPIC_PATH "2/ledstrip/doorserverroom"
#define CLIENTID "ledstripDoorserverroom"
#define FRAMES_PER_SECOND   25
#endif

#define COLOR_ORDER    GRB
#define LED_PER_METTER 30

#define LED_TYPE       WS2812
#define DATA_PIN 5
#define NUM_LEDS 5*LED_PER_METTER

#define FRAMES_PER_SECOND   26

#define DEFAULT_STROBO_MS 200

#define SECONDS_PER_PALETTE 10
#define TOKEN_MAX 10

#define NUM_ROW 8
#define NUM_COL 32

#define TOPIC_TIME "op/gameTime_remain_in_sec"