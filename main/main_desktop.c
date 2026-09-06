#include <stdio.h>
#include <math.h>
#include "rt_aspa.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static int16_t synthetic_pcm[RT_ASPA_FFT_SIZE];
static float windowed_pcm_buf[RT_ASPA_FFT_SIZE];
static rt_aspa_fft_t fft_workspace;

int main(void) {
    printf("[RT-ASPA] Generating 440 Hz Synthetic Audio Signal...\n");

    // Generate a 440 Hz sine wave at 16,000 Hz sample rate
    float target_freq = 440.0f;
    for (int i = 0; i < RT_ASPA_FFT_SIZE; i++) {
        float t = (float)i / (float)RT_ASPA_SAMPLE_RATE_HZ;
        synthetic_pcm[i] = (int16_t)(20000.0f * sinf(2.0f * M_PI * target_freq * t));
    }

    // Process pipeline
    rt_aspa_apply_hanning_window(synthetic_pcm, windowed_pcm_buf, RT_ASPA_FFT_SIZE);
    rt_aspa_fft_process(&fft_workspace, windowed_pcm_buf);
    float pitch_hz = rt_aspa_extract_fundamental_pitch(&fft_workspace, RT_ASPA_SAMPLE_RATE_HZ);

    printf("[RT-ASPA] Output Pitch (F0): %.2f Hz (Expected ~440.00 Hz)\n", pitch_hz);
    return 0;
}