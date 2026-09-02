#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "portaudio.h"
#include "fftw3.h"

#define SAMPLE_RATE 44100
#define BAND_LOW 100
#define BAND_HIGH 500
#define FFT_SIZE 4096 // Augmentation de la taille de la FFT pour améliorer la résolution fréquentielle
#define SPEED_OF_SOUND 343.0 // Vitesse du son en m/s
#define DISTANCE 0.5         // Distance entre l'enceinte et le micro en m
#define DELAY (DISTANCE / SPEED_OF_SOUND) // Temps de propagation du son

PaStream *stream;

typedef struct {
    fftw_complex *input, *output;
    fftw_plan plan;
    int fft_size;
    double detected_frequency;
    float *buffer;
    float low_freq;
    float high_freq;
    double max_amplitude;
} FFTBand;

// Appliquer une fenêtre de Hanning aux données
void applyHanningWindow(float *buffer, int size) {
    for (int i = 0; i < size; i++) {
        buffer[i] *= 0.5 * (1 - cos(2 * M_PI * i / (size - 1)));
    }
}

// Fonction pour effectuer une FFT sur une seule bande
void computeFFT(FFTBand *band) {
    int min_index = (int)(band->low_freq * band->fft_size / SAMPLE_RATE);
    int max_index = (int)(band->high_freq * band->fft_size / SAMPLE_RATE);

    applyHanningWindow(band->buffer, band->fft_size); // Appliquer une fenêtre de Hanning

    for (int i = 0; i < band->fft_size; i++) {
        band->input[i][0] = (double)band->buffer[i];
        band->input[i][1] = 0.0;
    }

    fftw_execute(band->plan);

    band->max_amplitude = 0.0;
    band->detected_frequency = -1;

    // Détection de fréquence avec interpolation spectrale
    for (int i = min_index; i <= max_index; i++) {
        double mag = sqrt(band->output[i][0] * band->output[i][0] + band->output[i][1] * band->output[i][1]);
        if (mag > band->max_amplitude) {
            if (i > 0 && i < band->fft_size / 2 - 1) {
                double left_mag = sqrt(band->output[i - 1][0] * band->output[i - 1][0] + band->output[i - 1][1] * band->output[i - 1][1]);
                double right_mag = sqrt(band->output[i + 1][0] * band->output[i + 1][0] + band->output[i + 1][1] * band->output[i + 1][1]);

                double delta = 0.5 * (left_mag - right_mag) / (left_mag - 2 * mag + right_mag);
                band->detected_frequency = ((double)i + delta) * SAMPLE_RATE / band->fft_size;
            } else {
                band->detected_frequency = (double)i * SAMPLE_RATE / band->fft_size;
            }
            band->max_amplitude = mag;
        }
    }
}

// Fonction pour générer le signal anti-bruit avec un décalage de phase
void generateAntiNoise(float *outputBuffer, double frequency, double max_amplitude, unsigned long framesPerBuffer) {
    static double phase = 0.0;

    double period = 1.0 / frequency;
    double phase_shift = fmod(DELAY * frequency * 2 * M_PI, 2 * M_PI);

    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        double time = (double)i / SAMPLE_RATE;
        outputBuffer[i] = (float)(max_amplitude * sin(2 * M_PI * frequency * time + phase + phase_shift));
    }

    phase += 2 * M_PI * frequency * (double)framesPerBuffer / SAMPLE_RATE;
    phase = fmod(phase, 2 * M_PI);
}

// Callback audio principal
static int audioCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {
    const float *inBuffer = (const float *)inputBuffer;
    float *outBuffer = (float *)outputBuffer;
    FFTBand *band = (FFTBand *)userData;

    band->buffer = (float *)inBuffer;
    computeFFT(band);

    if (band->detected_frequency != -1) {
        generateAntiNoise(outBuffer, band->detected_frequency, band->max_amplitude, framesPerBuffer);
    } else {
        for (unsigned long i = 0; i < framesPerBuffer; i++) {
            outBuffer[i] = 0.0f;
        }
    }

    return paContinue;
}

int main() {
    PaError err;

    FFTBand band;
    band.fft_size = FFT_SIZE;
    band.low_freq = BAND_LOW;
    band.high_freq = BAND_HIGH;

    band.input = fftw_malloc(sizeof(fftw_complex) * band.fft_size);
    band.output = fftw_malloc(sizeof(fftw_complex) * band.fft_size);
    band.plan = fftw_plan_dft_1d(band.fft_size, band.input, band.output, FFTW_FORWARD, FFTW_MEASURE);

    err = Pa_Initialize();
    if (err != paNoError) {
        printf("Erreur lors de l'initialisation de PortAudio: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    err = Pa_OpenDefaultStream(&stream, 1, 1, paFloat32, SAMPLE_RATE, FFT_SIZE, audioCallback, &band);
    if (err != paNoError) {
        printf("Erreur lors de l'ouverture du flux audio: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("Erreur lors du démarrage du flux audio: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    printf("Réduction de bruit en cours...\n");
    while (1) {
        Pa_Sleep(100);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    fftw_destroy_plan(band.plan);
    fftw_free(band.input);
    fftw_free(band.output);

    return 0;
}
