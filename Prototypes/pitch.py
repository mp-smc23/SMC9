import decomposeSTN as STN
import librosa

import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

import sounddevice as sd
import soundfile as sf

import config
import ploting 
import nm

audio_file = "Soft Spot"
# Load audio file using librosa 
audio, fs = librosa.load(f"../Evaluation/Audio/{audio_file}.aif", sr=None)

nWin1 = 8192 # samples
nWin2 = 512 # samples

# Trim audio to be a multiple of nWin1 (for easiness of STFT calculations)
cut = len(audio) - len(audio) % nWin1
audio = audio[:cut]

print(f"Sample Rate = {fs}")
print(f"Audio Input Shape = {audio.shape}")
print(f"Window 1 Size = {nWin1}")
print(f"Window 2 Size = {nWin2}")

print("Decomposing STN...")
[xs, xt, xn] = STN.decSTN(audio, fs, nWin1, nWin2)

pitch_shift_semitones = 12
pitch_shift_ratio = 2**(pitch_shift_semitones/12)
tsm_ratio = 1 / pitch_shift_ratio

print("========================")
print("Pitch Shift Semitones:", pitch_shift_semitones)
print("Pitch Shift Ratio:", pitch_shift_ratio)


# Sinusoids - pitch shifting
xs_shifted = librosa.effects.pitch_shift(xs, sr=fs, n_steps=pitch_shift_semitones)
if config.visualize:
    plt.figure(figsize=(10, 10))
    plt.subplot(2,1,1)
    ploting.plotSpectogram(xs, fs, 'Sines')
    plt.subplot(2,1,2)
    ploting.plotSpectogram(xs_shifted, fs, 'Shifted Sines')
    plt.show()

# Noise - TS and resample 
xn_stretched = nm.noise_stretching(xn, pitch_shift_ratio)
xn_shifted = librosa.resample(xn_stretched, orig_sr=fs*pitch_shift_ratio, target_sr=fs)
if config.visualize:
    plt.figure(figsize=(10, 10))
    plt.subplot(2,1,1)
    ploting.plotSpectogram(xn, fs, 'Noise')
    plt.subplot(2,1,2)
    ploting.plotSpectogram(xn_shifted, fs, 'Shifted Noise')
    plt.show()

# Transients - unprocessed
xt_shifted = xt

output = xs_shifted + xt_shifted + xn_shifted

# Save audio files
if config.save_audio:
    sf.write(f"Audio/librosa_ps_12_{audio_file}.wav", librosa.effects.pitch_shift(audio, sr=fs, n_steps=pitch_shift_semitones), fs)
    sf.write(f"Audio/nm_ps_12_{audio_file}_sines.wav", xs_shifted, fs)
    sf.write(f"Audio/nm_ps_12_{audio_file}_noise.wav", xn_shifted, fs)
    sf.write(f"Audio/nm_ps_12_{audio_file}_transients.wav", xt_shifted, fs)
    sf.write(f"Audio/nm_ps_12_{audio_file}_output.wav", output, fs)

if config.play_audio:
    input('Press Enter to play the input...')
    sd.play(audio, fs)

    input('Press Enter to play the output...')
    sd.play(output, fs)

    input('Press Enter to continue')


if config.visualize:
    plt.figure(figsize=(10, 10))
    plt.subplot(2,1,1)
    ploting.plotAudio(output, fs, 'Output')
    plt.subplot(2,1,2)
    ploting.plotSpectogram(output, fs, 'Output Spectrum')
    plt.show()
