#!/usr/bin/env python3

import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GObject



class Player:
    def __init__(self):
        pass

    def on_message(self, bus, message, loop):
        t = message.type
        if t == Gst.MessageType.EOS:
            loop.quit()
        elif t == Gst.MessageType.ERROR:
            loop.quit()
            err, debug = message.parse_error()
            print("Error: %s: %s" % (err, debug))

    def play(self, file, device):
        pipe = Gst.parse_launch('uridecodebin name=src ! audioconvert ! audioresample ! audio/x-raw, rate=48000 ! alsasink name=sink')

        file = Gst.filename_to_uri(file)
        pipe.get_by_name("src").set_property("uri", file)
        pipe.get_by_name("sink").set_property("device", device)

        loop = GObject.MainLoop()

        bus = pipe.get_bus()
        bus.add_signal_watch()
        bus.connect("message", self.on_message, loop)

        pipe.set_state(Gst.State.PLAYING)
        try:
            loop.run()
        except:
            print("exception")
        pipe.set_state(Gst.State.NULL)



GObject.threads_init()
Gst.init(None)

