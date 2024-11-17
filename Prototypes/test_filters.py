import librosa
import numpy as np
import matplotlib.pyplot as plt

from scipy import signal as sig
from scipy import ndimage, datasets

import matplotlib.pyplot as plt
import ploting


def vertical(filter_size: int, x: np.ndarray) -> np.ndarray:    
    pad = filter_size // 2
    num_samples = x.shape[0]

    result = np.zeros(num_samples)
    filter = np.zeros(num_samples + filter_size)

    print("Filter Size: ", filter_size)
    print("Pad: ", pad)

    filter[pad:-pad] = x

    for h in range(num_samples):
        kernel = filter[h:h + filter_size].copy()
        kernel.sort()
        result[h] = kernel[pad - 1]

    return result

def horizontal(filter_size: int, X: np.ndarray) -> np.ndarray:    
    pad = filter_size // 2 - 1
    num_samples = X.shape[0]

    result = np.zeros(X.shape)
    filter = np.zeros(shape=(filter_size, num_samples))
    kernel = np.zeros(filter_size)
    median = np.zeros(filter_size)

    for j in range(X.shape[1]):
        x = X[:,j]

        # pop front
        filter = filter[1:]
        # push back
        filter = np.vstack([filter, x])

        for h in range(num_samples):
            for i in range(filter_size):
                median[i] = filter[i][h]

            kernel = median.copy()
            kernel.sort()
            result[h][j] = kernel[pad]

    return result

def transientness(xHorizonal: np.ndarray, xVertical: np.ndarray) -> np.ndarray:
    dest = np.zeros(xVertical.shape)
    len = xVertical.shape[0]

    for i in range(len):
        dest[i] = xVertical[i] / (xVertical[i] + xHorizonal[i] + np.finfo(float).eps)
    
    return dest

def stn(xHorizonal: np.ndarray, xVertical: np.ndarray, G1: float, G2: float) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    rt = transientness(xHorizonal, xVertical)
    S = np.zeros(rt.shape)
    T = np.zeros(rt.shape)
    N = np.zeros(rt.shape)

    tmp = np.pi / (2 * (G1 - G2))
    for i in range(len(rt)):
        rs = 1 - rt[i]

        if rs >= G1:
            S[i] = 1
        elif rs >= G2:
            sinRs = np.sin(tmp * (rs - G2))
            S[i] = sinRs * sinRs
        else:
            S[i] = 0

        if rt[i] >= G1:
            T[i] = 1
        elif rt[i] >= G2:
            sinRt = np.sin(tmp*(rt[i] - G2))
            T[i] = sinRt * sinRt
        else:
            T[i] = 0

        N[i] = 1 - S[i] - T[i]

    # plt.plot(S+T+N)
    # plt.show()

    assert(np.all(np.abs(S + T + N - 1) < 1e-10))
    return S, T, N

audio_file = "Soft Spot"
# Load audio file using librosa 
x, fs = librosa.load(f"../Evaluation/Audio/{audio_file}.aif", sr=None)

win = 8192
hop = win*7//8
win1 = sig.windows.hann(win,sym=False)

filter_length_t = 200e-3 # ms
filter_length_f = 500 # Hz

G1 = 0.8
G2 = 0.7

n_h = int(np.round(filter_length_t * fs // (win-hop)))
n_v = int(np.round(filter_length_f * win // fs))

f,t,X = sig.stft(x, fs=fs, window=win1, nperseg=win, noverlap=hop, return_onesided=True)

X_h_1 = ndimage.median_filter(np.absolute(X),size=(1,n_h+1),mode='constant', origin=(0,+n_h//2))
X_v_1 = ndimage.median_filter(np.absolute(X),size=(n_v+1,1),mode='constant')    

X_v_2 = vertical(n_v, np.absolute(X[:,10]))
X_h_2 = horizontal(n_h, np.absolute(X))

Rt = np.divide(X_v_1,(X_v_1 + X_h_1)+np.finfo(float).eps)
Rs = 1-Rt  

S = np.sin(np.pi*(Rs-G2)/(2*(G1-G2)))**2    
S[Rs>=G1] = 1
S[Rs<G2] = 0

T = np.sin(np.pi*(Rt-G2)/(2*(G1-G2)))**2
T[Rt>=G1] = 1
T[Rt<G2] = 0

N = 1-S-T

s1, t1, n1 = stn(X_h_2[:,10], X_v_2, G1, G2)

plt.figure(figsize=(10, 10))
plt.subplot(3,1,1)
plt.plot(S[:,10], label='Original')
plt.subplot(3,1,2)
plt.plot(s1, label='Custom')
plt.subplot(3,1,3)
plt.plot(S[:,10] - s1, label='diff')
plt.legend(loc="upper left")
plt.show()

plt.figure(figsize=(10, 10))
plt.subplot(3,1,1)
plt.plot(T[:,10], label='Original')
plt.subplot(3,1,2)
plt.plot(t1, label='Custom')
plt.subplot(3,1,3)
plt.plot(T[:,10] - t1, label='diff')
plt.legend(loc="upper left")
plt.show()

plt.figure(figsize=(10, 10))
plt.subplot(3,1,1)
plt.plot(N[:,10], label='Original')
plt.subplot(3,1,2)
plt.plot(n1, label='Custom')
plt.subplot(3,1,3)
plt.plot(N[:,10] - n1, label='diff')
plt.legend(loc="upper left")
plt.show()
