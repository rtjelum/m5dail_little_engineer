// One-piece arm: UpperArm and Forearm joined at the elbow, with a new servo
// socket. Used by ArmLeft.scad and ArmRight.scad.
//
// The elbow joint is the original one: the UpperArm's 5 mm square end sits in
// the tilted 5.6 mm pocket in the Forearm's elbow ball, as if screwed together.
// The gap around it and the old elbow screw hole are filled in. The square end
// could sit in the pocket four ways. This uses the one where the servo side of
// the shoulder hub faces the chest and the forearm leans in toward the body
// (turned 90 degrees from the first version, after a test fit).
//
// The shoulder socket is the head-post socket that fit in HeadSocketTest:
// a 3.4 mm hex (across the flats), 2.4 mm deep, with the servo screw hole and
// a counterbore for the screw head on the outside. It prints sideways, the
// same way as in the test, so the fit carries over.
//
// Print orientation: the upper arm lies flat with the hub on its side, and the
// forearm and hand rise at about 38 degrees. The hand needs supports
// (tree supports work well).

// ── Shoulder socket (tested with HeadSocketTest) ───────────────────────────
socket_flats  = 3.4;    // across the hex flats
socket_depth  = 2.4;
web_t         = 1.5;
screw_r       = 0.8;
counterbore_r = 2.5;    // screw head recess, on the outer side of the hub

// ── UpperArm shape (from the original part) ────────────────────────────────
hub_r         = 4.92;
hub_t         = 6.71;
bar_end_x     = 20.56;  // end of the square bar, sits on the pocket floor
bar_w         = 5;
flat_y        = 3.5;    // print bed side (local -y): bar thickened and hub flattened to here

// ── Elbow (measured from ForearmLeft) ──────────────────────────────────────
// Places the UpperArm (its own frame) into the Forearm's frame.
PLACE = [[ 0.45315,  0.64435,  0.61601, -15.66488],
         [-0.42262, -0.45315,  0.78489,   4.80941],
         [ 0.78489, -0.61601,  0.06696, -13.19239],
         [ 0,        0,        0,         1       ]];
pocket_w      = 5.6;
ball_ctr      = [-6.38, 0.02, 2.52];
ball_r        = 5.95;   // just inside the elbow ball

// Turns the joined arm (Forearm frame) into print orientation.
PRINT = [[ 0.61601,  0.78489,  0.06696, 0],
         [ 0.45315, -0.42262,  0.78489, 0],
         [ 0.64435, -0.45315, -0.61601, 0],
         [ 0,        0,        0,       1]];
bed_z = -4.1463 - flat_y;  // print height of the flat side, before lifting

module upper_arm() {
    difference() {
        intersection() {
            union() {
                cylinder(r = hub_r, h = hub_t, $fn = 64);
                translate([0, -flat_y, 0]) cube([bar_end_x, flat_y + bar_w / 2, bar_w]);
            }
            translate([-10, -flat_y, -1]) cube([40, 10 + flat_y, hub_t + 2]);
        }
        // hex socket, from the servo side
        translate([0, 0, hub_t - socket_depth])
            cylinder(r = socket_flats / 2 / cos(30), h = socket_depth + 1, $fn = 6);
        // servo screw hole and head recess
        translate([0, 0, -1]) cylinder(r = screw_r, h = hub_t + 2, $fn = 24);
        translate([0, 0, -1])
            cylinder(r = counterbore_r, h = 1 + hub_t - socket_depth - web_t, $fn = 40);
    }
}

// Fills the gap around the square end and the old elbow screw hole.
module elbow_fill() {
    intersection() {
        translate(ball_ctr) sphere(r = ball_r, $fn = 64);
        translate([-50, -50, 0]) cube(100);   // keep the ball's flat underside
        multmatrix(PLACE)
            translate([bar_end_x - 12, -pocket_w / 2, bar_w / 2 - pocket_w / 2])
                cube([20, pocket_w, pocket_w]);
    }
}

module arm_left_assembled() {
    union() {
        import("base/es9051/ForearmLeft.stl", convexity = 10);
        multmatrix(PLACE) upper_arm();
        elbow_fill();
    }
}

module arm_left_print() {
    translate([0, 0, -bed_z])
        intersection() {
            multmatrix(PRINT) arm_left_assembled();
            translate([-100, -100, bed_z]) cube(200);
        }
}
