#!/bin/bash

#firefox  https://openinframap.org/
#ffplay -an -loop 0 -x 1024 -y 768 /home/ubilab/Videos/PowerGridFail/PowerGridFail_2.mp4

python3 - <<MQTT
import paho.mqtt.client as mqtt
import json, subprocess

vidproc = None

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("env/video")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global vidproc

    j = msg.payload
    
    try: vidproc.kill()
    except: pass

    print(j)
    #vidproc = subprocess.Popen(["ffplay", "-an", "-loop", "0", "-x", "1024", "-y", "768", j])
    vidproc = subprocess.Popen(["ffplay", "-an", "-loop", "0", "-fs", j])

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()
MQTT
