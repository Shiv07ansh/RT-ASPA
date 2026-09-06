#include "rt_aspa.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static uint16_t bit_reverse_index(uint16_t index, uint8_t log2n) {
    uint16_t reversed = 0;
    for (uint8_t i = 0; i < log2n; i++) {
        if (index & (1 << i)) {
            reversed |= (1 << ((log2n - 1) - i));
        }
    }
    return reversed;
}

void rt_aspa_fft_process(rt_aspa_fft_t *fft, const float *windowed_pcm) {
    uint16_t n = RT_ASPA_FFT_SIZE;
    uint8_t log2n = RT_ASPA_LOG2_FFT_SIZE;

    // 1. Bit-Reversal Sorting
    for (uint16_t i = 0; i < n; i++) {
        uint16_t rev = bit_reverse_index(i, log2n);
        fft->real[rev] = windowed_pcm[i];
        fft->imag[rev] = 0.0f;
    }

    // 2. Cooley-Tukey Radix-2 DIT Butterfly Loop
    for (uint16_t len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * M_PI / (float)len;
        float wlen_real = cosf(angle);
        float wlen_imag = sinf(angle);

        for (uint16_t i = 0; i < n; i += len) {
            float w_real = 1.0f;
            float w_imag = 0.0f;

            for (uint16_t j = 0; j < len / 2; j++) {
                uint16_t u_idx = i + j;
                uint16_t v_idx = i + j + (len / 2);

                float v_real_rot = fft->real[v_idx] * w_real - fft->imag[v_idx] * w_imag;
                float v_imag_rot = fft->real[v_idx] * w_imag + fft->imag[v_idx] * w_real;

                fft->real[v_idx] = fft->real[u_idx] - v_real_rot;
                fft->imag[v_idx] = fft->imag[u_idx] - v_imag_rot;
                fft->real[u_idx] += v_real_rot;
                fft->imag[u_idx] += v_imag_rot;

                float next_w_real = w_real * wlen_real - w_imag * wlen_imag;
                float next_w_imag = w_real * wlen_imag + w_imag * wlen_real;
                w_real = next_w_real;
                w_imag = next_w_imag;
            }
        }
    }

    // 3. Compute Positive-Frequency Spectrogram Magnitudes
    for (uint16_t i = 0; i < RT_ASPA_MAG_BINS; i++) {
        fft->magnitude[i] = sqrtf(fft->real[i] * fft->real[i] + fft->imag[i] * fft->imag[i]);
    }
}