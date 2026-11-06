// Test block for the cross-horn mount (horn_mount.scad).
//
// Same recess, hub hole and pilot holes as under the head post, printed the
// same way (recess face standing upright), so the fit matches the head.
// Check that the horn drops in without forcing and doesn't twist, and that
// the pilot holes line up with the holes in the horn's arms. If not, measure
// the horn and change the sizes in horn_mount.scad.
//
// Print as exported. No supports needed.

include <horn_mount.scad>

block   = 2 * (horn_extent + 2.5);   // square block around the recess
block_t = horn_t + horn_pilot_depth + 1;

// Build in the head's frame (mount face at z = 0), then turn it the way the
// head prints (rotate [0, 90, 0]) and lift it onto the bed.
translate([0, 0, block / 2])
    rotate([0, 90, 0])
        difference() {
            translate([-block / 2, -block / 2, 0]) cube([block, block, block_t]);
            horn_mount_cut(block_t + 1);
        }
