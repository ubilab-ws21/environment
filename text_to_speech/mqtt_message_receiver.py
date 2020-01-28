import json
import logging
import paho.mqtt.client as mqtt
import subprocess

import polly_communicator

LOGGER = logging.getLogger(__name__)

class MessageHandler:

    def __init__(self, topic_name, working_dir):
        self.topic_name = topic_name
        self.working_dir = working_dir

    def on_connect(self, client, userdata, flags, rc):
        client.subscribe(self.topic_name)

    def on_message(self, client, userdata, msg):
        message = json.loads(msg.payload.decode("utf-8"))
        try:
            audio_file = polly_communicator.generate_audio_file(message, self.working_dir)
            subprocess.run(
                    ["mpg123", "-a", "plughw:CARD=PCH,DEV=0", audio_file],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
                    )
            # playsound.playsound(audio_file) # playsound doesn't work.
        except Exception as e:
            LOGGER.exception("some error occurred")

def start_listening(host, topic_name, working_dir):
    mqtt_client = mqtt.Client()
    mqtt_client.connect(host)
    msg_handler = MessageHandler(topic_name, working_dir)
    mqtt_client.on_connect = msg_handler.on_connect
    mqtt_client.on_message = msg_handler.on_message
    return mqtt_client

