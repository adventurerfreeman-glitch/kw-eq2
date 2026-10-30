# KW Tilt Lean EQ  (VST3, Windows 64-bit)

Provisional model of the Tilt / Lean + Mids section of the K/W-derived boost.
It is a tone-shaping EQ for trying ranges, not a circuit emulation of the pedal.

## Controls
| Control | Does |
|---|---|
| TILT (0-1, 0.5 = centre) | General darker/brighter balance (+/-1.2 dB across the band) AND the strength of the Lean asymmetry (none at centre, full at either end) |
| MIDS (0-1, 0.5 = centre) | Bell at the pivot, +/-5.5 dB. Turn right to boost and reveal more of the Lean skirt, left to cut |
| VOLUME | Output trim, +/-12 dB |
| PIVOT | DARK = 550 Hz, BRIGHT = 1.2 kHz centre |
| LEAN | LOW = long skirt toward lower frequencies, HIGH = toward higher |
| WIDTH | BROAD (Q 0.55) or FOCUSED (Q 1.3) bell |
| BYPASS | Clean bypass |

All the numbers are named constants at the top of `Source/KwEq.h`
(`kMidsRangeDb`, `kTiltRangeDb`, `kPivotDark`, `kQBroad`, `kSkirtShift`, ...).

## Build route A: GitHub Actions (no tools to install)
1. Create a free GitHub account and a new empty repository.
2. Upload the contents of this folder (keep the `.github/workflows` folder).
3. Open the repo's **Actions** tab, choose **Build Windows VST3**, click **Run workflow**.
4. When it finishes (~5-10 min), download the artifact `KW-Tilt-Lean-EQ-VST3-win64`.
5. Copy the folder `KW Tilt Lean EQ.vst3` into `C:\Program Files\Common Files\VST3\`.
6. In Cakewalk: Preferences > File > VST Settings > Scan, then add the plugin to a track's FX bin.

## Build route B: your own PC
Install Visual Studio 2022 Community (workload "Desktop development with C++") and CMake, then in a Developer prompt:
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --target KWTiltLeanEQ_VST3
The plugin ends up in build\KWTiltLeanEQ_artefacts\Release\VST3\.

## Check the maths
Add -DKW_BUILD_TESTS=ON and build KWResponseTest to print the response table.
