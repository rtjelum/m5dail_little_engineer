// Mount for the servo's cross horn (the X-shaped arm that comes with the servo).
//
// The horn sits in an X-shaped recess so it can't twist, with a 5.5 mm hole
// in the middle for the horn's hub and the center screw. Small pilot holes
// line up with the holes in the horn's arms, so tiny screws through the horn
// hold the part on.
//
// If the horn doesn't fit, measure it and change the sizes below; the head
// updates.

// ── Horn sizes (mm) ────────────────────────────────────────────────────────
horn_reach    = 9;      // center to the tip of one arm
horn_root_w   = 3.8;    // arm width at the hub
horn_tip_w    = 2.8;    // arm width at the tip
horn_t        = 1.6;    // arm thickness = recess depth
horn_clear    = 0.25;   // gap around the horn
horn_angle    = 45;     // turn of the X (45 = arms point diagonally)
hub_hole_d    = 5.5;    // hole for the horn's hub and the center screw
horn_holes    = [5, 7.5];  // pilot holes: distance from the center along each arm
horn_pilot_d  = 1.0;    // pilot for the horn's small screws
horn_pilot_depth = 5;

// How far the recess reaches along x or y from the center (arms at 45 deg)
horn_extent = (horn_reach + horn_clear) * cos(45) + (horn_tip_w / 2 + horn_clear) * sin(45);

// 2D X shape, centered
module horn_x_2d() {
    rotate(horn_angle)
        for (a = [0, 90, 180, 270]) rotate(a)
            hull() {
                circle(d = horn_root_w + 2 * horn_clear, $fn = 32);
                translate([horn_reach - horn_tip_w / 2, 0])
                    circle(d = horn_tip_w + 2 * horn_clear, $fn = 32);
            }
}

// Cut-out for the horn. The mounting face is at z = 0, material is above it
// (+z). The hub hole runs up hub_len.
module horn_mount_cut(hub_len) {
    translate([0, 0, -1]) linear_extrude(horn_t + 1) horn_x_2d();
    translate([0, 0, -1]) cylinder(d = hub_hole_d, h = hub_len + 1, $fn = 48);
    rotate(horn_angle)
        for (a = [0, 90, 180, 270], r = horn_holes) rotate(a)
            translate([r, 0, -1]) cylinder(d = horn_pilot_d, h = horn_t + horn_pilot_depth + 1, $fn = 16);
}
