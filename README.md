# RT-ASPA — Real-Time Acoustic Spectrogram & Pitch Analyzer

> **A deterministic, allocation-free C99 DSP engine for real-time spectral analysis and fundamental-frequency estimation on resource-constrained systems.**

**RT-ASPA** (Real-Time Acoustic Spectrogram & Pitch Analyzer) is a portable Digital Signal Processing (DSP) engine written in standard **C99**. It transforms raw time-domain PCM audio into frequency-domain spectral information and estimates the fundamental frequency ($F_0$) of the input signal.

The engine is designed for **embedded and resource-constrained environments**, including ESP32-S3 and ARM Cortex-M microcontrollers, while also providing a desktop host implementation for development and testing.

RT-ASPA forms the DSP layer of the **Kusanagi Embedded Machine Intelligence** ecosystem:

```text
┌──────────────────────┐
│ ESPRIT               │
│ Zero-Copy DMA Input  │
└──────────┬───────────┘
           │ PCM Audio
           ▼
┌──────────────────────┐
│ RT-ASPA              │
│ DSP / Signal Analysis│
│                      │
│ Window → FFT → HPS   │
└──────────┬───────────┘
           │ Spectral Features
           │ + F₀
           ▼
┌──────────────────────┐
│ HANNO                │
│ Edge ML Engine       │
└──────────────────────┘
```

---

## Overview

RT-ASPA processes incoming PCM audio through a fixed-size signal-processing pipeline:

```text
PCM Audio
    │
    ▼
┌───────────────┐
│ Hann Window   │
└───────┬───────┘
        │
        ▼
┌───────────────┐
│ Radix-2 DIT   │
│ FFT           │
└───────┬───────┘
        │
        ▼
┌───────────────┐
│ Magnitude     │
│ Spectrum      │
└───────┬───────┘
        │
        ├──────────────► Spectral Bins
        │
        ▼
┌───────────────┐
│ HPS Pitch     │
│ Estimation    │
└───────┬───────┘
        │
        ▼
      F₀ (Hz)
```

The implementation deliberately avoids dynamic memory allocation. Processing uses fixed-size arrays and caller-provided/static workspaces, making the execution model suitable for embedded systems where predictable memory behavior is important.

---

## Key Features

* **Pure C99 DSP core** — minimal dependencies and portable implementation.
* **Allocation-free processing** — no `malloc()` / `free()` in the signal-processing path.
* **In-place Radix-2 DIT FFT** — Cooley-Tukey FFT with bit-reversal permutation.
* **Hann windowing** — reduces spectral leakage before frequency-domain analysis.
* **Spectral magnitude extraction** — computes positive-frequency magnitude bins.
* **Fundamental-frequency estimation** — Harmonic Product Spectrum (HPS) with spectral peak fallback.
* **Fixed-size workspaces** — designed around predictable memory usage.
* **Embedded-oriented API** — suitable for integration into ESP-IDF and bare-metal-style processing pipelines.
* **Desktop host support** — allows algorithm development and testing without target hardware.
* **CMake build system** — straightforward cross-platform builds and testing.
* **Unit-test structure** — FFT and signal-processing components can be tested independently.
* **Zero-copy pipeline compatibility** — designed to consume PCM buffers supplied by an upstream DMA/ring-buffer subsystem.

---

# Signal Processing Pipeline

## 1. PCM Input & Hann Window

RT-ASPA accepts signed 16-bit PCM samples and normalizes them into floating-point values before applying a Hann window.

The window is defined as:

$$
w[n] =
0.5
\left(
1-\cos\left(\frac{2\pi n}{N-1}\right)
\right)
$$

The resulting signal is:

$$
x_w[n] =
\frac{x[n]}{32768}
w[n]
$$

Windowing reduces discontinuities at the boundaries of the finite analysis frame and therefore reduces spectral leakage.

---

## 2. Radix-2 Decimation-in-Time FFT

The windowed signal is transformed from the time domain into the frequency domain using an in-place **Radix-2 Decimation-in-Time FFT**.

The underlying transform is:

$$
X[k] =
\sum_{n=0}^{N-1}
x[n]e^{-j2\pi kn/N}
$$

The implementation performs:

1. Bit-reversal permutation
2. Radix-2 butterfly stages
3. Complex frequency-domain accumulation
4. Positive-frequency magnitude extraction

For the default configuration:

```text
FFT size:          512 samples
Sample rate:       16,000 Hz
Positive bins:     256
Frequency spacing: 31.25 Hz/bin
```

Frequency resolution is:

$$
\Delta f = \frac{f_s}{N}
$$

so:

$$
\Delta f =
\frac{16000}{512}
=
31.25\text{ Hz}
$$

---

## 3. Spectral Magnitude

For each positive-frequency FFT bin, RT-ASPA computes the magnitude:

$$
|X[k]| =
\sqrt{
\operatorname{Re}(X[k])^2 +
\operatorname{Im}(X[k])^2
}
$$

This produces the spectral representation used by downstream analysis.

---

## 4. Fundamental Frequency Estimation

RT-ASPA estimates the fundamental frequency ($F_0$) using a **Harmonic Product Spectrum (HPS)** approach.

The spectrum is compressed across harmonic multiples:

$$
HPS[i] =
|X[i]|
|X[2i]|
|X[3i]|
$$

The harmonic structure reinforces candidate fundamental frequencies while suppressing many non-fundamental peaks.

To improve robustness for signals where harmonic energy is weak or absent, the implementation also provides a **spectral peak fallback** when the harmonic-product estimate does not produce a reliable candidate.

The resulting frequency estimate is converted from a spectral bin to Hertz:

$$
F_0 =
k\frac{f_s}{N}
$$

---

# Memory & Execution Model

RT-ASPA is designed around a static/fixed-memory processing model.

The DSP path does **not** require:

```c
malloc();
calloc();
realloc();
free();
```

Instead, processing operates on explicitly sized buffers and workspaces.

This provides several advantages for embedded deployment:

* predictable memory consumption
* no heap fragmentation
* explicit buffer ownership
* easier integration with RTOS/DMA pipelines
* simpler lifetime management
* deterministic memory behavior

The project does not currently claim hard real-time execution guarantees; target-specific timing should be established through hardware profiling.

---

# Architecture

```text
rt_aspa/
│
├── include/
│   ├── rt_aspa.h
│   └── rt_aspa_config.h
│
├── src/
│   ├── rt_aspa_window.c
│   ├── rt_aspa_fft.c
│   └── rt_aspa_pitch.c
│
├── main/
│   ├── main.c
│   └── main_desktop.c
│
├── tests/
│   ├── CMakeLists.txt
│   └── test_fft.c
│
├── .github/
│   └── workflows/
│       └── build_tests.yml
│
├── CMakeLists.txt
├── LICENSE
└── README.md
```

### Components

| Component          | Responsibility                                 |
| ------------------ | ---------------------------------------------- |
| `rt_aspa_window.c` | PCM normalization and Hann windowing           |
| `rt_aspa_fft.c`    | Radix-2 FFT and spectral magnitude computation |
| `rt_aspa_pitch.c`  | HPS-based fundamental frequency estimation     |
| `main_desktop.c`   | Host-side simulation and development harness   |
| `main.c`           | Embedded integration example                   |
| `test_fft.c`       | Signal-processing verification tests           |
| `rt_aspa_config.h` | Compile-time DSP configuration                 |

---

# Configuration

Core DSP parameters are configurable at compile time:

| Parameter                | Default | Description                            |
| ------------------------ | ------: | -------------------------------------- |
| `RT_ASPA_FFT_SIZE`       |   `512` | FFT frame size; must be a power of two |
| `RT_ASPA_SAMPLE_RATE_HZ` | `16000` | Input PCM sample rate                  |
| `RT_ASPA_LOG2_FFT_SIZE`  |     `9` | $\log_2(N)$ for FFT indexing           |
| `RT_ASPA_MAG_BINS`       |   `256` | Number of positive-frequency bins      |

For example:

```c
#define RT_ASPA_FFT_SIZE          512
#define RT_ASPA_SAMPLE_RATE_HZ   16000
#define RT_ASPA_MAG_BINS         256
```

---

# API

## `rt_aspa_apply_hanning_window`

```c
void rt_aspa_apply_hanning_window(
    const int16_t *pcm_in,
    float *windowed_out,
    size_t len
);
```

Converts signed 16-bit PCM samples to normalized floating-point values and applies the Hann window.

---

## `rt_aspa_fft_process`

```c
void rt_aspa_fft_process(
    rt_aspa_fft_t *fft,
    const float *windowed_pcm
);
```

Executes the configured Radix-2 DIT FFT and populates the real, imaginary, and positive-frequency magnitude buffers.

---

## `rt_aspa_extract_fundamental_pitch`

```c
float rt_aspa_extract_fundamental_pitch(
    const rt_aspa_fft_t *fft,
    uint32_t sample_rate_hz
);
```

Analyzes the magnitude spectrum using HPS and spectral peak fallback logic to estimate the fundamental frequency in Hertz.

---

# Desktop Usage

RT-ASPA includes a host-side executable for development and algorithm verification.

### Requirements

* GCC, Clang, or another C99-compatible compiler
* CMake 3.16+
* Make, Ninja, or another supported build backend

### Build

```bash
git clone https://github.com/your-username/rt_aspa.git
cd rt_aspa

mkdir build
cd build

cmake ..
cmake --build .
```

Run the host demonstration:

```bash
./rt_aspa_demo
```

The demonstration generates a synthetic test tone and passes it through the complete DSP pipeline.

Example:

```text
[RT-ASPA] Generating 440 Hz Synthetic Audio Signal...
[RT-ASPA] Output Pitch (F0): 437.50 Hz
[RT-ASPA] Expected: ~440.00 Hz
```

The difference is expected because the default 512-point FFT at 16 kHz provides a frequency-bin spacing of 31.25 Hz.

---

# ESP32-S3 Integration

RT-ASPA can be integrated as an ESP-IDF component into an embedded audio pipeline.

Example project structure:

```text
my_project/
├── components/
│   ├── esprit/
│   │   └── ...
│   │
│   └── rt_aspa/
│       ├── include/
│       ├── src/
│       └── CMakeLists.txt
│
└── main/
    └── main.c
```

A typical processing loop can connect a DMA/ring-buffer subsystem directly to RT-ASPA:

```c
uint8_t *dma_frame_ptr = NULL;

while (1) {
    if (dma_ringbuf_get_next_frame(
            &ringbuf,
            &dma_frame_ptr,
            portMAX_DELAY) == 0) {

        const int16_t *pcm_data =
            (const int16_t *)dma_frame_ptr;

        rt_aspa_apply_hanning_window(
            pcm_data,
            windowed_pcm,
            RT_ASPA_FFT_SIZE
        );

        rt_aspa_fft_process(
            &fft_workspace,
            windowed_pcm
        );

        float pitch =
            rt_aspa_extract_fundamental_pitch(
                &fft_workspace,
                RT_ASPA_SAMPLE_RATE_HZ
            );

        printf("Detected Pitch: %.2f Hz\n", pitch);
    }
}
```

This allows RT-ASPA to sit between a low-level audio acquisition subsystem and downstream feature extraction or machine-learning components.

---

# Ecosystem Integration

RT-ASPA is intended to serve as the **signal-processing layer** of a larger embedded intelligence pipeline.

```text
             AUDIO SENSOR
                  │
                  ▼
        ┌───────────────────┐
        │ ESPRIT            │
        │ DMA / Acquisition │
        └─────────┬─────────┘
                  │
                  │ Raw PCM
                  ▼
        ┌───────────────────┐
        │ RT-ASPA           │
        │                   │
        │ Hann Window       │
        │       ↓           │
        │ Radix-2 FFT       │
        │       ↓           │
        │ Spectrum / HPS    │
        │       ↓           │
        │ F₀ Estimation	    │
        └─────────┬─────────┘
                  │
                  │ Spectral Features
                  ▼
        ┌───────────────────┐
        │ HANNO             │
        │ Edge ML / TinyML  │
        └───────────────────┘
```

This separation keeps **signal acquisition, DSP, and machine learning** as independent components that can be developed, tested, and optimized separately.

---

# Design Goals

RT-ASPA is built around several principles:

### Predictable memory

Avoid dynamic allocation and make working memory explicit.

### Portable DSP

Keep the core implementation independent of vendor-specific DSP libraries.

### Hardware-oriented design

Use fixed-size buffers and simple computational primitives suitable for microcontrollers.

### Modular architecture

Separate acquisition, signal processing, and ML layers.

### Host-first development

Allow algorithms to be developed and tested on a desktop before deployment to target hardware.

---

# Current Status

| Component                      | Status             |
| ------------------------------ | ------------------ |
| C99 DSP core                   | Implemented        |
| Hann window                    | Implemented        |
| Radix-2 DIT FFT                | Implemented        |
| Spectral magnitude extraction  | Implemented        |
| HPS pitch estimation           | Implemented        |
| Spectral peak fallback         | Implemented        |
| Static/fixed workspaces        | Implemented        |
| Desktop host harness           | Implemented        |
| CMake build                    | Implemented        |
| Unit-test framework            | Implemented        |
| ESP32-S3 integration           | Integration target |
| Hardware performance profiling | Planned            |

Hardware timing and performance characteristics are intentionally not reported until they have been measured under controlled conditions.

---

# License

RT-ASPA is released under the **MIT License**.
