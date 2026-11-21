# HILAL-DETECT V2 Dual-Model Integration in Sabit

## 1. Overview & Model Architectures

Sabit integrates two trained single-class YOLOv8 (`hilal`) ONNX object detection models from `D:\Projects\HILAL-DETECT-V2-TRAINING` alongside the preserved legacy `hilal_v1.onnx` model:

1. **Primary Model -- `yolov8s` (`YOLOv8s Small`)**
   - Role: Default production model (`YOLOv8s (Primary - Recommended)`).
   - Architecture: YOLOv8s static-shape ONNX export (`opset 17`, `embedded_nms: false`).
   - Tuned defaults: Confidence threshold `0.30`, NMS IoU threshold `0.45`.

2. **Alternative Lightweight Model -- `yolov8n` (`YOLOv8n Nano`)**
   - Role: Lightweight alternative (`YOLOv8n (Lightweight Alternative)`) for lower CPU latency.
   - Architecture: YOLOv8n static-shape ONNX export (`opset 17`, `embedded_nms: false`).
   - Tuned defaults: Confidence threshold `0.75`, NMS IoU threshold `0.45`.

Both models can be switched at runtime from the Sabit top toolbar (`AiModelCombo`) without restarting the application, automatically loading the target ONNX model (avoiding duplicate session reloads if already active), updating confidence and NMS thresholds, and updating telemetry and toast notifications.

---

## 2. Model Comparison

| Property | YOLOv8s (Primary) | YOLOv8n (Alternative) | Legacy `hilal_v1` (Preserved) |
| :--- | :--- | :--- | :--- |
| Profile ID | `yolov8s` | `yolov8n` | `hilal_v1` |
| UI Selection Label | `YOLOv8s (Primary - Recommended)` | `YOLOv8n (Lightweight Alternative)` | Legacy fallback only |
| Model File | `models/hilal/yolov8s/hilal_yolov8s_640.onnx` | `models/hilal/yolov8n/best.onnx` | `models/hilal_v1.onnx` |
| File Size | 44,746,477 bytes | 12,773,193 bytes | 12,238,446 bytes |
| SHA-256 | `c83c1f3b1f3626d7de84c5f25a87ce010a992cac1aba577828f523db3fcde0bc` | `0f3281bfbf96dad2860ee1747e833349f9db878aa48cede60d09cd75fc93fc79` | `e8b8db0323e8e895b2c97837a2c213d751f85ee0ac162155babfc1bf53c52451` |
| ONNX Opset | 17 | 17 | 12 |
| Input Tensor | `images` `[1, 3, 640, 640]` float32 | `images` `[1, 3, 640, 640]` float32 | `images` `[1, 3, 640, 640]` float32 |
| Output Tensor | `output0` `[1, 5, 8400]` float32 | `output0` `[1, 5, 8400]` float32 | `output0` `[1, 5, 8400]` float32 |
| Default Confidence | `0.30` | `0.75` | `0.40` (legacy config) |
| NMS IoU Threshold | `0.45` | `0.45` | `0.45` |
| Embedded NMS | `false` (Host C++ NMS) | `false` (Host C++ NMS) | `false` (Host C++ NMS) |
| Training Verification Parity (8 samples) | 8/8 counts match, max score diff `0.000192`, max box diff `0.0249 px` | 8/8 counts match, max score diff `4.3e-05`, max box diff `0.0234 px` | - |
| Sabit C++ CPU Latency (`sample_input.jpg`) | `138.664 ms` | `54.0181 ms` | - |
| Sabit C++ Sample Score (`sample_input.jpg`) | `0.585283` (ref `0.585283`, diff `1.19209e-07`) | `0.8134` (ref `0.8134`, diff `5.96046e-08`) | - |

---

## 3. Model & Configuration File Paths

All artifacts remain untouched in `D:\Projects\HILAL-DETECT-V2-TRAINING`. Deployed runtime copies in Sabit are stored under `models/` (and mirrored under `dist/package/Sabit/models/`):

- `models/hilal/model_profiles.json` -- Dual-model profile registry (`default_model: "yolov8s"`), confidence/NMS defaults, tensor names, and expected SHA-256 digests.
- `models/manifest.json` -- Updated to `"schema": 2`, retaining `hilal_v1` for backward compatibility and adding `yolov8s` and `yolov8n`.
- **YOLOv8s deployment directory** (`models/hilal/yolov8s/`):
  - `hilal_yolov8s_640.onnx`
  - `checksums.sha256`
  - `export_report.md`
  - `onnx_contract.json`, `onnx_verification.json`
  - `metadata/classes.txt`, `metadata/model_metadata.json`, `metadata/preprocess.json`, `metadata/postprocess.json`
  - `samples/sample_input.jpg`, `samples/expected_output.json`
  - `docs/DATASET_ATTRIBUTION.md`, `docs/INTEGRATION_SABIT_QT6.md`, `docs/ONNX_INPUT_OUTPUT_CONTRACT.md`
- **YOLOv8n deployment directory** (`models/hilal/yolov8n/`):
  - `best.onnx`
  - `export_report.md`
  - `onnx_contract.json`, `onnx_verification.json`
  - `metadata/classes.txt`, `metadata/model_metadata.json`, `metadata/preprocess.json`, `metadata/postprocess.json`
  - `samples/sample_input.jpg`, `samples/expected_output.json`
- **Preserved legacy files** (`models/`): `hilal_v1.onnx`, `onyx_hilal_v1.onnx`, `onyx_hilal_v1.pt` (never deleted or overwritten).

---

## 4. Preprocessing & Postprocessing Contract

Implemented in `sabit::detect::OnnxDetector` (`src/detect/onnx_detector.h` and `src/detect/onnx_detector.cpp`):

### 4.1 Input Preprocessing (`preprocessImage`)
- **Contract**:
  - Input name: `images`, shape `[1, 3, 640, 640]`, element type `float32`.
  - Color space: `RGB`, tensor layout `NCHW` (`1 * 3 * 640 * 640` floats).
  - Normalization: linear scaling to `[0.0, 1.0]` (`pixel_u8 / 255.0f`; 16-bit camera frames scale `pixel_u16 / 65535.0f`, mono frames replicate across R/G/B).
  - Resize & Letterbox: aspect-preserving bilinear interpolation (`scale = min(640 / orig_h, 640 / orig_w)`), padded equally around borders with constant gray RGB `(114, 114, 114)` (normalized `114.0f / 255.0f`).
- Exact `LetterboxMeta` offsets (`scale`, `dw`, `dh`, `left`, `top`, `orig_w`, `orig_h`) are returned for downstream coordinate restoration.
- `OnnxDetector::load_image_file()` decodes `.jpg`/`.png` reference inputs via Windows WIC (`windowscodecs`, `ole32`) without introducing an OpenCV dependency.

### 4.2 Inference & Output Decoding (`runInference`, `decodeOutput`)
- `runInference()` binds the tensor name read dynamically from the ONNX session (`images` -> `output0`), executes `OrtRun`, and validates the returned element count (`5 * 8400`).
- Output layout `[1, 5, 8400]` is feature-major across 8,400 anchors:
  - Row `0`: `cx` (letterboxed pixel center X)
  - Row `1`: `cy` (letterboxed pixel center Y)
  - Row `2`: `w` (letterboxed box width)
  - Row `3`: `h` (letterboxed box height)
  - Row `4`: `hilal_score` (class 0 confidence in `[0, 1]`)
- `decodeOutput()` filters anchors with `conf >= conf_thresh` and converts `(cx, cy, w, h)` to `[x0, y0, x1, y1]`.

### 4.3 Host NMS & Coordinate Restoration (`applyNMS`, `restoreBoundingBoxes`)
- `applyNMS()` sorts candidates descending by confidence and applies greedy IoU suppression (`iou > iou_thresh`, default `0.45`). Embedded NMS in ONNX is not assumed (`embedded_nms: false`).
- `restoreBoundingBoxes()` maps surviving boxes back to original frame pixels:
  - `orig_x0 = clamp((x0 - dw) / scale, 0, orig_w)`
  - `orig_y0 = clamp((y0 - dh) / scale, 0, orig_h)`
  - `orig_x1 = clamp((x1 - dw) / scale, 0, orig_w)`
  - `orig_y1 = clamp((y1 - dh) / scale, 0, orig_h)`
  - Converts `[orig_x0, orig_y0, orig_x1, orig_y1]` to Sabit's `Detection { label="Hilal", confidence, x, y, width, height, timestamp_utc_ms }`.

---

## 5. Thresholds, Engine API & UI Integration

### 5.1 Confidence Thresholds & Switching Behavior
- **YOLOv8s (`yolov8s`)**: `confidence_threshold = 0.30`, `nms_iou = 0.45`. Lower threshold recovers subtle crescent detections (`~0.585` on the BMKG sample) where `yolov8s` outputs moderate scores.
- **YOLOv8n (`yolov8n`)**: `confidence_threshold = 0.75`, `nms_iou = 0.45`. Calibrated higher threshold prevents false positives while preserving crisp hilal detections (`~0.813` on the BMKG sample).

### 5.2 Detector Interface (`sabit::detect::OnnxDetector`)
- `loadModel(model_path, expected_sha256)` -- Verifies SHA-256 integrity, initializes ORT session under `session_mutex_`, inspects tensor names/shapes, and avoids reloading an already loaded path (`loaded_model_path_`).
- `setActiveModel(model_id_or_path)` -- Resolves profile from `models/hilal/model_profiles.json` (or explicit path), applies profile confidence & NMS thresholds, and loads/switches the session in place.
- `setConfidenceThreshold(float)` / `setNmsThreshold(float)` -- Atomic thread-safe threshold updates.
- `preprocessImage(...)`, `runInference(...)`, `decodeOutput(...)`, `applyNMS(...)`, `restoreBoundingBoxes(...)`, `releaseModel()`.

### 5.3 Sabit Controller & Qt6 UI Integration
- `sabit::app::AppController` (`src/app/app_controller.h/.cpp`): initializes on `yolov8s` by default, exposes `set_active_ai_model()`, `active_ai_profile()`, `ai_conf_threshold()`, `ai_nms_threshold()`, and `ai_execution_provider()`.
- `sabit::ui::MainWindow` (`src/ui/main_window.h/.cpp`):
  - Adds `ai_model_combo_` (`AiModelCombo`) in toolbar Group 4 (`YOLOv8s (Primary - Recommended)` and `YOLOv8n (Lightweight Alternative)`), defaulting to `yolov8s` and `0.30` confidence.
  - Switching the combo box triggers `MainWindow::on_ai_model_changed()`, calling `controller_.set_active_ai_model()`, updating `ai_conf_spin_`, updating tooltips/logs/toasts, and keeping the active ONNX worker thread responsive without UI stalls.
  - Persisted settings in `settings.json` store `"active_model"`, `"model"`, `"confidence"`, `"nms_iou"`, and `"provider"`, with a legacy guard so pre-v2 `settings.json` files referencing `hilal_v1.onnx` do not override the `0.30` `yolov8s` default.

---

## 6. Runtime Requirements & Threading

- **ONNX Runtime**: Bundled `onnxruntime.dll` v1.17.1 (`ORT_API_VERSION 17` loaded dynamically via `LoadLibraryA` from `vendor/onnxruntime/onnxruntime_c_api.h`).
- **Execution Providers**:
  - `CPUExecutionProvider` is always supported and enabled by default.
  - `CUDAExecutionProvider` is appended only when `prefer_cuda` is enabled, ORT reports `CUDAExecutionProvider` in `GetAvailableProviders()`, AND both `nvcuda.dll` and `onnxruntime_providers_cuda.dll` load successfully; otherwise Sabit falls back cleanly to `CPUExecutionProvider`.
- **Threading**: Live video inference runs asynchronously on `OnnxDetector`'s dedicated `worker_thread_` using a latest-frame slot (rate-limited by `max_fps`, default `3.0` FPS) so the Qt6 GUI thread is never blocked. `session_mutex_` serializes `loadModel`/`setActiveModel`/`releaseModel`/`runInference` safely during live switching.

---

## 7. Build & Test Instructions

Environment: Windows 64-bit, MinGW64 (`D:\mingw64\bin`), Qt 6.8.0 (`D:\Qt\6.8.0\mingw_64`), CMake + Ninja.

```powershell
$env:PATH = "D:\mingw64\bin;D:\Qt\6.8.0\mingw_64\bin;" + $env:PATH
cmake --build build-release --parallel 2
ctest --test-dir build-release -C Release --output-on-failure
```

To run the standalone integration suite directly with detailed per-test telemetry:

```powershell
.\build-release\tests\sabit_hardware_ai_tests.exe
```

To verify zero emojis or disallowed Unicode symbols across code, tests, models, and docs:

```powershell
python tools\check_no_emoji.py src
python tools\check_no_emoji.py tests
python tools\check_no_emoji.py models
python tools\check_no_emoji.py docs
```

---

## 8. Validation Results

All 4 CTest targets (`FoundationTests`, `ImagingAstroTests`, `HardwareAiTests`, `AppUiTests`) passed (100% pass rate, total CTest time `9.87 s`).

Specifically, `[Test 7] HILAL-DETECT V2 Dual-Model (YOLOv8s + YOLOv8n) Integration Validation` in `tests/integration/test_hardware_ai.cpp` and `[Test 4]` in `tests/unit/test_app_ui.cpp` validated all 10 required checks:

1. **Model Loading & Integrity**: Both `models/hilal/yolov8s/hilal_yolov8s_640.onnx` and `models/hilal/yolov8n/best.onnx` loaded cleanly under `onnxruntime.dll` v1.17.1 with SHA-256 verification passing.
2. **Tensor Input/Output Dimensions**: Both sessions inspected dynamically via ORT C API and verified as input `images` `[1, 3, 640, 640]` and output `output0` `[1, 5, 8400]`.
3. **Preprocessing**: `preprocessImage()` verified on `640x640` `sample_input.jpg` (`scale=1.0`, zero pad) and synthetic `1280x720` frame (`scale=0.5`, `new_unpad=640x360`, `top=140`, `left=0`, border pixels normalized to `114.0f / 255.0f`).
4. **Bounding-Box Decoding**: `decodeOutput()` parsed raw `[1, 5, 8400]` anchor arrays cleanly at both `0.30` and `0.75` thresholds.
5. **Coordinate Restoration**: `restoreBoundingBoxes()` verified inverse letterbox formula `(coord - offset) / scale` on synthetic `1280x720` boxes as well as real decoded `sample_input.jpg` boxes.
6. **Confidence Threshold**: Raising threshold to `0.75` filtered `yolov8s`'s `0.585283` detection as expected, while retaining `yolov8n`'s `0.8134` detection.
7. **NMS**: `applyNMS(..., 0.45f)` verified suppressing overlapping boxes with high IoU while retaining disjoint candidates.
8. **CPU Inference Parity vs `expected_output.json`**:
   - **YOLOv8s (`CPUExecutionProvider`, `138.664 ms`)**:
     - Predicted Conf: `0.585283` vs Reference: `0.585283` (abs diff `1.19209e-07`)
     - Predicted XYXY: `[228.15, 411.362, 378.557, 446.293]` vs Reference: `[228.15, 411.362, 378.557, 446.293]`
   - **YOLOv8n (`CPUExecutionProvider`, `54.0181 ms`)**:
     - Predicted Conf: `0.8134` vs Reference: `0.8134` (abs diff `5.96046e-08`)
     - Predicted XYXY: `[225.998, 410.128, 389.667, 445.58]` vs Reference: `[225.998, 410.128, 389.667, 445.58]`
9. **GPU EP Probe / Fallback**: `prefer_cuda=true` probed ORT providers and CUDA runtime DLLs (`nvcuda.dll`, `onnxruntime_providers_cuda.dll`) safely, falling back to `CPUExecutionProvider` on a CPU-only ORT runtime without errors.
10. **Runtime Model Switching & UI Binding**: `setActiveModel("yolov8s") -> setActiveModel("yolov8n") -> setActiveModel("yolov8n")` (verified duplicate reload skipped) `-> setActiveModel("yolov8s") -> releaseModel()`, plus `AppUiTests` verifying `MainWindow::ai_model_selector()` defaults to `yolov8s` (`conf=0.30`), switches cleanly to `yolov8n` (`conf=0.75`), and restores `yolov8s`.

---

## 9. Known Limitations

1. **Detection Disclaimer**: Both models produce visual hilal detection candidates only and do not independently confirm astronomical or religious/legal (`imkanur rukyat`) hilal visibility.
2. **GPU Acceleration Dependency**: Sabit's distributed runtime bundles CPU `onnxruntime.dll` (v1.17.1). `CUDAExecutionProvider` activates only when a CUDA-enabled ONNX Runtime build (`onnxruntime_providers_cuda.dll`) and NVIDIA CUDA driver (`nvcuda.dll`) are installed alongside the executable.
3. **Host Image Preprocessing Approximation**: Bilinear resize in C++ (`preprocessImage()`) matches OpenCV `INTER_LINEAR` closely, and Windows WIC JPEG decoding shows negligible variance (`< 2e-7` score diff on `sample_input.jpg`), though minor 1-LSB rounding differences vs Python `cv2.imdecode` are theoretically possible on arbitrary compressed inputs.
4. **16-bit Astronomical Frames**: 16-bit camera frames are linearly scaled `/ 65535.0f` before YOLOv8 inference; non-linear auto-stretch in `ImageStretcher` affects display/preview rather than altering the raw camera frame unless enabled upstream of AI submission.
