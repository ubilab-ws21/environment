import argparse
import configparser
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
    mqtt_client = mqtt_message_receiver.start_listening(host, topic_name, working_dir)
    mqtt_client.loop_forever()

if __name__ == "__main__":
    main()
