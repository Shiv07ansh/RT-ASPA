#include <stdio.h>
#include "dma_ringbuf.h"
#include "rt_aspa.h"

#define FRAME_SIZE_BYTES (RT_ASPA_FFT_SIZE * sizeof(int16_t))

// Static Workspace Memory (Zero Dynamic Allocations)
static float windowed_pcm_buf[RT_ASPA_FFT_SIZE];
static rt_aspa_fft_t fft_workspace;

void app_main(void) {
    dma_ringbuf_handle_t ringbuf;
    uint8_t *dma_frame_ptr = NULL;

    // 1. Initialize ESPRIT Zero-Copy DMA Driver
    dma_ringbuf_init(&ringbuf, FRAME_SIZE_BYTES);
    dma_ringbuf_start(&ringbuf);

    printf("[RT-ASPA] Pipeline active. Ingesting I2S audio frames...\n");

    while (1) {
        // 2. Fetch populated buffer pointer from ESPRIT DMA engine
        if (dma_ringbuf_get_next_frame(&ringbuf, &dma_frame_ptr, 1000) == 0) {
            
            // Zero-copy cast directly to int16 PCM array
            const int16_t *pcm_stream = (const int16_t *)dma_frame_ptr;

            // 3. Step A: Windowing
            rt_aspa_apply_hanning_window(pcm_stream, windowed_pcm_buf, RT_ASPA_FFT_SIZE);

            // 4. Step B: Compute Spectrogram (FFT Magnitudes)
            rt_aspa_fft_process(&fft_workspace, windowed_pcm_buf);

            // 5. Step C: Extract Fundamental Pitch (F0)
            float pitch_hz = rt_aspa_extract_fundamental_pitch(&fft_workspace, RT_ASPA_SAMPLE_RATE_HZ);

            printf("[RT-ASPA] Frame Processed | Peak Bin[10]: %.2f | Pitch (F0): %.2f Hz\n",
                   fft_workspace.magnitude[10], pitch_hz);
        }
    }
}