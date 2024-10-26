import decomposeSTN as STN
import librosa

import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

import sounddevice as sd
import soundfile as sf

import config

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

def noise_morphing(x, stretch_ratio=2.0):
    fft_size = window_size = 2048
    hop = fft_size // 2
    window = np.hanning(window_size)

    # stft
    X = librosa.stft(x, n_fft=fft_size, hop_length=hop, window=window)
    print("STFT:", X.shape)

    Xn_db = 10 * np.log10(np.abs(X)) # log-magnitude spectrum

    Xn_len = X.shape[1] # number of frames
    Xn_stretched_len = int(np.ceil(Xn_len * stretch_ratio)) - 1 # number of frames after stretching (-1 to fix stft padding)

    Xn_stretched_db = np.zeros((X.shape[0], Xn_stretched_len)) # output array

    # Boundries
    Xn_stretched_db[:,0] = Xn_db[:,0] 
    Xn_stretched_db[:,-1] = Xn_db[:,-1]

    # streteched original indices -> [0,1,2,3,...,N] -> 1.5 -> [0, 1.5, 3, 4.5,..., N*1.5]
    stretched_indices = np.arange(Xn_len) * stretch_ratio

    j = 1
    
    prev_idx = 0
    prev_xn = Xn_db[:,0] 

    next_idx = stretched_indices[j]
    next_xn = Xn_db[:,j]

    prev_next_dist = next_idx - prev_idx

    # Interpolation
    for i in range(1, Xn_stretched_len-1):
        if i > next_idx:
            j += 1

            prev_idx = next_idx
            prev_xn = next_xn

            next_idx = stretched_indices[j]
            next_xn = Xn_db[:,j]

            prev_next_dist = next_idx - prev_idx
        
        prev_dist = i - prev_idx
        next_dist = next_idx - i

        Xn_stretched_db[:,i] = prev_xn * (next_dist / prev_next_dist) + next_xn * (prev_dist / prev_next_dist)

    if config.visualize:
        plt.figure(figsize=(10, 10))
        plt.subplot(5,2,1)
        plt.plot(Xn_stretched_db[:,0])
        plt.subplot(5,2,2)
        plt.plot(Xn_db[:,0])

        plt.subplot(5,2,3)
        plt.plot(Xn_stretched_db[:,1])
        plt.subplot(5,2,4)
        plt.plot()

        plt.subplot(5,2,5)
        plt.plot(Xn_stretched_db[:,2])
        plt.subplot(5,2,6)
        plt.plot(Xn_db[:,1])

        plt.subplot(5,2,7)
        plt.plot(Xn_stretched_db[:,3])
        plt.subplot(5,2,8)
        plt.plot()
        
        plt.subplot(5,2,9)
        plt.plot(Xn_stretched_db[:,4])
        plt.subplot(5,2,10)
        plt.plot(Xn_db[:,2])

        plt.show()

    Xn_stretched = 10**(Xn_stretched_db / 10) # inverse log-magnitude spectrum
    
    white_noise = np.random.normal(0, 1, int(np.ceil(len(x)*stretch_ratio))) # generate white noise
    E = librosa.stft(white_noise, n_fft=fft_size, hop_length=hop, window=window) # run stft on it 
    assert E.shape[1] == Xn_stretched.shape[1]

    E = E / np.sqrt(np.sum(window**2)) # normalize by the window energy to ensure spectral magnitude equals 1

    # multiply each frame of the white noise by the interpolated frame of the input's noise (Xn_stretched)
    Xn_stretched = Xn_stretched * E # element wise multiplication

    plt.plot(Xn_stretched[:,0])
    plt.show()

    return librosa.istft(Xn_stretched, n_fft=fft_size, hop_length=hop, window=window)


# Load audio file using librosa 
audioInput, Fs = librosa.load('../Evaluation/Audio/Soft Spot.aif', sr=None)

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
    plotAudio(np.concatenate([xs, np.zeros(len(xs))]), Fs, 'Sines')
    plt.subplot(2,1,2)
    plotAudio(xs_stretched, Fs, 'Stretched Sines')
    plt.show()


# Transients - just moved in time
xt_stretched = move_transients(xt, Fs, timeStretchRatio)
if config.visualize:
    plt.figure(figsize=(10, 10))
    plt.subplot(2,1,1)
    plotAudio(np.concatenate([xt, np.zeros(len(xt))]), Fs, 'Transients')
    plt.subplot(2,1,2)
    plotAudio(xt_stretched, Fs, 'Stretched Transients')
    plt.show()

# Noise - Noise Morphing
xn_stretched = noise_morphing(xn, timeStretchRatio)
if config.visualize:
    plt.figure(figsize=(10, 10))
    plt.subplot(2,1,1)
    plotAudio(np.concatenate([xn, np.zeros(len(xt))]), Fs, 'Noise')
    plt.subplot(2,1,2)
    plotAudio(xn_stretched, Fs, 'Stretched Noise')
    plt.show()

# Combine the three components
output = xs_stretched + xt_stretched + xn_stretched

if config.visualize:
    plotSTN(audioInput, xs, xt, xn, Fs)
    plotSTN(output, xs_stretched, xt_stretched, xn_stretched, Fs)

# Save audio files
if config.save_audio:
    sf.write('Audio/noise.wav', xn, Fs)
    sf.write('Audio/transients.wav', xt, Fs)
    sf.write('Audio/sines.wav', xs, Fs)

    sf.write('Audio/nm_ts_x2_noise.wav', xn_stretched, Fs)
    sf.write('Audio/nm_ts_x2_sines.wav', xs_stretched, Fs)
    sf.write('Audio/nm_ts_x2_transients.wav', xt_stretched, Fs)
    sf.write('Audio/nm_ts_x2_output.wav', output, Fs)

if config.play_audio:
    input('Press Enter to play the input...')
    sd.play(xn, Fs)

    input('Press Enter to play the output...')
    sd.play(xn_stretched, Fs)

    input('Press Enter to continue')


if config.visualize:
    plotAudio(output, Fs, 'Output')
    plt.show()


