#!/usr/bin/env python3
"""
check_no_emoji.py
Scans source files for emojis and disallowed decorative Unicode symbols.
Fails with exit code 1 if any forbidden symbols are found.
"""

import sys
import os
import re

# Range of typical emoji and decorative symbol code points
EMOJI_PATTERN = re.compile(
    r'[\U0001F000-\U0001FAFF]|'  # Miscellaneous Symbols and Pictographs, Emoticons, Transport, etc.
    r'[\U00002600-\U000027BF]|'  # Miscellaneous Symbols, Dingbats
    r'[\U00002300-\U000023FF]|'  # Miscellaneous Technical
    r'[\u25A0-\u25FF]|'          # Geometric Shapes (like ●, ▲, ▾)
    r'[\u2700-\u27BF]|'          # Dingbats (like ✓, ✗)
    r'[\u2B00-\u2BFF]'           # Misc Symbols & Arrows
)

EXTENSIONS = ('.cpp', '.h', '.hpp', '.c', '.json', '.ts', '.qss', '.ui')
IGNORE_DIRS = {'.git', 'build', 'dist', '.design_skills', 'vcpkg', 'vcpkg_installed', '__pycache__', '.vscode'}

def scan_directory(root_dir):
    findings = []
    for root, dirs, files in os.walk(root_dir):
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
        for f in files:
            if not f.endswith(EXTENSIONS):
                continue
            path = os.path.join(root, f)
            try:
                with open(path, 'r', encoding='utf-8', errors='replace') as fp:
                    for line_idx, line in enumerate(fp, start=1):
                        matches = EMOJI_PATTERN.findall(line)
                        if matches:
                            findings.append((path, line_idx, line.strip(), matches))
            except Exception as e:
                print(f"Error reading {path}: {e}")
    return findings

def main():
    target = sys.argv[1] if len(sys.argv) > 1 else "."
    print(f"Scanning '{target}' for emojis and decorative Unicode symbols...")
    findings = scan_directory(target)

    if findings:
        print(f"\n[FAIL] Found {len(findings)} disallowed emoji/symbol instances:")
        for path, line_no, content, chars in findings:
            print(f"  {path}:{line_no} => symbols: {chars} | line: {content}")
        sys.exit(1)
    else:
        print("\n[PASS] No emojis or disallowed Unicode symbols found in scanned files.")
        sys.exit(0)

if __name__ == "__main__":
    main()
