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

def move_env(x: float, att: float, rel: float, env: float):
    x = np.abs(x)
    if x > env:
        env += att * (x - env)
    else:
        env += rel * (x - env)
    return env


def move_transients(x, sr, stretch_ratio=2.0):
    attack = 1000 / (sr * 2.23)
    release = 1000 / (sr * 10.40)
    attackControl = 1000 / (sr * 60.84)
    releaseControl = 1000 / (sr * 8.67)

    env = [0]
    envCtrl = [0]

    processing_transient = False
    t_start = 0
    eps = 1e-8
    output = np.zeros(int(len(x) * stretch_ratio))
    for i in range(len(x)):
        env.append(move_env(x[i], attack, release, env[-1]))
        envCtrl.append(move_env(x[i], attackControl, releaseControl, envCtrl[-1]))

        if not processing_transient and env[-1] > envCtrl[-1]:
            # ! new transient
            # Copy previous transient
            t_start_stretched = int(t_start * stretch_ratio)
            t_len = i - t_start
            window = signal.windows.tukey(t_len)
            output[t_start_stretched : t_start_stretched + t_len] = x[t_start : i] * window

            # Update transient start
            t_start = i
            processing_transient = True
        elif env[-1] - 0.0005 < envCtrl[-1]:
            processing_transient = False
            
    
    if config.visualize:
        plt.plot(x)
        plt.plot(env)
        plt.plot(envCtrl)
        plt.show()  
    return output


audio_file = "Drum Loop"
# Load audio file using librosa 
audioInput, Fs = librosa.load(f"../Evaluation/Audio/{audio_file}.aif", sr=None)

nWin1 = 8192 # samples
nWin2 = 512 # samples

cut = len(audioInput) - len(audioInput) % nWin1
audioInput = audioInput[:cut]

print(f"Sample Rate = {Fs}")
print(f"Audio Input Shape = {audioInput.shape}")
print(f"Window 1 Size = {nWin1}")
print(f"Window 2 Size = {nWin2}")

print("Decomposing STN...")
[xs, xt, xn] = STN.decSTN(audioInput, Fs, nWin1, nWin2)


timeStretchRatio = 2.0
inverseTimeStretchRatio = 1.0 / timeStretchRatio

# Sines - phase vocoder
xs_stretched = librosa.effects.time_stretch(xs, rate=inverseTimeStretchRatio) # for some reason 0.5 == 2
if config.visualize:
    plt.figure(figsize=(10, 10))
    plt.subplot(2,1,1)
    ploting.plotAudio(np.concatenate([xs, np.zeros(len(xs))]), Fs, 'Sines')
    plt.subplot(2,1,2)
    ploting.plotAudio(xs_stretched, Fs, 'Stretched Sines')
    plt.show()


# Transients - just moved in time
xt_stretched = move_transients(xt, Fs, timeStretchRatio)
if config.visualize:
    plt.figure(figsize=(10, 10))
    plt.subplot(2,1,1)
    ploting.plotAudio(np.concatenate([xt, np.zeros(len(xt))]), Fs, 'Transients')
    plt.subplot(2,1,2)
    ploting.plotAudio(xt_stretched, Fs, 'Stretched Transients')
    plt.show()

# Noise - Noise Morphing
xn_stretched = nm.noise_stretching(xn, timeStretchRatio)
if config.visualize:
    plt.figure(figsize=(10, 10))
    plt.subplot(2,1,1)
    ploting.plotAudio(np.concatenate([xn, np.zeros(len(xn))]), Fs, 'Noise')
    plt.subplot(2,1,2)
    ploting.plotAudio(xn_stretched, Fs, 'Stretched Noise')
    plt.show()

# Combine the three components
output = xs_stretched + xt_stretched + xn_stretched

if config.visualize:
    ploting.plotSTN(audioInput, xs, xt, xn, Fs)
    ploting.plotSTN(output, xs_stretched, xt_stretched, xn_stretched, Fs)

# Save audio files
if config.save_audio:
    sf.write(f"Audio/{audio_file}_noise.wav", xn, Fs)
    sf.write(f"Audio/{audio_file}_transients.wav", xt, Fs)
    sf.write(f"Audio/{audio_file}_sines.wav", xs, Fs)

    sf.write(f"Audio/librosa_x2_{audio_file}.wav", librosa.effects.time_stretch(audioInput, rate=0.5), Fs)
    sf.write(f"Audio/nm_ts_x2_{audio_file}_sines.wav", xs_stretched, Fs)
    sf.write(f"Audio/nm_ts_x2_{audio_file}_noise.wav", xn_stretched, Fs)
    sf.write(f"Audio/nm_ts_x2_{audio_file}_transients.wav", xt_stretched, Fs)
    sf.write(f"Audio/nm_ts_x2_{audio_file}_output.wav", output, Fs)

if config.play_audio:
    input('Press Enter to play the input...')
    sd.play(xn, Fs)

    input('Press Enter to play the output...')
    sd.play(output, Fs)

    input('Press Enter to continue')


if config.visualize:
    ploting.plotAudio(output, Fs, 'Output')
    plt.show()


