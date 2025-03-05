## Low-latency Pitch-Shifting with STN decomposition
Semester Project for SMC9 AAU CPH

### Description

This repository hosts the code and resources for implementing a novel real-time pitch-shifting algorithm designed for complex audio signals. The project integrates fuzzy Sines-Transient-Noise (STN) decomposition with specialized processing pipelines for sines, transient, and noise components.

Developed as a VST audio plug-in using the JUCE framework, the system allows precise pitch adjustments across a wide range of semitones. It is optimized for real-time performance and features user-configurable parameters, such as pitch-shift range, decomposition bounds, and FFT size.

### Key Features

- Sines-Transient-Noise (STN) Decomposition: Separates audio into distinct components for targeted processing
- Specialized Algorithms: Noise Morphing and "Vase-Phocoder" for enhanced pitch-shifting quality
- Real-Time Processing: Low-latency implementation for live applications
- User-friendly interface with adjustable parameters for precise sound design

### Evaluation

The system has been tested through blind listening experiments and interviews, comparing its performance to commercial alternatives. While the current version shows artistic potential, ongoing work focuses on improving audio quality, optimizing computational performance, and refining evaluation methodologies.

### Future Plans

- Audio quality enhancements
- Performance optimization
- Improved evaluation strategies

### Set-Up Notes

This project includes 3 external dependecies setup as submodules: webMUSRHA and signalsmith DSP libraries.
