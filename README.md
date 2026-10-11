# ReaperianiBass

**ReaperianiBass** is a stereo/mono harmonic bass enhancer by **Reaperiani**. It uses a low-frequency split, nonlinear harmonic generation, spectral shaping, and a separate original-bass switch to create a stronger perception of low notes on small speakers.

## Relationship to Waves RBass

ReaperianiBass is an independent project **freely inspired by the general operation and bass-enhancement concept of Waves Renaissance Bass (RBass)**. We have not had access to, and have not used, the original source code. The DSP is independently written from general psychoacoustic principles and the user-provided research notes. A few REAPER tone tests compared broad output levels/spectrum against the installed plugin; they were not used to reconstruct its proprietary algorithm. The GUI, artwork, presets, and Reaperiani branding are original; no Waves logos or product assets are included. ReaperianiBass is not affiliated with, sponsored by, or endorsed by Waves.

## Formats and platforms

| Platform | Formats in the release |
| --- | --- |
| Windows x64 | VST3 |
| macOS universal (Apple Silicon + Intel) | VST3, Audio Unit (AU) |
| Linux x86_64 | VST3 |

The first release is built by GitHub Actions and published on the [Releases page](https://github.com/reaperiani/ReaperianiBass/releases). Binaries are unsigned; on macOS, use the normal local security approval flow when opening an unsigned plugin.

## Controls

- **Bass Frequency** — cutoff for the low-frequency source band (35–180 Hz).
- **Harmonics** — amount of generated harmonic signal.
- **Odd / Even** — crossfades between odd-order saturation character and an even-harmonic component.
- **Harmonic Decay** — adjusts the upper spectral limit of the generated harmonics.
- **Dynamics** — level-dependent control of the harmonic branch.
- **Clean** — removes more sub-bass from the nonlinear detector path to reduce low-frequency intermodulation and muddiness.
- **Original Bass** — keeps or removes the original low-pass band while retaining the harmonic-enhancement path.
- **Output** — output trim from -24 to +12 dB.

The editor uses a custom-drawn, late-1990s studio-hardware palette and controls. All artwork and interface code are original to this project.

## Install

Download the archive for your operating system from Releases, unpack it, then copy the plugin bundle/file into the matching system or DAW-specific plugin directory:

- **VST3:** `ReaperianiBass.vst3`
- **AU:** `ReaperianiBass.component` (macOS only)

Rescan plugins in your DAW. Linux packages the VST3 bundle; host system libraries are not bundled.

## Build from source

Requirements: CMake 3.22+, a C++17 compiler, and the platform's normal JUCE GUI/audio development dependencies. CMake fetches JUCE 8.0.15 automatically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

On Apple platforms, the CMake project also generates the AU target. Build the additional format with:

```sh
cmake --build build --config Release --target ReaperianiBass_AU
```

For a Windows REAPER smoke test, `python scripts/install_vst3_user.py install` copies the built VST3 into the current user's standard plug-in folder. Use `python scripts/install_vst3_user.py remove` to remove that test install afterward.

**Temporary license selection:** AGPL-3.0-only for this first release, matching the open-source JUCE 8 licence used to build the plugin. This selection is marked TEMPORANEA in [LICENSE](LICENSE) and may be reviewed for a future release. No commercial JUCE licence is needed for this free, open-source build.

## DSP notes

The input is divided into a low-pass bass band and its complementary residual. The low band is optionally high-passed for cleanliness, then passed through a 4x interpolated nonlinear shaper. A selectable even/odd mix and post-shaper high/low-pass pair shape the generated spectrum. The harmonic branch is dynamically controlled and mixed with the original bands before output trim. The implementation is intentionally a transparent, tweakable approximation rather than a claim about another plugin's internal design.

## License and names

Copyright (C) 2026 Reaperiani. Licensed under the GNU Affero General Public License, version 3 (AGPLv3). See [LICENSE](LICENSE).

“Waves” and “Renaissance Bass” are trademarks of their respective owners and are mentioned only as user-supplied reference points; this project is independent and is not affiliated with or endorsed by them.
