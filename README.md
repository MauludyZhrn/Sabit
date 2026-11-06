# Sabit

**Sabit** (Automated Precision for Lunar Sighting) is a high-performance Windows desktop application engineered for astronomical crescent moon (*Hilal*) observation, real-time image processing, ephemeris calculations, and computer vision-assisted lunar detection.

Developed as part of a Master's thesis in *Ilmu Falak* (Islamic Astronomy / Celestial Mechanics), Sabit provides an integrated observation workstation designed to address the physical and computational challenges of young crescent detection during daytime and civil twilight.

---

## Scientific Purpose

Young lunar crescent (*Hilal*) observation presents severe observational difficulties:
1. **Low Contrast Ratio**: The luminance of a young crescent moon is often fractions of a percent above the bright twilight or daylight sky background.
2. **Short Observational Window**: Between local sunset and moonset (lag time), observers often have only 15 to 40 minutes under rapidly changing atmospheric conditions.
3. **Rigorous Criteria Verification**: Determining visibility requires precise topocentric coordinates and alignment with established criteria (e.g., modern MABIMS criteria: elongation >= 6.4 degrees, altitude >= 3.0 degrees; Danjon limit; and Odeh criteria).

Sabit integrates real-time ephemeris modeling, multi-driver astronomical hardware control, low-latency image enhancement pipelines, and on-device neural network inference into a unified platform to assist observers and researchers in systematic data recording and analysis.

> **Note on Research Status**: The detection algorithms and ephemeris models implemented in this software represent academic research methods developed for thesis evaluation. They do not claim independent official validation and are intended for research, field trials, and scientific documentation.

---

## Main Features

### 1. Multi-Driver Astronomical Camera Support
- **DirectShow / UVC**: Full support for HDMI capture cards, DSLR virtual webcams (e.g., Canon EOS Webcam Utility), and standard webcams with configurable resolutions (up to 1080p60 / 4K) and low-latency buffer management (`CAP_PROP_BUFFERSIZE = 1`).
- **Dedicated Astronomy Cameras**: Native C/C++ SDK integrations for ZWO ASI and QHYCCD cameras with hardware gain, exposure, temperature, and ROI controls.
- **ASCOM Platform**: Compatibility with ASCOM-compliant camera drivers and DSLR ASCOM bridges.
- **Synthetic Simulator**: Built-in ephemeris-driven synthetic camera simulator for testing, offline verification, and demonstration.

### 2. Real-Time Image Processing and Live Stacking
- **Zero-Latency Pipeline**: Decoupled multi-threaded architecture separating camera acquisition, UI rendering, disk recording, and AI analysis.
- **Contrast Enhancement**: Real-time MTF (Midtone Transfer Function) auto-contrast stretching, Contrast Limited Adaptive Histogram Equalization (CLAHE), and unsharp masking.
- **Live Stacking Engine**: Real-time frame accumulation supporting Mean and Sigma-Clipped Average stacking algorithms with sub-pixel phase correlation motion alignment.
- **Calibration Engine**: Dark frame subtraction and Flat field division wizard with master frame generation.

### 3. Ephemeris and Islamic Astronomical Calculations
- **Real-Time Topocentric Ephemeris**: Continuous calculation of Solar and Lunar altitudes, azimuths, right ascension, declination, elongation angle, crescent width (W), and illumination fraction based on observer GPS coordinates and elevation.
- **Visibility Criteria Assessment**: Real-time evaluation against MABIMS (new criteria: altitude >= 3 deg, elongation >= 6.4 deg), Danjon limit, and Odeh visibility models.
- **Dual Calendar System**: Synchronized Gregorian and Hijri date displays with automatic sunset/moonset event countdowns.

### 4. Neural Network-Assisted Crescent Detection
- **Edge AI Inference**: Embedded ONNX Runtime engine executing real-time object detection models specifically trained for young crescent detection.
- **Visual Overlays**: Dynamic bounding boxes, confidence score telemetry, and detection logging without blocking the high-frame-rate preview.

### 5. Scientific Recording and Official Reporting
- **Multi-Format Capture**: Lossless 16-bit scientific FITS capture with comprehensive astronomical WCS/ephemeris metadata headers, uncompressed AVI/MP4 video streams, and lossless PNG/TIFF snapshots.
- **Session Management**: Session-based folder hierarchies accompanied by JSON/CSV capture manifests and hardware telemetry logs.
- **Official Observation PDF Reports**: Built-in reporting generator producing standardized observation reports suitable for institutional documentation (e.g., Ministry of Religious Affairs / Sidang Isbat).

---

## Technology Stack

- **Language**: C++20 (ISO/IEC 14882:2020)
- **GUI Framework**: Qt 6 (Widgets, Core, Gui, Svg)
- **Styling**: Qlementine custom style engine and Fluent design UI components
- **Image Processing**: OpenCV 4.x / Native SIMD-optimized processing pipelines
- **Machine Learning**: Microsoft ONNX Runtime C API
- **Build System**: CMake 3.22+
- **Camera Interfaces**: DirectShow, Media Foundation, ASCOM COM API, ZWO ASI SDK, QHYCCD SDK
- **Data Formats**: FITS (Flexible Image Transport System), JSON (nlohmann/json), CSV, MP4, AVI

---

## Architecture Overview

Sabit is architected as an asynchronous, decoupled, multi-threaded application designed to prevent UI freezes and dropped frames:

```
+-----------------------------------------------------------------------+
|                              Sabit UI                                 |
|  (Qt 6 GUI Thread: Main Window, Panels, Video Viewport, Badges)       |
+-----------------------------------------------------------------------+
             ^                                      ^
             | (Signals / Slots)                    | (LatestSlot Slot)
+-----------------------------------------------------------------------+
|                           AppController                               |
|        (Central Coordinator, Session Manager, Pipeline Dispatcher)    |
+-----------------------------------------------------------------------+
        |                     |                       |
        v                     v                       v
+------------------+  +--------------------+  +-------------------------+
| Camera Worker    |  | Processing Worker  |  | AI Inference Worker     |
| (ICamera Drivers)|  | (Live Stacker,     |  | (ONNX Runtime,          |
| - UVC / DShow    |  |  CLAHE, Stretch,   |  |  YOLO Hilal Model,      |
| - ZWO ASI        |  |  Calibration)      |  |  Confidence Filter)     |
| - QHYCCD         |  +--------------------+  +-------------------------+
| - ASCOM / Sim    |          |
+------------------+          v
        |             +--------------------+
        v             | Recording Worker   |
+------------------+  | (FITS 16-bit, AVI, |
| RingBuffer       |  |  MP4, PNG, TIFF)   |
| (Zero-Allocation)|  +--------------------+
+------------------+
```

---

## Requirements

### Operating System
- Microsoft Windows 10 (version 21H2 or later) 64-bit or Windows 11 64-bit.

### Hardware Prerequisites
- **Processor**: Intel Core i5 (8th Gen+) / AMD Ryzen 5 or higher recommended.
- **Memory**: Minimum 8 GB RAM (16 GB recommended for multi-frame high-resolution live stacking).
- **Storage**: Fast SSD recommended for continuous FITS and video stream recording.
- **Interface**: USB 3.0 / USB 3.1 Gen 1 ports for astronomical cameras and HDMI capture devices.

### Development Prerequisites
- **Compiler**: MinGW-w64 (GCC 13+) or Microsoft Visual Studio 2022 (MSVC v143, C++20).
- **Build System**: CMake 3.22 or higher.
- **Framework**: Qt 6.5+ (Widgets, Core, Gui, Svg).
- **Libraries**: OpenCV 4.x, ONNX Runtime (C API), nlohmann_json, spdlog.

---

## Build Instructions

### 1. Clone the Repository

```bash
git clone https://github.com/MauludyZhrn/Sabit.git
cd Sabit
```

### 2. Configure Environment

Ensure that your Qt6 and CMake toolchains are located in your system `PATH`, or provide their explicit paths during configuration:

```powershell
# Example for MinGW64 + Qt 6.8:
$env:PATH = "D:\Qt\6.8.0\mingw_64\bin;D:\mingw64\bin;$env:PATH"
```

### 3. Configure the Build

Generate build files using CMake:

```powershell
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

Or when using MSVC:

```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
```

### 4. Compile

Build the main executable and accompanying libraries:

```powershell
cmake --build build --config Release --parallel
```

### 5. Run Tests

Verify core algorithms and components via CTest:

```powershell
ctest --test-dir build --output-on-failure
```

---

## Project Directory Structure

```
Sabit/
├── assets/                     # Application icons, logos, and UI SVG resources
├── cmake/                      # Custom CMake helper modules and toolchain configs
├── docs/                       # Specifications, user manuals, and technical notes
├── include_fix/                # Cross-compiler compatibility headers
├── models/                     # Production ONNX neural network models
│   └── hilal_v1.onnx          # Production Hilal detection model
├── src/                        # C++ source code
│   ├── app/                    # Central application controller and state
│   ├── astro/                  # Ephemeris, coordinates, and criteria calculations
│   ├── camera/                 # Multi-driver camera hardware abstraction
│   │   ├── ascom/              # ASCOM COM driver integration
│   │   ├── qhy/                # QHYCCD native SDK driver
│   │   ├── simulator/          # Ephemeris-driven synthetic camera
│   │   ├── uvc/                # DirectShow / MediaFoundation camera backend
│   │   └── zwo/                # ZWO ASI native SDK driver
│   ├── core/                   # Core data structures (RingBuffer, LatestSlot, Logger)
│   ├── detect/                 # ONNX Runtime detector and visual overlays
│   ├── imaging/                # Stacking, stretching, CLAHE, calibration engines
│   ├── ui/                     # Qt6 UI implementation (dialogs, panels, widgets)
│   └── main.cpp                # Application entry point
├── tests/                      # Unit and integration test suites
├── tools/                      # Validation scripts and pre-commit checks
├── vendor/                     # Third-party dependencies and header libraries
│   ├── onnxruntime/            # ONNX Runtime headers
│   ├── qlementine/             # Qlementine UI engine
│   ├── qhy/                    # QHYCCD headers
│   └── zwo/                    # ZWO ASI SDK headers
├── .antigravityrules           # Development rules and coding guidelines
├── .gitignore                  # Git repository exclusion rules
├── CMakeLists.txt              # Top-level CMake configuration
├── CMakePresets.json           # Standardized CMake build presets
├── settings.example.json       # Template application configuration
└── vcpkg.json                  # vcpkg package manifest
```

---

## Configuration

Sabit utilizes a structured JSON configuration file (`settings.json`) located alongside the executable or in the application data directory. A template is provided in `settings.example.json`:

```json
{
  "schema": 2,
  "general": {
    "language": "en",
    "theme": "dark",
    "prevent_sleep": true
  },
  "site": {
    "preset": "Jakarta",
    "latitude": -6.2088,
    "longitude": 106.8456,
    "elevation_m": 10.0,
    "timezone": "Asia/Jakarta",
    "refraction": false
  },
  "astro": {
    "criteria": "MABIMS-baru",
    "calendar": "rukyat_hilal_indonesia"
  },
  "capture": {
    "output_dir": "Captures",
    "format": "fits",
    "filename_pattern": "{DATE}_{TIME}_{TARGET}_{SEQ}"
  },
  "ai": {
    "model": "models/hilal_v1.onnx",
    "confidence": 0.5,
    "provider": "cpu"
  }
}
```

---

## License Status

**License Status: To be determined.**

All rights reserved. This repository contains academic research and software developed as part of a Master's degree thesis in Ilmu Falak. Proprietary hardware SDK headers (ZWO, QHYCCD) remain under their respective owners' copyright and licenses.
