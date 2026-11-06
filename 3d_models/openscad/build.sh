#!/bin/sh
# Builds GH-S37D STLs into ../parts/gh-s37d/stl/.
# Every <Part>.scad here is rendered to <Part>.stl (gh_s37d.scad and horn_mount.scad are shared libraries).
# Base parts with no .scad don't depend on the servo and are copied from base/es9051/.
set -e
cd "$(dirname "$0")"
OUT=../parts/gh-s37d/stl
mkdir -p "$OUT"
for stl in base/es9051/*.stl; do
    name=$(basename "$stl" .stl)
    [ -f "$name.scad" ] || cp "$stl" "$OUT/$name.stl"
done
for scad in *.scad; do
    name=$(basename "$scad" .scad)
    case $name in gh_s37d|horn_mount|arm_onepiece) continue;; esac   # shared libraries
    echo "render  $name"
    openscad -q --export-format binstl -o "$OUT/$name.stl" "$scad"
done
echo "done: $OUT"
