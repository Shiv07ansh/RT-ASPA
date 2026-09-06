#ifndef RT_ASPA_H
#define RT_ASPA_H

#include "rt_aspa_config.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    float real[RT_ASPA_FFT_SIZE];
    float imag[RT_ASPA_FFT_SIZE];
    float magnitude[RT_ASPA_MAG_BINS];
} rt_aspa_fft_t;

// Public API
void rt_aspa_apply_hanning_window(const int16_t *pcm_in, float *windowed_out, size_t len);
void rt_aspa_fft_process(rt_aspa_fft_t *fft, const float *windowed_pcm);
float rt_aspa_extract_fundamental_pitch(const rt_aspa_fft_t *fft, uint32_t sample_rate_hz);

#endif // RT_ASPA_H