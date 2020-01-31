import json
import logging
import paho.mqtt.client as mqtt
import subprocess

import gst
import polly_communicator

LOGGER = logging.getLogger(__name__)


class MessageHandler:

    def __init__(self, topic_name, working_dir, audio_device, saved_audio_map):
        self.topic_name = topic_name
        self.working_dir = working_dir
        self.audio_device = audio_device
        self.saved_audio_map = saved_audio_map
        self.player = gst.Player()

    def on_connect(self, client, userdata, flags, rc):
        client.subscribe(self.topic_name)

    def on_message(self, client, userdata, msg):
        message = json.loads(msg.payload.decode("utf-8"))
        try:
            LOGGER.info("Received request")
            audio_file = polly_communicator.generate_audio_file(message,
                    self.working_dir, self.saved_audio_map)
            self.player.play(audio_file, self.audio_device)
            LOGGER.info("Played audio_file {}".format(audio_file))
        except Exception as e:
            LOGGER.exception("some error occurred")

def start_listening(host, topic_name, working_dir, audio_device, saved_audio_map):
    mqtt_client = mqtt.Client()
    mqtt_client.connect(host)
    msg_handler = MessageHandler(topic_name, working_dir, audio_device, saved_audio_map)
    mqtt_client.on_connect = msg_handler.on_connect
    mqtt_client.on_message = msg_handler.on_message
    return mqtt_client

