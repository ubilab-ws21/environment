Environnment Lightening
=======================

# LEDstrips controller

This project is the root of the Arduino directory. It contains the Arduino project "lights" and also the required libraries.

## Summary
- [Environnment Lightening](#environnment-lightening)
- [LEDstrips controller](#ledstrips-controller)
  - [Summary](#summary)
  - [Introduction](#introduction)
  - [Internal Parameter](#internal-parameter)
  - [Configuration](#configuration)
    - [Info](#info)
    - [Log Level](#log-level)
    - [Name](#name)
    - [Restart](#restart)
    - [Reset](#reset)
    - [WiFi](#wifi)
    - [MQTT](#mqtt)
    - [LEDCount](#ledcount)
  - [Commands](#commands)
    - [power](#power)
    - [globes](#globes)
    - [blink](#blink)
    - [rgb](#rgb)
    - [pattern](#pattern)
    - [brightness](#brightness)
    - [brightnessadjust](#brightnessadjust)
    - [patternadjust](#patternadjust)
  - [Contributors](#contributors)


<a id="INTRODUCTION"></a>
## Introduction

Project has been rewritten to be a little more reliable. Each light can now be updated with the exact same code over OTA. Configurations like name and MQTT address can now be set using mqtt or TCP commands (see section [Commands](#COMMANDS)). 
The project includes code form the esp8266-fastled-mqtt" project ( https://github.com/awilhelmer/esp8266-fastled-mqtt/ ) with a lot of modifications :
* Adjustable number of LEDs, names, topics 
* Additional TCP server on port 54321 which supports logging and same JSON commands
* All commands use specific JSON format
* Adding new patterns
* OTA

The ledstrip are listening to commands or configurations inside their own mqtt topic.
- "2/ledstrip/<NAME>"
  
For the EScaperoom WS19 this is:
- "2/ledstrip/timer"
- "2/ledstrip/labroom/north"
- "2/ledstrip/labroom/middle"
- "2/ledstrip/labroom/south"
- "2/ledstrip/serverroom"
  
For the EScaperoom WS21, the lobby was added:
- "2/ledstrip/lobby"

If the hardware is altered or the memory is remapped, the modules need to be reconfigured by setting the names accordingly. If an underscore is added to the name, it is treated as '/' so to listen to topic `<xxx>/labroom/north` you need to set the name `labroom_north`

All ledstrips are giving a feedback under the topic `2/info`.

If the name includes the substring `timer`, it also listens to the topic `op/gameTime_remain_in_sec`.

Configuration and commands can be either send over mqtt:\
```mosquitto_pub -h 10.0.0.2 -t '2/ledstrips/labroom_middle' -m '{"cmd":"info"}'```

Or via TCP after connecting to the server at port `54321`.

<a id="PARAMETER"></a>
## Internal Parameter
Internal parameter like name, mqtt server, foreground-color, background-color, pattern, brightness and power are stored in flash and are kept upon re-flashes and power cycles. 

<a id="Configuration"></a>
## Configuration

Configurations follow the form ```{"cmd":<name>,[optionalParameters]}```

### Info
```
{"cmd":"info"}
```
The info command will print information about the ledstrip. As this is the most often required command, it is automatically sent to each new connected TCP client. 

### Log Level
```
{"cmd":"log","level":<logLevel>}
```
Choose between the following log levels: ```all, debug, info, warning, error```\
Each new connection is initialized with log level ```info```. Log levels only work over direct TCP connections.
```bash
Info:{"error":false,"msg":"Log Level set to: warning"}
```

### Name
```
{"cmd":"mdns","payload":{"name":"<newName>"}}
```
This command should be used to give the strip a new unique name.
It will be used to announce presence over MDNS and if no known network is found, an AP is opened using this name.

If an underscore is added to the name, the strip treats it as '/' for MQTT topics. So to listen to topic `2/ledstrips/labroom/north` you need to set the name `labroom_north`.

The strip will reboot automatically.


### Restart
```
{"cmd":"restart"}
```
Restarts the strip.

### Reset
```
{"cmd":"factoryReset"}
```
Resets everything to defaults. The default values are:
- name: `labroom_middle`
- wifi aps: empty list if no privateConfig given
- timeserver: `time.google.com`
- mqtt broker: `10.0.0.2`
- solidColor: `black`
- backSolidColor: `black`
- pattern: `0`
- numLEDs: `150`
- brightness: `64`
- power: `true`

If you want to reset everything except the name, use: ```{"cmd":"basicReset"}```

### WiFi
Add wifi AP: 
```
{"cmd":"addWifi", "payload":{"ssid":"<ssidName>","pwd":"<pwdName>"}}
```
Answer:
```bash
Info:{"error":false,"ssid":"Test","pwd":"TestPwd","msg":"New Ap, SSID: Test, PW: TestPwd","ssids":"[ubilab, Test]"}
```
Remove a wifi AP: 
```
{"cmd":"delWifi", "payload":{"ssid":"<ssidName>"}}
```
Answer:
```bash
Info:{"error":false,"msg":"Removed SSID: Test","ssids":"[ubilab]"}
```

### MQTT
Once connected to an MQTT server, the strip will listen to the topics and send status info on topic `2/info`. 
To set the MQTT server, use the command: 
```
{"cmd":"mqttServer", "payload":{"server":"<ServerAddress>"}}
```
Currently, only the MQTT standard server port ```1883``` is supported. 
```bash
Info:[I]03/02 11:38:42: MQTT connected to 192.168.0.13
Info:[I]03/02 11:38:42: Subscribing to: 2/ledstrip/<name>
Info:{"error":false,"msg":"Set MQTTServer address to: 192.168.0.13","mqtt_server":"192.168.0.13"}
```

### LEDCount
```
{"cmd":"ledCount", "num":<num>}
```

To set the number of LEDs for the strip.
The maximum supported number is 1024.


<a id="COMMANDS"></a>
## Commands

Overall, the commands to set different patterns or colors stick to the defined pattern given by the operator group: [µC-Communication](https://github.com/ubilab-escape/operator#%CE%BCc-communication)
```
{"method": "<method>","state": "<state>","data": "<data>"}
```
As the ledstrip are `outputs` and do not need to have a game logic, all the command are under the `trigger` method.
```
{"method":"trigger","state": "<state>","data": "<data>"}
```
`<state>` part can be one of the following commands:
- power
- globes
- blink
- rgb
- pattern
- brightness
- brightnessadjust
- patternadjust


<a id="POWER"></a>
### power
The power command turns on and off the ledstrip/timer.
```
{"method":"trigger", "state": "power","data": "<on|off>"}
```

<a id="GLOBES"></a>
### globes
This commands answer to the need of the group team4-door1, there are four globes, but only three led strips. One of the led strip (on top of the "north" wall between the escaperoom and lobbyroom is twice as long as the two other. This is split in two, therefore generating four "independent" ledstrips out of three! Both parts of the splitted ledstrip are triggered with the following command :
```
{"method":"trigger", "state": "globes", "data": "<0-255>,<0-255>,<0-255>,<0-255>,<0-255>,<0-255>"}
```
The three first values are the level of Red, Green and Blue of the first part, normally on the "North" (in fact, East) wall, this part is above Door1. The three next values are the RGB level of the remaining strip.

<a id="BLINK"></a>
### blink
It was asked to blink between two colors. With this command, the first value is the time spent on each color in milliseconds, the next three values are defining the RGB levels of the first color and the last three values are defining the RGB levels of the second color.
```
{"method":"trigger", "state": "blink", "data": "<integer>,<0-255>,<0-255>,<0-255>,<0-255>,<0-255>,<0-255>"}
```
This color also sets the foreground and background color and can therewith be used to change the color of the timer as well.

_NOTE:_ the timerprint pattern needs to be activated again.

<a id="RGB"></a>
### rgb
This function just sets the whole ledstrip to this color. 
```
{"method":"trigger", "state": "rgb", "data": "<0-255>,<0-255>,<0-255>"}
```
As this also sets the foreground color, it can be used to change the timer color. 

_NOTE:_ the timerprint pattern needs to be activated again.

<a id="PATTERN"></a>
### pattern
This function change the pattern of the ledstrip.
```
{"method":"trigger", "state": "pattern", "data": "<pattern name>"}
```
Available patterns are:
- colorwaves
- palettetest (test different color palettes)
- pride
- rainbow
- rainbowwithglitter
- confetti
- sinelon
- juggle
- bpm
- fire
- Solid Color
- timerprint (to show the current gametime)
- globes  

<a id="BRIGHTNESS"></a>
### brightness
This command independently from from the RGB values rescale the maximum intensity, between 0 (turn off) to 255 (full brightness).
```
{"method":"trigger", "state": "brightness", "data": "<0-255>"}
```

<a id="BRIGHTNESSADJUST"></a>
### brightnessadjust
This command cycles through few preset brightness values: 16, 32, 64, 128 and 155.
```
{"method":"trigger", "state": "brightnessadjust", "data": "<+1|-1>"}
```

<a id="PATTERNADJUST"></a>
### patternadjust
Similar to brightnessadjust, with this function, the next pattern is selected. With the exception, the pattern for the timer is ignored, i.e. the command will have no effect on ledstrip when the timer pattern is the next selected.
```
{"method":"trigger", "state": "patternadjust", "data": "<+1|-1>"}
```

<a id="CONTRIBUTORS"></a>
## Contributors
Jybz & Benjamin Völker
