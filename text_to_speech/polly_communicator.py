import boto3
import logging
import os

LOGGER = logging.getLogger(__name__)

class TextMessageParsingError(Exception):
    """
    Any error in parsing the text message will raise this Exception.
    """

class Message:

    def __init__(self, text_message, audio_root_dir, kwargs):
        self.client = boto3.client("polly")
        self.speech_kwargs = kwargs
        self.speech_kwargs["Text"] = text_message
        self.speech_kwargs["VoiceId"] = self.speech_kwargs.get(
                "VoiceId", "Joanna")
        self.speech_kwargs["OutputFormat"] = self.speech_kwargs.get(
                "OutputFormat", "mp3")
        self.audio_root_dir = audio_root_dir
        self.polly_response = None
        self.audio_file = None

    @classmethod
    def instance_from_mqtt_msg(cls, mqtt_message, audio_root_dir):
        """
        creates an instance from a mqtt message

            mqtt_message = {
                "method": "MESSAGE",
                "data": "I want to play this message on speaker.",
            }
        """
        try:
            LOGGER.debug("Working on the message {}".format(mqtt_message))
            text_message = mqtt_message["data"]
            kwargs = mqtt_message.get("kwargs", {})
            return cls(text_message, audio_root_dir, kwargs)
        except KeyError as e:
            raise TextMessageParsingError("Can't find the message to play.  KeyError {}".format(e.message))

    def synthesize_voice(self):
        response = self.client.synthesize_speech(**self.speech_kwargs)
        LOGGER.debug("Response received {}".format(response))
        self.polly_response = response

    def write_on_disk(self):
        filename = "{}.{}".format(
                self.polly_response["ResponseMetadata"]["RequestId"],
                self.speech_kwargs["OutputFormat"])
        filepath = os.path.join(self.audio_root_dir, filename)
        with open(filepath, "wb") as fd:
            fd.write(self.polly_response["AudioStream"].read())
        LOGGER.info("Wrote file {} on disk".format(filepath))
        self.audio_file = filepath

def generate_audio_file(mqtt_message, working_dir, saved_audio_map):
    msg_obj = Message.instance_from_mqtt_msg(mqtt_message, working_dir)
    audio_file = saved_audio_map.get(msg_obj.speech_kwargs["Text"])
    if audio_file:
        LOGGER.info("Using saved audio file for message {}".format(msg_obj.speech_kwargs["Text"]))
        return audio_file
    else:
        LOGGER.info("Generating audio file for message {}".format(msg_obj.speech_kwargs["Text"]))
        msg_obj.synthesize_voice()
        msg_obj.write_on_disk()
        saved_audio_map[msg_obj.speech_kwargs["Text"]] = msg_obj.audio_file
        return msg_obj.audio_file
