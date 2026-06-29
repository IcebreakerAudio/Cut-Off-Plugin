# Cut-Off Audio Plugin

<img width="1486" height="810" alt="Screenshot 2026-06-11 235643" src="https://github.com/user-attachments/assets/91c8c758-a64a-4eee-819a-9ccf94a524ad" />

A VST3, AU and Standalone audio plugin designed for frequency shaping and dynamic control. Cut-Off combines filtering and compression with a sleek, real-time reactive neon spectrum visualizer.

## 🎛️ Features & Functionality

Cut-Off is designed for quick, visual sound sculpting. The plugin processes audio through a serial chain: **Filters ➔ Dynamics ➔ Output**.

* **High-Pass & Low-Pass Filters (HPF / LPF):** State-Variable zero-delay feedback (TPT) filters. Sweep the neon knobs to seamlessly carve out muddy lows or harsh highs.
* **Dynamics Module:** A built-in compressor to tame transients or glue the filtered signal back together. Features adjustable **Attack**, **Release**, and **Threshold**. Ratio is fixed at 4:1, and stereo channels are not linked.
* **Make-Up Gain:** Clean output gain stage to compensate for volume lost during heavy filtering or compression.
* **Real-Time Spectrum Analyzer:** The centerpiece display provides instant visual feedback. It bridges the audio and UI threads to render a highly optimized, logarithmic cyan waveform that dances with the audio in real time.

## ⚙️ Implementation Details

* **Core Stack:** C++17, JUCE Framework.
* **Build System:** Modern CMake. Bypasses legacy `JuceHeader.h` and Projucer.
* **DSP Engine:** * Powered by the native `juce::dsp` module.
    * Thread-safe parameter management handled via `AudioProcessorValueTreeState` (APVTS).
* **Real-Time Spectrum Visualizer:**
    * **Audio Thread:** Feeds processed mono-summed audio samples into a lock-free FIFO buffer.
    * **Analysis Thread:** Performs a Fast Fourier Transform to convert from the time domain to the frequency domain.
    * **UI Thread:** Uses a 60Hz `juce::Timer` to read from the Analysis Thread.
    * **Rendering:** Converts linear FFT data to a logarithmic scale (mimicking human hearing). Drawn dynamically using `juce::Path`, and filled with custom translucent gradients.
* **Vector UI:**
    * Default JUCE aesthetics overridden by a custom `LookAndFeel` class generating math-based neon vector rotary sliders.
    * SVG assets (logos) are serialized and compiled directly into the binary via CMake `juce_add_binary_data`. The plugin relies on zero external visual files at runtime.

## 🛠️ Build Instructions

**Prerequisites:**
* CMake (3.15+)
* C++ Compiler (MSVC / Clang / GCC)
* *Note: JUCE is handled automatically by CMake. No separate installation required.*

**Compilation:**
```bash
# 1. Configure the build directory
cmake -B build

# 2. Compile the plugin (Release mode recommended for performance)
cmake --build build --config Release
