import json
import logging
import os
import paho.mqtt.client as mqtt
import subprocess

import gst
import polly_communicator

LOGGER = logging.getLogger(__name__)


class MessageHandler:

    def __init__(self, topic_name, working_dir, audio_device, saved_audio_map,
            saved_audios_dir):
        self.topic_name = topic_name
        self.working_dir = working_dir
        self.audio_device = audio_device
        self.saved_audio_map = saved_audio_map
        self.saved_audios_dir = saved_audios_dir
        self.player = gst.Player()

    def on_connect(self, client, userdata, flags, rc):
        client.subscribe(self.topic_name)

    def on_message(self, client, userdata, msg):
        message = json.loads(msg.payload.decode("utf-8"))
        try:
            if message.get("play_from_file"):
                LOGGER.info("Received request to play a audio file.")
                try:
                    audio_file = os.path.join(self.saved_audios_dir,
                            message["file_location"])
                except KeyError as e:
                    LOGGER.error("file location was missing in the message {}".format(message))
                    raise e
            else:
                LOGGER.info("Received request to play a text message.")
                audio_file = polly_communicator.generate_audio_file(message,
                        self.working_dir, self.saved_audio_map)
            self.player.play(audio_file, self.audio_device)
            LOGGER.info("Played audio_file {}".format(audio_file))
        except Exception as e:
            LOGGER.exception("some error occurred")

def start_listening(host, topic_name, working_dir, audio_device,
        saved_audio_map, saved_audios_dir):
    mqtt_client = mqtt.Client()
    mqtt_client.connect(host)
    LOGGER.info("MQTT client connected! Started listening ...")
    msg_handler = MessageHandler(topic_name, working_dir, audio_device,
            saved_audio_map, saved_audios_dir)
    mqtt_client.on_connect = msg_handler.on_connect
    mqtt_client.on_message = msg_handler.on_message
    return mqtt_client

