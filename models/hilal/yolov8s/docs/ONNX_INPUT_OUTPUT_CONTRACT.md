# ONNX Input / Output Contract

- **Input**: `[{'name': 'images', 'shape': [1, 3, 640, 640], 'type': 'tensor(float)'}]` — `NCHW`, `float32`, `RGB`, scaled to `[0.0, 1.0]`, letterboxed to `640x640` with pad `(114, 114, 114)`.
- **Output**: `[{'name': 'output0', 'shape': [1, 5, 8400], 'type': 'tensor(float)'}]` — raw YOLOv8 head `[1, 5, 8400]` (`cx, cy, w, h, score`) in letterboxed pixels.
- **NMS**: Embedded NMS is `False`; C++ host performs confidence filtering + NMS and un-letters boxes via `(x - dw) / scale`, `(y - dh) / scale`.
- **Class**: `0 = hilal`.
