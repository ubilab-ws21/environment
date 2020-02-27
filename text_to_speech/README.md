# Text-to-speech

Text-to-speech provides an interaction medium between the game operators and the players. The operator can play text message for the players.

![flow diagram](https://github.com/ubilab-escape/environment/raw/master/images/text-to-speech_flow.png)

This code runs on a local system which has speakers and correct *audio-drivers*.
## MQTT messages
```json
msg1 = {
        "method": "MESSAGE",
        "data": "I would like to play the message.",
        }
msg2 = {
        "method": "MESSAGE",
        "data": "<speak>It is a normal voice. <amazon:effect vocal-tract-length='+100%'>Play something in with different vocal tract length</amazon:effect> </speak>",
        "kwargs": {"TextType": "ssml", "VoiceId":"Brian", "LanguageCode":"en-GB"}
        }
```
The messages should be sent to the topic specified in the config file.

## Running parameters
Below parameters should be saved under a config file.\
`[speech]`\
`voice_id`: Default Voice ID of the Polly service to use.\
`output_format`: Default format of the audio file in which it should be saved.\
`working_dir`: Working directory where all audio files would be saved.\
`saved_audio_map`: Cache file which contains all the text messages with the corresponding audio file.


`[mqtt]`\
`host`: MQTT server address.\
`topic_name`: MQTT topic on which this service would listen.


`[logging]`\
`level`: Logging level.\
`file`: Log file address.


## Start Command
`/path/to/the/text-to-speech/service/on/the/disk/main.py -c /path/to/the/config/file/filename.cfg`

## Setup AWS Key
The requests to the Amazon Polly should be authorized. Since, the requests will be made from a local system, all the requests should be signed with a valid AWS access key. 
### Create user
Create a user with ` AmazonPollyFullAccess ` permissions.
### Generate access key
Generate an access key for that user. This access key will be used to sign API requests.
### Setup AWS access key
Install `awscli` on the local machine.
Run command `aws configure`.
Fill in the all the parameters.

Access key can now be used to make requests to Amazon Polly.
