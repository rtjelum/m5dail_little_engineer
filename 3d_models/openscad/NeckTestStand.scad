// Test stand for the M5Dial head: holds one GH-S37D servo shaft-up, so the
// head can be pressed onto it and tried without printing the rest of the robot.
//
// The servo drops into the pocket from the top and rests on its mounting tabs.
// Fasten the tabs with two M2 screws (or the screws that came with the servo).
// Thread the servo cable through the tunnel at the end of the pocket first.
//
// Frame: the servo shaft axis is at x = 0, y = 0. Print as exported, base down.

// ── GH-S37D sizes (mm), same estimates as gh_s37d.scad ─────────────────────
body_w        = 20;     // body length, tabs not included
body_d        = 8.75;   // body thickness
body_h        = 14.5;   // body height below the tabs
tab_w         = 5.45;   // how far each tab sticks out from the body
shaft_pos     = 4.4;    // shaft axis distance from the near body end
clearance     = 0.3;

// ── Stand ──────────────────────────────────────────────────────────────────
base_size     = 80;     // square base, wide enough for the top-heavy head
base_t        = 3;
base_r        = 6;      // base corner radius
pillar_wall   = 4;
pilot_r       = 1.05;   // M2 self-tapping pilot hole
pilot_depth   = 4.5;
cable_w       = 6;
cable_top     = 5;      // solid wall kept above the cable tunnel, under the tab

body_x0  = -shaft_pos;                 // body end near the shaft
body_cx  = body_x0 + body_w / 2;       // body center
tab_off  = body_w / 2 + tab_w / 2;     // tab hole distance from body center
pillar_h = body_h + 0.5;               // body hangs 0.5 above the base
top_z    = base_t + pillar_h;          // tabs rest here

pillar_x0 = body_cx - body_w / 2 - tab_w - pillar_wall;
pillar_x1 = body_cx + body_w / 2 + tab_w + pillar_wall;
pillar_hw = body_d / 2 + clearance + pillar_wall;

module base() {
    translate([0, 0, 0]) linear_extrude(base_t)
        offset(r = base_r) offset(delta = -base_r)
            square(base_size, center = true);
}

module pillar() {
    translate([pillar_x0, -pillar_hw, 0])
        cube([pillar_x1 - pillar_x0, 2 * pillar_hw, top_z]);
}

module pocket() {
    // servo body
    translate([body_x0 - clearance, -body_d / 2 - clearance, base_t])
        cube([body_w + 2 * clearance, body_d + 2 * clearance, pillar_h + 1]);
    // cable tunnel through the bottom of the far end wall (the tab rests above it)
    translate([body_x0 + body_w, -cable_w / 2, base_t])
        cube([pillar_x1 - (body_x0 + body_w) + 1, cable_w, pillar_h - cable_top]);
    // tab screw pilots
    for (s = [-1, 1])
        translate([body_cx + s * tab_off, 0, top_z - pilot_depth])
            cylinder(r = pilot_r, h = pilot_depth + 1, $fn = 24);
}

difference() {
    union() {
        base();
        pillar();
    }
    pocket();
}
