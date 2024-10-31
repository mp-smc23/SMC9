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
