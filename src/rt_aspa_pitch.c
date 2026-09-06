#include "rt_aspa.h"
#include <math.h>

float rt_aspa_extract_fundamental_pitch(const rt_aspa_fft_t *fft, uint32_t sample_rate_hz) {
    uint16_t num_bins = RT_ASPA_MAG_BINS;
    float bin_resolution = (float)sample_rate_hz / (float)RT_ASPA_FFT_SIZE; // 16000 / 512 = 31.25 Hz

    // 1. First Pass: Find absolute maximum magnitude peak bin in raw spectrum
    float max_mag = 0.0f;
    uint16_t max_mag_idx = 0;

    // Skip DC offset bin (idx 0)
    for (uint16_t i = 1; i < num_bins; i++) {
        if (fft->magnitude[i] > max_mag) {
            max_mag = fft->magnitude[i];
            max_mag_idx = i;
        }
    }

    // Silence detection: return 0.0 Hz if signal energy is too low
    if (max_mag < 0.01f) {
        return 0.0f;
    }

    // 2. Second Pass: Run HPS with Harmonic Energy Check
    uint16_t max_search_bin = num_bins / 3;
    float max_hps = 0.0f;
    uint16_t max_hps_idx = 0;

    for (uint16_t i = 1; i < max_search_bin; i++) {
        float h1 = fft->magnitude[i];
        float h2 = fft->magnitude[i * 2];
        float h3 = fft->magnitude[i * 3];

        // Only weight harmonics if they contain actual energy above noise floor
        float hps_val = h1 * (h2 > 0.05f * max_mag ? h2 : 1.0f) * (h3 > 0.05f * max_mag ? h3 : 1.0f);

        if (hps_val > max_hps) {
            max_hps = hps_val;
            max_hps_idx = i;
        }
    }

    // 3. Fallback: If HPS detected a sub-harmonic octave, stick to fundamental spectral peak
    uint16_t selected_bin = max_hps_idx;
    if (selected_bin == 0 || (max_mag_idx / selected_bin >= 2 && fft->magnitude[max_mag_idx] > 3.0f * max_hps)) {
        selected_bin = max_mag_idx;
    }

    return (float)selected_bin * bin_resolution;
}