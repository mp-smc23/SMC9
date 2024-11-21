import librosa
import numpy as np
import matplotlib.pyplot as plt
import config

def noise_stretching(x, stretch_ratio=2.0):
    fft_size = window_size = 2048
    hop = fft_size // 2
    window = np.hanning(window_size)

    # stft
    X = librosa.stft(x, n_fft=fft_size, hop_length=hop, window=window)
    print("STFT:", X.shape)

    Xn_db = 10 * np.log10(np.abs(X)) # log-magnitude spectrum

    Xn_stretched_db = frames_interpolation(Xn_db, stretch_ratio) # interpolate log-magnitude spectrum

    if config.visualize:
        plt.figure(figsize=(10, 10))
        plt.subplot(5,2,1)
        plt.plot(Xn_db[:,0])
        plt.subplot(5,2,2)
        plt.plot(Xn_stretched_db[:,0])

        plt.subplot(5,2,3)
        plt.plot(Xn_db[:,1])
        plt.subplot(5,2,4)
        plt.plot()

        plt.subplot(5,2,5)
        plt.plot(Xn_db[:,2])
        plt.subplot(5,2,6)
        plt.plot(Xn_stretched_db[:,1])

        plt.subplot(5,2,7)
        plt.plot(Xn_db[:,3])
        plt.subplot(5,2,8)
        plt.plot()
        
        plt.subplot(5,2,9)
        plt.plot(Xn_db[:,4])
        plt.subplot(5,2,10)
        plt.plot(Xn_stretched_db[:,2])

        plt.show()

    Xn_stretched = 10**(Xn_stretched_db / 10) # inverse log-magnitude spectrum
    
    Xn_stretched = noise_morphing(
        Xn_stretched, 
        original_signal_len=len(x), 
        n_fft=fft_size, 
        hop_length=hop, 
        window=window, 
        stretch_ratio=stretch_ratio) # morph with white noise

    return librosa.istft(Xn_stretched, n_fft=fft_size, hop_length=hop, window=window)


def frames_interpolation(x: np.ndarray, stretch_ratio: float = 2.0) -> np.ndarray:
    """ Linearly interpolates the log-magnitude spectrum is then according to the stretching factor based on the two neighboring spectra, occurring before and after the interpolation point.

    Args:
        x (np.ndarray): log-magnitude spectrum
        stretch_ratio (float, optional): stretching factor. Defaults to 2.0.

    Returns:
        np.ndarray: Stretched log-magnitude spectrum
    """

    x_len = x.shape[1] # number of frames
    x_stretched_len = int(np.ceil(x_len * stretch_ratio)) # number of frames after stretching 
    
    if stretch_ratio > 1: # fixing stft padding
        x_stretched_len -= 1

    x_stretched = np.zeros((x.shape[0], x_stretched_len)) # output array

    # Boundries
    x_stretched[:,0] = x[:,0] 
    x_stretched[:,-1] = x[:,-1]

    # streteched original indices -> [0,1,2,3,...,N] -> 1.5 -> [0, 1.5, 3, 4.5,..., N*1.5]
    stretched_indices = np.arange(x_len) * stretch_ratio
    j = 1
    
    prev_idx = 0
    prev_xn = x[:,0] 

    next_idx = stretched_indices[j]
    next_xn = x[:,j]

    prev_next_dist = next_idx - prev_idx

    # Interpolation
    for i in range(1, x_stretched_len-1):
        while i > next_idx:
            j += 1

            prev_idx = next_idx
            prev_xn = next_xn

            next_idx = stretched_indices[j]
            next_xn = x[:,j]

            prev_next_dist = next_idx - prev_idx
        
        prev_dist = i - prev_idx
        next_dist = next_idx - i

        x_stretched[:,i] = prev_xn * (next_dist / prev_next_dist) + next_xn * (prev_dist / prev_next_dist)

    return x_stretched


def noise_morphing(x: np.ndarray, original_signal_len: int, n_fft: int, window: np.ndarray, hop_length: int,  stretch_ratio: float = 2.0) -> np.ndarray:
    """ Interpolated magnitude spectra are modulated by the white noise spectral frames via element-wise multiplication.

    Args:
        x (np.ndarray): magnitude spectrum
        original_signal_len (int): length of the original signal
        n_fft (int): FFT length
        window (np.ndarray): FFT windowing function
        hop_length (int): FFT hop length
        stretch_ratio (float, optional): stretch factor. Defaults to 2.0.

    Returns:
        np.ndarray: Modulated magnitude spectra
    """

    white_noise = np.random.normal(0, 1, int(np.ceil(original_signal_len*stretch_ratio))) # generate white noise
    white_noise = white_noise / np.max(np.abs(white_noise)) # normalize it

    E = librosa.stft(white_noise, n_fft=n_fft, hop_length=hop_length, window=window) # run stft on it 

    print("E", E.shape)
    print("x", x.shape)
    assert E.shape[1] == x.shape[1]

    E = E / np.sqrt(np.sum(window**2)) # normalize by the window energy to ensure spectral magnitude equals 1

    print("X", x[1,0])
    print("E", E[1,0])
    print("XE", (x * E)[1,0])
    # multiply each frame of the white noise by the interpolated frame of the input's noise (x)
    return x * E # element wise multiplication

     