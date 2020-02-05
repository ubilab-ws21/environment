import logging
import os, sys, time
from sdl2 import *
from sdl2.sdlmixer import *

LOGGER = logging.getLogger(__name__)

class Player:
	def __init__(self, channels = 16, path='.'):
		self.channels = channels
		self.path = os.path.realpath(path)
		self.cc = 0

	def init(self):
		if SDL_Init(SDL_INIT_AUDIO) != 0:
			raise RuntimeError("Cannot initialize audio system: {}".format(SDL_GetError()))
		f = 48000
		if Mix_OpenAudio(f, MIX_DEFAULT_FORMAT, 2, 4000):
			raise RuntimeError("Cannot open mixed audio: {}".format(Mix_GetError()))

		self.channels = Mix_AllocateChannels(8)
		LOGGER.info("Device opened at %d Hz with %d mixing channels" % (f, self.channels))

	def deinit(self):
		Mix_CloseAudio()
		SDL_Quit(SDL_INIT_AUDIO)

	def play(self, filename):
		p = os.path.join(self.path, filename)
		sample = Mix_LoadWAV(p.encode())
		if sample is None:
			raise RuntimeError("Cannot open audio file: {}".format(Mix_GetError()))
		c = self.cc
		Mix_HaltChannel(c)
		r = Mix_PlayChannel(c, sample, 0)
		if r < 0:
			raise RuntimeError("Cannot play sample: {}".format(Mix_GetError()))
		self.cc = (c+1) % self.channels

	def wait(self):
		while Mix_Playing(-1) > 0:
			SDL_Delay(100)


if __name__ == "__main__":
	p = Player()
	p.init()

	p.play("sdl.mp3")
	p.wait()

	p.play("sdl.mp3")
	time.sleep(0.5)
	p.play("sdl.mp3")
	p.wait()

	p.deinit()
