// Boxy robot-style torso. Used by Torso.scad, the desk
// parts and RobotScene.scad.
//
// The top is the original Chest (with the GH-S37D tab slots from gh_s37d.scad),
// unchanged: the two arm servos lie in it with their shafts out the sides, and
// the neck servo stands in the middle, shaft up. So the arms and the M5Dial
// head mount exactly as before. Under it is a new hollow lower body that
// replaces Belly, BellyJoint, Bell and the Chair. Its open bottom drops into
// the socket in the desk top (see desk_parts.scad), with no screws.
//
// Arm clearance: the one-piece forearms lean in so the hands meet in front of
// the body. So the Chest's side scoops are kept, and the lower body's front is
// set back 3 mm, with a 45-degree slope up to the Chest's front so it
// prints without supports. The arms clear the body from straight up down to about 40
// degrees below horizontal toward the front (the typing pose). They can't hang
// straight down, same as with the original Belly.
//
// Cables: the servo cables go down through the Chest's holes into the lower
// body and out the open bottom into the desk. The Dial's cables run down a
// groove in the back, which carries on into the desk socket.
//
// Torso frame (same as Chest): +x front, +y robot width, +z up. The Chest's
// bottom is at z = 0. The neck servo shaft is at x = neck_shaft_x, y = 0.

include <gh_s37d.scad>

// ── Chest outline (measured from Chest.stl) ────────────────────────────────
chest_half   = [17.35, 28];   // half depth (x), half width (y)
chest_top    = 25.7;
corner_r     = 2;             // vertical edge radius
neck_shaft_x = -6.55;         // neck servo shaft (cable end of the pocket is +x)
arm_shaft    = [4.65, 21];    // arm servo shafts: x, z (out the sides at y = +-28)

// ── Lower body ─────────────────────────────────────────────────────────────
body_h       = 32;            // below the Chest
body_front   = 14.35;         // lower body front face (the Chest's is at 17.35)
step_h       = chest_half[0] - body_front;   // height of the 45-degree slope up to the Chest
seam_z       = -step_h - 1.5; // seam line, just below the slope
body_wall    = 2.5;
roof_apex    = -3;            // top of the hollow inside, a 45-degree gable roof
roof_x       = 3.45;          // ridge line, under the Chest's cable holes
sink         = 8;             // how far the torso sits down into the desk socket
bottom_chamfer = 0.6;

// ── Cable groove for the Dial (back face) ──────────────────────────────────
groove_y     = [-12.5, -6];   // clear of the Chest's screw holes and neck tab slot
groove_d     = 3.5;

torso_bottom = -body_h;

module footprint(inset = 0) {
    offset(r = corner_r - inset) offset(delta = -corner_r)
        square(2 * chest_half, center = true);
}

// lower body outline: the Chest's, with the front set back
module body_footprint(inset = 0) {
    offset(r = corner_r - inset) offset(delta = -corner_r)
        translate([-chest_half[0], -chest_half[1]]) square([chest_half[0] + body_front, 2 * chest_half[1]]);
}

// The Chest's inside holes: cable holes, neck pocket and its cable slot
module inner_holes_2d() {
    intersection() { chest_holes_2d(); square([29, 39.6], center = true); }
}

// The Chest's holes at z = 8 (cable holes, neck pocket, screw holes), plus
// everything outside its outline.
module chest_holes_2d() {
    difference() {
        square(2 * chest_half + [2, 2], center = true);
        projection(cut = true) translate([0, 0, -8]) adapted_part("Chest");
    }
}

// Hollow inside the lower body: vertical walls, then a 45-degree gable roof
// with its ridge along y, so it prints without supports.
module body_cavity() {
    hw = chest_half[1] - body_wall;
    intersection() {
        translate([0, 0, torso_bottom - 1])
            linear_extrude(body_h + roof_apex + 1) body_footprint(body_wall);
        translate([roof_x, 0, roof_apex]) rotate([90, 0, 0])
            linear_extrude(2 * hw + 2, center = true)
                polygon([[-100, -100], [0, 0], [100, -100]]);
    }
}

module seam(z) {
    translate([0, 0, z - 0.4]) linear_extrude(0.8)
        difference() { body_footprint(-1); body_footprint(0.6); }
}

// Details from the picture. Front is +x; the viewer's right is +y.
module details() {
    fx = body_front;
    cx = chest_half[0];
    // seam between the chest and the lower body, and the panel lines
    seam(seam_z);
    translate([fx - 0.6, -21, -21]) cube([1, 31, 0.7]);           // panel bottom
    translate([fx - 0.6, -21, -6]) cube([1, 31, 0.7]);            // panel top
    translate([fx - 0.6, -21, -21]) cube([1, 0.7, 15.7]);         // panel left
    translate([fx - 0.6, 9.3, -21]) cube([1, 0.7, 15.7]);         // panel right
    translate([fx - 0.6, 13, -21]) cube([1, 0.7, 18]);            // long line on the right
    translate([cx - 0.6, 13, 0.5]) cube([1, 0.7, 12]);            // ...carried on up the chest
    // three vent slots, top right of the chest front
    for (i = [0 : 2]) translate([cx - 1, 9, 15 + i * 2.6]) cube([2, 11, 1.2]);
    // speaker grille, bottom right
    for (i = [0 : 3], j = [0 : 2])
        translate([fx + 1, 16 + i * 2.2, -21 + j * 2.2]) rotate([0, -90, 0]) cylinder(d = 1.2, h = 2, $fn = 12);
    // vent slots on both sides of the lower body
    for (s = [-1, 1], i = [0 : 3])
        translate([-8, s * chest_half[1] - 1, -18 + i * 3]) cube([12, 2, 1.2]);
}

// groove down the back, from the top to the bottom
module dial_cable_path() {
    translate([-chest_half[0] - 1, groove_y[0], torso_bottom - 1])
        cube([groove_d + 1, groove_y[1] - groove_y[0], chest_top + body_h + 2]);
}

module torso() {
    difference() {
        union() {
            adapted_part("Chest");
            // lower body
            difference() {
                union() {
                    hull() {
                        translate([0, 0, torso_bottom + bottom_chamfer])
                            linear_extrude(body_h - bottom_chamfer - step_h + 0.01) body_footprint();
                        translate([0, 0, torso_bottom]) linear_extrude(1) body_footprint(bottom_chamfer);
                    }
                    // 45-degree slope up to the Chest's front, instead of a flat step
                    hull() {
                        translate([0, 0, -step_h]) linear_extrude(0.01) body_footprint();
                        translate([0, 0, -0.01]) linear_extrude(0.02) footprint();
                    }
                }
                body_cavity();
                // keep the Chest's holes open down into the cavity
                translate([0, 0, roof_apex - 20]) linear_extrude(-roof_apex + 21)
                    inner_holes_2d();
            }
        }
        details();
        dial_cable_path();
    }
}
