import argparse
import configparser
import json
import logging

import mqtt_message_receiver

logging.basicConfig()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", help="add config file")
    args = parser.parse_args()
    config = configparser.ConfigParser()
    config.read(args.config)
    host = config['mqtt']['host']
    topic_name = config['mqtt']['topic_name']
    working_dir = config['speech']['working_dir']
    saved_audio_map_file = config['speech']['saved_audio_map']
    with open(saved_audio_map_file, "r") as fd:
        saved_audio_map = json.load(fd)
    mqtt_client = mqtt_message_receiver.start_listening(host, topic_name,
            working_dir, saved_audio_map)
    mqtt_client.loop_forever()
    with open(saved_audio_map_file, "w") as fd:
        json.dump(saved_audio_map, fd)

if __name__ == "__main__":
    main()
