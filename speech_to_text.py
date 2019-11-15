#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import absolute_import, division, print_function

import argparse
import numpy as np
import shlex
import subprocess
import sys
import wave

from glob import iglob
from deepspeech import Model, printVersions
from timeit import default_timer as timer

try:
    from shhlex import quote
except ImportError:
    from pipes import quote

# Define the sample rate for audio
SAMPLE_RATE = 16000

# Number of MFCC features to use
N_FEATURES = 26

# Size of the context window used for producing timesteps in the input vector
N_CONTEXT = 9

def convert_samplerate(audio_path, desired_sample_rate):
    sox_cmd = 'C:/Program Files (x86)/sox-14-4-2/sox {} --type raw --bits 16 --channels 1 --rate {} --encoding signed-integer --endian little --compression 0.0 --no-dither - '.format(quote(audio_path), desired_sample_rate)
    try:
        output = subprocess.check_output(shlex.split(sox_cmd), stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        raise RuntimeError('SoX returned non-zero status: {}'.format(e.stderr))
    except OSError as e:
        raise OSError(e.errno, 'SoX not found, use {}hz files or install it: {}'.format(desired_sample_rate, e.strerror))

    return desired_sample_rate, np.frombuffer(output, np.int16)


def metadata_to_string(metadata):
    return ''.join(item.character for item in metadata.items)


class VersionAction(argparse.Action):
    def __init__(self, *args, **kwargs):
        super(VersionAction, self).__init__(nargs=0, *args, **kwargs)

    def __call__(self, *args, **kwargs):
        printVersions()
        exit(0)


def main():
    audio_file = "audio/2830-3980-0043.wav"
    beam_width = 500
    lm_alpha = 0.75
    lm_beta = 1.85
    print('Loading model from file {}'.format("deepspeech-0.5.1-models/output_graph.pbmm"), file=sys.stderr)
    model_load_start = timer()
    a = 2 & 0xffffffff
    ds = Model("deepspeech-0.5.1-models/output_graph.pbmm", N_FEATURES, N_CONTEXT, "deepspeech-0.5.1-models/alphabet.txt", beam_width)
    model_load_end = timer() - model_load_start
    print('Loaded model in {:.3}s.'.format(model_load_end), file=sys.stderr)

    # desired_sample_rate = ds.sampleRate()

    # if args.lm and args.trie:
    print('Loading language model from files {} {}'.format("deepspeech-0.5.1-models/lm.binary", "deepspeech-0.5.1-models/trie"), file=sys.stderr)
    lm_load_start = timer()
    ds.enableDecoderWithLM("deepspeech-0.5.1-models/alphabet.txt", "deepspeech-0.5.1-models/lm.binary", "deepspeech-0.5.1-models/trie", lm_alpha, lm_beta)
    lm_load_end = timer() - lm_load_start
    print('Loaded language model in {:.3}s.'.format(lm_load_end), file=sys.stderr)

    # for audio_file in ["audio/20191114235456.wav", "audio/20191114235736.wav", "audio/20191115004812.wav"]:
    for audio_file in iglob("audio/*427.wav"):
        fin = wave.open(audio_file, 'rb')
        fs = fin.getframerate()
        print(fs)
        if fs != SAMPLE_RATE:
            print('Warning: original sample rate ({}) is different than {}hz. Resampling might produce erratic speech recognition.'.format(fs, SAMPLE_RATE), file=sys.stderr)
            fs, audio = convert_samplerate(audio_file, SAMPLE_RATE)
        else:
            audio = np.frombuffer(fin.readframes(fin.getnframes()), np.int16)

        audio_length = fin.getnframes() * (1/fs)
        fin.close()

        print('Running inference.', file=sys.stderr)
        inference_start = timer()
        # if args.extended:
        #     print(metadata_to_string(ds.sttWithMetadata(audio)))
        # else:
        print(ds.stt(audio, fs))
        inference_end = timer() - inference_start
        print('Inference took %0.3fs for %0.3fs audio file.' % (inference_end, audio_length), file=sys.stderr)

if __name__ == '__main__':
    main()
    # fin = wave.open("audio/20191114235456.wav", "rb")