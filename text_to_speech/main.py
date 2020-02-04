import argparse
import configparser
import json
import logging
import logging.handlers
import signal

import mqtt_message_receiver

LOGGER = logging.getLogger()

def signal_handler(mqtt_client, saved_audio_map_file, saved_audio_map):
    mqtt_client.disconnect()
    mqtt_client.loop_stop()
    with open(saved_audio_map_file, "w") as fd:
        LOGGER.info("Saving played messages")
        json.dump(saved_audio_map, fd)
    LOGGER.info("Shutting down text to speech")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", help="add config file")
    args = parser.parse_args()
    config = configparser.ConfigParser()
    config.read(args.config)
    LOGGER.setLevel(config['logging']['level'])
    rotating_handler = logging.handlers.TimedRotatingFileHandler(
            config['logging']['file'], 'd', 1, 5)
    formatter = logging.Formatter('[%(asctime)s][%(process)d][%(levelname)s] %(message)s')
    rotating_handler.setFormatter(formatter)
    LOGGER.addHandler(rotating_handler)
    host = config['mqtt']['host']
    topic_name = config['mqtt']['topic_name']
    working_dir = config['speech']['working_dir']
    saved_audio_map_file = config['speech']['saved_audio_map']
    audio_device = config['speech']['audio_device']
    LOGGER.info("Config parsed. Starting text-to-speech.")
    with open(saved_audio_map_file, "r") as fd:
        LOGGER.info("Loading previously played messages")
        saved_audio_map = json.load(fd)
    mqtt_client = mqtt_message_receiver.start_listening(host, topic_name,
            working_dir, audio_device, saved_audio_map)
    [signal.signal(sig, lambda signum, frame: signal_handler(mqtt_client,
        saved_audio_map_file, saved_audio_map)) for sig in [signal.SIGINT, signal.SIGTERM]]
    mqtt_client.loop_forever()
    signal.pause()

if __name__ == "__main__":
    main()
