#!/bin/sh
# Builds GH-S37D STLs into ../parts/gh-s37d/stl/.
# Every <Part>.scad here is rendered to <Part>.stl, except the shared libraries and the display scene.
set -e
cd "$(dirname "$0")"
OUT=../parts/gh-s37d/stl
mkdir -p "$OUT"
for scad in *.scad; do
    name=$(basename "$scad" .scad)
    case $name in gh_s37d|horn_mount|arm_onepiece|torso_body|desk_parts|RobotScene) continue;; esac   # shared libraries and the display scene
    echo "render  $name"
    openscad -q --export-format binstl -o "$OUT/$name.stl" "$scad"
done
echo "done: $OUT"
