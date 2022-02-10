import sys, os, signal, json, subprocess
import paho.mqtt.client as mqtt

OFF_VIDEO = os.path.join(os.path.abspath(os.path.dirname(__file__)), "black.mp4")
BASE_VIDCMD = ["ffplay", "-v", "error", "-fs", "-x", "1024", "-y", "768"]
vidproc = None

def signal_handler(mqtt_client):
    print("[videoplayer] Disconnecting from MQTT...")
    mqtt_client.disconnect()
    mqtt_client.loop_stop()
    print("[videoplayer] Killing video...")
    try: vidproc.kill()
    except: pass
    sys.exit(0)


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("[videoplayer] Connected to MQTT with result code " + str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("env/video")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global vidproc

    try:
        j = json.loads(msg.payload.decode("utf-8"))
    except:
        print("[videoplayer] Got invalid or non-JSON message: " + str(msg.payload))
        return

    vidcmd = BASE_VIDCMD

    # {"method": "trigger", "state": "on", "data": {"path":"/foo/bar.mp4", "loop": True, "mute": True}}
    if "method" in j and j["method"] == "trigger" and "state" in j and (j["state"] == "on" or j["state"] == "off"):
        if j["state"] == "on":
            if "data" in j and "path" in j["data"]:
                loop = j["data"]["loop"] if "loop" in j["data"] else True
                mute = j["data"]["mute"] if "mute" in j["data"] else True
                if loop: vidcmd.extend(["-loop", "0"])
                if mute: vidcmd.extend(["-an"])
                vidcmd.extend([j["data"]["path"]])
            else:
                print("[videoplayer] Got invalid message: " + str(j))
                return
        elif j["state"] == "off":
            vidcmd.extend(["-an", "-loop", "0", OFF_VIDEO])
    else:
        print("[videoplayer] Got invalid message: " + str(j))
        return
    
    print("[videoplayer] Playing video with command: " + " ".join(vidcmd))
    try: vidproc.kill()
    except: pass
    vidproc = subprocess.Popen(vidcmd)


if __name__ == "__main__":
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect("localhost", 1883, 60)

    vidproc = subprocess.Popen(BASE_VIDCMD + ["-an", "-loop", "0", OFF_VIDEO])

    [signal.signal(sig, lambda signum, frame: signal_handler(client)) for sig in [signal.SIGINT, signal.SIGTERM]]
    client.loop_start()
    signal.pause()
