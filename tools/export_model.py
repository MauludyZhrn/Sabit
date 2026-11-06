#!/usr/bin/env python3
"""
export_model.py
Exports YOLO PyTorch model (*.pt) to ONNX runtime model (*.onnx),
computes SHA-256 checksum, and writes models/manifest.json.
Conforms to 01-PRD.md FR-19, G-05 & 02-Technical-Specification.md Section 8, ADR-02.
"""

import os
import sys
import json
import hashlib
import shutil
from pathlib import Path

def compute_sha256(file_path):
    h = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def export_model(pt_path, onnx_dest_path, manifest_path):
    from ultralytics import YOLO

    pt_file = Path(pt_path).resolve()
    if not pt_file.exists():
        raise FileNotFoundError(f"Input model not found: {pt_file}")

    print(f"[Export] Loading model from {pt_file}...")
    model = YOLO(str(pt_file))

    print(f"[Export] Exporting to ONNX (imgsz=640, opset=12)...")
    # Export with fixed batch=1, imgsz=640 for CPU inference stability
    exported_file = model.export(
        format="onnx",
        imgsz=640,
        opset=12,
        dynamic=False,
        simplify=False
    )

    exported_path = Path(exported_file)
    onnx_dest = Path(onnx_dest_path).resolve()
    onnx_dest.parent.mkdir(parents=True, exist_ok=True)

    if exported_path != onnx_dest:
        shutil.copy2(exported_path, onnx_dest)
        print(f"[Export] Saved ONNX to {onnx_dest}")

    file_size = os.path.getsize(onnx_dest)
    sha256 = compute_sha256(onnx_dest)
    print(f"[Export] SHA-256: {sha256} ({file_size} bytes)")

    # Extract class names
    classes = []
    if hasattr(model, "names") and model.names:
        classes = [model.names[i] for i in sorted(model.names.keys())]

    manifest = {
        "schema": 1,
        "models": [
            {
                "name": "hilal_v1",
                "file": onnx_dest.name,
                "format": "onnx",
                "task": getattr(model, "task", "detect"),
                "classes": classes,
                "sha256": sha256,
                "size_bytes": file_size,
                "input_shape": [1, 3, 640, 640],
                "opset": 12,
                "source": pt_file.name,
                "description": "Hilal detection model (single-class YOLO) exported for Sabit ONNX Runtime"
            }
        ]
    }

    manifest_file = Path(manifest_path).resolve()
    manifest_file.parent.mkdir(parents=True, exist_ok=True)
    with open(manifest_file, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print(f"[Export] Manifest written to {manifest_file}")

def main():
    pt_path = sys.argv[1] if len(sys.argv) > 1 else "models/onyx_hilal_v1.pt"
    onnx_dest = sys.argv[2] if len(sys.argv) > 2 else "models/hilal_v1.onnx"
    manifest = sys.argv[3] if len(sys.argv) > 3 else "models/manifest.json"

    export_model(pt_path, onnx_dest, manifest)

if __name__ == "__main__":
    main()
