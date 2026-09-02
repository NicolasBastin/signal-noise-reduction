# Active Noise Cancellation (ANC) via LMS Adaptive Filtering 🎧

## Project Overview
This repository contains the source code for my research project (TIPE) on Active Noise Cancellation (ANC). The goal of this project was to understand, mathematically model, and programmatically implement the noise reduction phenomena used in modern ANC headphones. 

The project progressed from a purely mathematical phase-shifting algorithm (using FFT) to a more robust, real-time adaptive filtering approach using the **Least Mean Squares (LMS)** algorithm.

## Features & Evolution
1. **FFT-Based Phase Shifting (`src/fft_noise_reduction.c`):**
   * Real-time audio capture using `PortAudio`.
   * Frequency domain analysis using `FFTW3` with Hanning windowing.
   * Geometric phase-shift calculation based on the physical distance between the microphone and speaker (0.5m).
2. **LMS Adaptive Filter (`src/lms_filter.c`):**
   * Implementation of a 32-order LMS digital filter.
   * Real-time weight adaptation (Step size: 0.01) to continuously minimize the error signal.
   * Combines spectral interpolation (FFT) with adaptive filtering for enhanced accuracy.

## Tech Stack
* **Language:** C
* **Audio I/O Library:** PortAudio
* **Signal Processing Library:** FFTW3

## Results & Documentation
We tested our algorithms in real-world conditions using speakers and a sound level meter. 
* The FFT algorithm achieved an average reduction of **5.21 dB** (from 85 dB to 79.79 dB).
* The LMS algorithm significantly improved the results, achieving an average reduction of **10.21 dB** (from 85 dB to 74.79 dB).

For more details on the experimental setup and the mathematical models, please refer to the `docs/` folder containing the presentation slides and the research report (MCOT) (in French).
