// Desk top and walls, with the socket the torso drops into. See desk_parts.scad.
// Print as exported: upside down, top on the bed. No supports.
include <desk_parts.scad>

translate([0, 0, desk_h]) rotate([180, 0, 0]) desk_box();
