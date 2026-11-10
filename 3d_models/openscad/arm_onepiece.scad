// One-piece arm: UpperArm and Forearm joined at the elbow, with a new servo
// socket. Used by ArmLeft.scad and ArmRight.scad.
//
// The elbow keeps the original angle: the UpperArm sits where its 5 mm square
// end sat in the Forearm's tilted elbow pocket. The round elbow ball is
// replaced by a square block (see "Square elbow" below). The square end
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
bar_end_x     = 20.56;  // end of the original square bar, sat on the pocket floor
upper_ext     = 12;     // extra upper arm length: the elbow and forearm move out by this much
bar_w         = 5;
flat_y        = 3.5;    // print bed side (local -y): bar thickened and hub flattened to here

// ── Elbow (measured from ForearmLeft) ──────────────────────────────────────
// Places the UpperArm (its own frame) into the Forearm's frame.
PLACE = [[ 0.45315,  0.64435,  0.61601, -15.66488],
         [-0.42262, -0.45315,  0.78489,   4.80941],
         [ 0.78489, -0.61601,  0.06696, -13.19239],
         [ 0,        0,        0,         1       ]];
// The same, with the upper arm made upper_ext longer: its hub moves back
// along the bar, away from the elbow.
PLACE_ARM = PLACE * [[1, 0, 0, -upper_ext], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]];

// ── Square elbow ───────────────────────────────────────────────────────────
// The round elbow ball is replaced by a flat-faced block: the hull of the end
// of the upper arm and the start of the forearm, each a little oversize. So
// both bars run straight into it and nothing is cut off the forearm (only
// its ball, at Forearm x < ball_end_x, is removed). It doesn't reach closer
// to the body than the forearm itself.
ball_end_x    = -0.3;               // Forearm frame: the ball ends here, the bar starts
elbow_grow    = 0.3;                // how much bigger than the bars the block is
elbow_upper   = [14.5, 20.56];      // UpperArm frame: x range of the upper arm end
elbow_fore    = [-0.3, 4.5];        // Forearm frame: x range of the forearm start

// ── Robot hand ─────────────────────────────────────────────────────────────
// The original human-style hand is replaced by a simple robot claw, like in
// improve_robot.jpg, kept chunky so it prints easily: a square wrist collar,
// a block palm, two thick prongs and a thumb, all at least 3 mm thick, with no
// small details. Forearm frame: the bar runs along +x (y -2.5..2.5, z 0..5);
// the hand starts at hand_x. The prongs bend toward +z, the palm side.
hand_x        = 15;                 // the forearm bar ends here
palm          = [6, 8, 5];          // length, width, thickness
prong         = [3, 3.2];           // prong width, thickness
prong_gap     = 1.6;                // gap between the two prongs
prong_len     = [4, 3.5];           // straight part, bent tip
prong_bend    = 35;                 // tip bend toward the palm side
thumb_len     = 5;

// straight block between two points, w across (y), t thick
module bone(a, b, w, t) {
    hull() for (p = [a, b]) translate(p - [0.5, w / 2, t / 2]) cube([1, w, t]);
}

module robot_hand() {
    zc = 2.5;                                   // forearm bar center height
    palm_x0 = hand_x + 1.5;
    palm_x1 = palm_x0 + palm[0];
    // square wrist collar and palm
    translate([hand_x - 0.5, -3, zc - 3]) cube([2.5, 6, 6]);
    translate([palm_x0, -palm[1] / 2, zc - palm[2] / 2]) cube(palm);
    // two prongs from the front of the palm, tips bent toward +z
    for (sy = [-1, 1]) let(y = sy * (prong_gap + prong[0]) / 2) {
        p0 = [palm_x1 - 0.5, y, zc];
        p1 = p0 + [prong_len[0], 0, 0];
        p2 = p1 + prong_len[1] * [cos(prong_bend), 0, sin(prong_bend)];
        bone(p0, p1, prong[0], prong[1]);
        bone(p1, p2, prong[0], prong[1]);
    }
    // thumb, from the palm side, leaning forward
    t0 = [palm_x0 + 1.5, -palm[1] / 2 + prong[0] / 2, zc + palm[2] / 2 - 1];
    bone(t0, t0 + thumb_len * [cos(50), 0, sin(50)], prong[0], prong[1]);
}

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
                translate([0, -flat_y, 0]) cube([bar_end_x + upper_ext, flat_y + bar_w / 2, bar_w]);
            }
            translate([-10, -flat_y, -1]) cube([40 + upper_ext, 10 + flat_y, hub_t + 2]);
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

module elbow_block() {
    g = elbow_grow;
    hull() {
        // end of the upper arm (its bar: y -flat_y..bar_w/2, z 0..bar_w)
        multmatrix(PLACE) translate([elbow_upper[0], -flat_y, -g])
            cube([elbow_upper[1] - elbow_upper[0] + g, flat_y + bar_w / 2 + g, bar_w + 2 * g]);
        // start of the forearm (its bar: y -2.5..2.5, z 0..5)
        translate([elbow_fore[0], -2.5 - g, 0])
            cube([elbow_fore[1] - elbow_fore[0], 5 + 2 * g, 5 + g]);
    }
}

module arm_left_assembled() {
    union() {
        // the original forearm, without its elbow ball and hand
        intersection() {
            import("base/es9051/ForearmLeft.stl", convexity = 10);
            translate([ball_end_x, -50, -50]) cube([hand_x - ball_end_x, 100, 100]);
        }
        robot_hand();
        multmatrix(PLACE_ARM) upper_arm();
        elbow_block();
    }
}

module arm_left_print() {
    translate([0, 0, -bed_z])
        intersection() {
            multmatrix(PRINT) arm_left_assembled();
            translate([-100, -100, bed_z]) cube(200);
        }
}
