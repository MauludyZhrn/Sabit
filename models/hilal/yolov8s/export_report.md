# Phase 9 — ONNX Export Report (HILAL-DETECT V2)

- Exported ONNX: `D:\Projects\HILAL-DETECT-V2-TRAINING\artifacts\research\best.onnx` (44746477 bytes)
- Research PyTorch weights: `D:\Projects\HILAL-DETECT-V2-TRAINING\artifacts\research\best.pt` (22525802 bytes)
- Ultralytics `8.4.216` | ONNX `1.23.2` | ORT `1.23.2`
- Opset: `17` | Static shape: `True` | Embedded NMS: `False`

## Input / Output Contract
- Inputs: `[{'name': 'images', 'shape': [1, 3, 640, 640], 'type': 'tensor(float)'}]`
- Outputs: `[{'name': 'output0', 'shape': [1, 5, 8400], 'type': 'tensor(float)'}]`
- Preprocess: Letterbox BGR->RGB to `640x640` with pad `(114, 114, 114)`, scale `0..1` FP32, layout `NCHW`.
- Postprocess: Raw output `[1, 5, 8400]` contains `[cx, cy, w, h, hilal_score]` in letterboxed pixel space; perform confidence filtering + NMS in C++ unless embedded NMS was enabled.