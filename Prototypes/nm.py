import decomposeSTN as STN
import librosa

import matplotlib.pyplot as plt
import numpy as np

def plotAudio(x, sr, title):
    librosa.display.waveshow(x, sr=sr)
    plt.title(title)

def plotSpectogram(x, sr, title):
    D = librosa.stft(x)
    S_db = librosa.amplitude_to_db(np.abs(D), ref=np.max)
    librosa.display.specshow(S_db, x_axis='time', sr=sr)
    plt.colorbar() 
    plt.title(title)

def plotSTN(x, xs, xt, xn, sr):
    plt.figure(figsize=(10, 10))
    plt.suptitle('STN Decomposition')

    plt.subplot(4,2,1)
    plotAudio(x, sr, 'Original Audio')
    plt.subplot(4,2,2)
    plotSpectogram(x, sr, 'Original Spectogram')

    plt.subplot(4,2,3)
    plotAudio(xs, sr, 'Sines')
    plt.subplot(4,2,4)
    plotSpectogram(xs, sr, 'Sines Spectogram')

    plt.subplot(4,2,5)
    plotAudio(xt, sr, 'Transients')
    plt.subplot(4,2,6)
    plotSpectogram(xt, sr, 'Transients Spectogram')

    plt.subplot(4,2,7)
    plotAudio(xn, sr, 'Noise')
    plt.subplot(4,2,8)
    plotSpectogram(xn, sr, 'Noise Spectogram')

    plt.tight_layout()
    plt.show()


# Load audio file using librosa 
audioInput, Fs = librosa.load('../Evaluation/Audio/Soft Spot.aif', sr=None)

nWin1 = 8192 # samples
nWin2 = 512 # samples

print(f"Sample Rate = {Fs}")
print(f"Audio Input Shape = {audioInput.shape}")
print(f"Window 1 Size = {nWin1}")
print(f"Window 2 Size = {nWin2}")

print("Decomposing STN...")
[xs, xt, xn] = STN.decSTN(audioInput, Fs, nWin1, nWin2)

# Plot the results
plotSTN(audioInput, xs, xt, xn, Fs)

timeStretchRatio = 2.0

# Sines - phase vocoder
xs_stretched = librosa.effects.time_stretch(xs, timeStretchRatio)

# Transients - just moved in time
# Noise - Noise Morphing

