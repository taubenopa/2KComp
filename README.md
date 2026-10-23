# 2KComp

VST3-Compressor für Underground-Rap-Vocals – von **Jxhn2k**.

## Features
- Stereo-linked Feed-Forward-Compressor, Soft Knee
- Peak- oder RMS-Detektor, Sidechain-HPF (20–500 Hz)
- Attack 0.1–100 ms, Release 10–1200 ms, **Auto Release** (Dual-Release, programmabhängig)
- **GRIT** (asymmetrische Tanh-Sättigung) und **AIR** (High-Band-Exciter) auf dem Wet-Signal
- **MIX** für Parallel-Kompression, Input/Makeup/Output
- 6 Presets (`<` `>` im Header)
- UI: Glitch/RGB-Split-Artwork, Transfer-Kurve live, GR-History, Meter, Particles, Scanlines, Ticker

## Bauen
Voraussetzungen: CMake ≥ 3.22, C++17-Compiler (Visual Studio 2022 / Xcode), Git.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Das VST3 liegt danach in `build/TwoKComp_artefacts/Release/VST3/2KComp.vst3`
(Windows: nach `C:\Program Files\Common Files\VST3\` kopieren, macOS: `~/Library/Audio/Plug-Ins/VST3/`).

JUCE wird automatisch per CMake geladen.

## Struktur (alles in einem Ordner, keine Unterordner)
- `Compressor.h` – DSP
- `PluginProcessor.*` – Parameter, Presets, State
- `PluginEditor.*` – komplettes UI + Animationen
- `jxhn2k.png` – Artwork (wird per BinaryData eingebettet)
- `CMakeLists.txt` – Build

## Lizenz-Hinweis
JUCE ist unter AGPLv3 oder kommerziell lizenziert. Für ein veröffentlichtes Plugin bitte die JUCE-Lizenzbedingungen prüfen.
