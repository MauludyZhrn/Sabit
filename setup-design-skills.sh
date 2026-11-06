#!/usr/bin/env bash
# Pasang skill desain ke .design_skills/ di root repo Sabit.
# Jalankan dari root repo: bash tools/setup-design-skills.sh
set -euo pipefail

DEST=".design_skills"
mkdir -p "$DEST"

clone_or_update() {  # $1 = owner/repo, $2 = nama folder
  local dir="$DEST/$2"
  if [ -d "$dir/.git" ]; then
    git -C "$dir" pull --ff-only
  else
    if command -v gh >/dev/null 2>&1; then
      gh repo clone "$1" "$dir" -- --depth 1
    else
      git clone --depth 1 "https://github.com/$1.git" "$dir"
    fi
  fi
  rm -rf "$dir/.git"    # jangan jadikan repo bersarang di dalam repo Sabit
}

clone_or_update nextlevelbuilder/ui-ux-pro-max-skill ui-ux-pro-max-skill
clone_or_update wwewtech/anti-slop-design            anti-slop-design
clone_or_update miqdadbadjuber/anti-slop             anti-slop

# Skill probe dari penulis anti-slop-design: hanya untuk statistik install, bukan panduan desain.
rm -rf "$DEST/anti-slop-design/skills/zzprobe-in-working-repo-5e18"

echo "Selesai. Skill terpasang di $DEST/"
