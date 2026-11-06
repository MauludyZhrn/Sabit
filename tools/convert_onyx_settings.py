#!/usr/bin/env python3
"""
convert_onyx_settings.py
Migrate Onyx OS settings.json to Sabit Schema v2 settings.json.
Conforms to 01-PRD.md FR-31 & 02-Technical-Specification.md Section 4.3 & 4.4.
"""

import sys
import os
import json
import shutil
from datetime import datetime

DEFAULT_SCHEMA_V2 = {
    "schema": 2,
    "general": {
        "language": "id",
        "theme": "dark",
        "prevent_sleep": True
    },
    "site": {
        "preset": "Jakarta",
        "latitude": -6.2088,
        "longitude": 106.8456,
        "elevation_m": 10.0,
        "timezone": "Asia/Jakarta",
        "refraction": True
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
    "performance": {
        "ring_buffer_mb": 512,
        "stack_ram_mb": 2048,
        "ai_max_fps": 3
    },
    "ai": {
        "model": "models/hilal_v1.onnx",
        "confidence": 0.5,
        "provider": "cpu"
    }
}

THEME_MAP = {
    "Dark Onyx": "dark",
    "dark": "dark",
    "Night Vision Red": "night_red",
    "night": "night_red",
    "night_red": "night_red",
    "Light System": "light",
    "light": "light"
}

CRITERIA_MAP = {
    "MABIMS Baru (Alt 3°, Elongasi 6.4°)": "MABIMS-baru",
    "MABIMS-baru": "MABIMS-baru",
    "MABIMS Lama": "MABIMS-lama",
    "Odeh": "Odeh",
    "Danjo": "Danjo"
}

def convert_onyx_to_v2(onyx_dict):
    """Convert Onyx settings dictionary to Schema v2 and list discarded keys."""
    v2 = json.loads(json.dumps(DEFAULT_SCHEMA_V2))
    discarded = []

    # 1. General section
    if "general" in onyx_dict:
        g = onyx_dict["general"]
        if "language" in g:
            v2["general"]["language"] = g["language"]
        if "theme" in g:
            v2["general"]["theme"] = THEME_MAP.get(g["theme"], "dark")
        if "prevent_sleep" in g:
            v2["general"]["prevent_sleep"] = bool(g["prevent_sleep"])

        for k in g:
            if k not in ["language", "theme", "prevent_sleep"]:
                discarded.append(f"general.{k}")

    # 2. Location -> Site section
    if "location" in onyx_dict:
        loc = onyx_dict["location"]
        if "preset" in loc:
            v2["site"]["preset"] = loc["preset"]
        if "latitude" in loc:
            v2["site"]["latitude"] = float(loc["latitude"])
        if "longitude" in loc:
            v2["site"]["longitude"] = float(loc["longitude"])
        if "elevation" in loc:
            v2["site"]["elevation_m"] = float(loc["elevation"])
        for k in loc:
            if k not in ["preset", "latitude", "longitude", "elevation"]:
                discarded.append(f"location.{k}")

    # 3. Astro section
    if "astro" in onyx_dict:
        a = onyx_dict["astro"]
        if "criteria" in a:
            v2["astro"]["criteria"] = CRITERIA_MAP.get(a["criteria"], a["criteria"])
        if "atmospheric_refraction" in a:
            v2["site"]["refraction"] = bool(a["atmospheric_refraction"])
        for k in a:
            if k not in ["criteria", "atmospheric_refraction"]:
                discarded.append(f"astro.{k}")

    # 4. Saving -> Capture section
    if "saving" in onyx_dict:
        s = onyx_dict["saving"]
        if "output_dir" in s:
            v2["capture"]["output_dir"] = s["output_dir"]
        for k in s:
            if k not in ["output_dir"]:
                discarded.append(f"saving.{k}")

    # 5. Top-level unused sections
    for section in ["cameras", "hardware_guiding", "performance"]:
        if section in onyx_dict:
            for k in onyx_dict[section]:
                discarded.append(f"{section}.{k}")

    return v2, discarded


def main():
    if len(sys.argv) < 2:
        print("Usage: convert_onyx_settings.py <input_onyx_settings.json> [output_settings.json]")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else "settings.json"

    if not os.path.exists(input_path):
        print(f"Error: input file '{input_path}' not found.", file=sys.stderr)
        sys.exit(1)

    with open(input_path, "r", encoding="utf-8") as f:
        try:
            onyx_data = json.load(f)
        except json.JSONDecodeError as err:
            print(f"Error decoding JSON from '{input_path}': {err}", file=sys.stderr)
            sys.exit(1)

    v2_data, discarded = convert_onyx_to_v2(onyx_data)

    if os.path.abspath(input_path) == os.path.abspath(output_path):
        timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
        backup_path = f"{output_path}.bak-{timestamp}"
        shutil.copy2(output_path, backup_path)
        print(f"[Backup] Original backed up to: {backup_path}")

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(v2_data, f, indent=2)

    print(f"[Success] Converted '{input_path}' to Schema v2: '{output_path}'")
    print(f"[Report] Migrated keys: general, site, astro, capture, performance, ai.")
    print(f"[Report] Discarded {len(discarded)} unused/dead Onyx keys:")
    for k in discarded:
        print(f"  - {k}")

if __name__ == "__main__":
    main()
