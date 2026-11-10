// Torso: the Chest's servo block on a boxy lower body. See torso_body.scad.
// Replaces Chest, Belly, BellyJoint, Bell, BottomCover and Chair.
// Print as exported, open bottom down, no supports.
include <torso_body.scad>

translate([0, 0, body_h]) torso();
