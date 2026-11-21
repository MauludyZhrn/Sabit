# Sabit C++/Qt6 ONNX Runtime Integration Guide

1. Load `models/hilal_yolov8s_640.onnx` via `Ort::Session` (CPUExecutionProvider or CUDAExecutionProvider).
2. Letterbox input `cv::Mat` (BGR) to `640x640` using pad `(114, 114, 114)` (`cv::copyMakeBorder`), convert BGR->RGB, divide by `255.0f`, and pack into `NCHW` (`1x3x640x640`) `float` buffer.
3. Run `Session::Run` and read `[1, 5, 8400]` output tensor.
4. Iterate over all `8400` anchors: read `score = data[4 * 8400 + i]`; if `score >= conf_threshold`, convert `(cx, cy, w, h)` -> `(x1, y1, x2, y2)` and run `cv::dnn::NMSBoxes`.
5. Map final boxes back to original image coordinates using `x_orig = (x_pad - dw) / scale`, `y_orig = (y_pad - dh) / scale`.
6. Present detections strictly as visual candidates.
