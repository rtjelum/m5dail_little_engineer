// Laptop for the desk: one piece with the lid open, and a tab underneath that
// drops into the slot in DeskBox in front of the robot. See desk_parts.scad.
// Print as exported, lying on its side (the base, lid and tab ends share one
// flat face on the bed). No supports.
include <desk_parts.scad>

translate([0, 0, lap_w / 2]) rotate([0, -90, 0]) translate([0, 0, -desk_h]) laptop_part();
