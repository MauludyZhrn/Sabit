#!/usr/bin/env python3
"""Gagal (exit 1) bila ada emoji atau simbol pictografik di kode/UI Sabit.

Dipakai di CI dan pra-commit. Ikon harus berupa SVG (lihat docs/petunjuk-skill-ui.md).
"""
import re
import sys
from pathlib import Path

# Rentang pictografik umum + selector/ZWJ yang membentuk emoji gabungan.
EMOJI = re.compile(
    "["
    "\U0001F000-\U0001F2FF"   # kartu, mahjong, dll.
    "\U0001F300-\U0001FAFF"   # pictograph utama (🚀 🔥 📊 dst.)
    "\u2600-\u27BF"           # simbol misc (☀ ✔ ✈ dst.)
    "\u2B50\u2B55"            # bintang, lingkaran tebal
    "\uFE0F\u200D"            # variation selector, zero-width joiner
    "]"
)
SCAN_SUFFIXES = {".cpp", ".h", ".hpp", ".qss", ".ui", ".qrc", ".qml", ".json", ".ts", ".md"}
SKIP_DIRS = {"vendor", "build", "dist", "cmake-build-debug", ".design_skills", "Captures", ".git"}


def scan(root: Path) -> list[str]:
    hits = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in SCAN_SUFFIXES:
            continue
        if any(part in SKIP_DIRS for part in path.parts):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for lineno, line in enumerate(text.splitlines(), 1):
            for m in EMOJI.finditer(line):
                hits.append(f"{path}:{lineno}: emoji U+{ord(m.group()):04X} -> {line.strip()[:90]}")
    return hits


if __name__ == "__main__":
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    found = scan(root)
    for h in found:
        print(h)
    print(f"{len(found)} temuan emoji.")
    sys.exit(1 if found else 0)
