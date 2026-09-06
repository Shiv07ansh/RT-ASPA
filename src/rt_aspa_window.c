#include "rt_aspa.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void rt_aspa_apply_hanning_window(const int16_t *pcm_in, float *windowed_out, size_t len) {
    for (size_t i = 0; i < len; i++) {
        float hanning_mult = 0.5f * (1.0f - cosf((2.0f * M_PI * (float)i) / (float)(len - 1)));
        // Normalize int16 PCM [-32768, 32767] to float [-1.0, 1.0] and apply window
        windowed_out[i] = ((float)pcm_in[i] / 32768.0f) * hanning_mult;
    }
}