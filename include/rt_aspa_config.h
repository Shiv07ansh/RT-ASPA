#ifndef RT_ASPA_CONFIG_H
#define RT_ASPA_CONFIG_H

#include <stdint.h>

#define RT_ASPA_FFT_SIZE        512
#define RT_ASPA_SAMPLE_RATE_HZ  16000
#define RT_ASPA_LOG2_FFT_SIZE   9       // 2^9 = 512
#define RT_ASPA_MAG_BINS        (RT_ASPA_FFT_SIZE / 2)

#endif // RT_ASPA_CONFIG_H