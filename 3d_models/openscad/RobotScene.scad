// Display scene: the printable robot parts put together on the desk, typing
// on a small laptop. Uses the real Torso, HeadM5Dial, ArmLeft/Right,
// DeskBox, DeskBase, Laptop and a PCA9685 model, so it doubles as an assembly
// check. Only the screens and the face drawn on the Dial are props.
//
// Not printable; build.sh skips it.
//
// Scene frame: the robot faces -y (toward the viewer), +z up, the desk top at
// z = 0, the torso centered on x = 0, y = 0.
//
// Render (front, side and cut-away views, as in ../renders/):
//   openscad -o front.png --imgsize=1000,1100 --colorscheme=Tomorrow --camera=0,-10,25,72,0,28,520 RobotScene.scad
//   openscad -o side.png  --imgsize=1000,1100 --colorscheme=Tomorrow --camera=0,-10,25,62,0,-35,520 RobotScene.scad
//   openscad -o cut.png -D cutaway=true --imgsize=1000,1100 --colorscheme=Tomorrow --camera=0,-10,10,70,0,-60,480 RobotScene.scad

head_m5dial_no_output = true;
include <HeadM5Dial.scad>
include <desk_parts.scad>    // also brings in torso_body and gh_s37d
include <arm_onepiece.scad>

cutaway   = false;   // true: hide the desk's left side wall (seen from the viewer) to show the board
arm_angle = -70;     // shoulder angle: 0 = arms straight forward, -90 = down

// ── Colors ─────────────────────────────────────────────────────────────────
shell   = "#d3cbb9";   // beige plastic
joint   = "#bdb4a1";
blue    = "#2f62d9";
yellow  = "#f6d22a";
wood    = "#c9b79a";
wood2   = "#b5a385";
alu     = "#b9bcc2";
dark    = "#2b2d31";

// ── Assembly positions (estimates where noted) ─────────────────────────────
head_z  = 29;        // head floor above the Chest's bottom (servo + horn stack, estimate)
hub_y   = 29.3;      // arm hub's inner face from the torso center (estimate)

// ── Head ───────────────────────────────────────────────────────────────────
// the Dial's screen, just behind the front plate, with a face drawn on it
module dial_face() {
    module disc(x, h) translate([x, 0, dial_zc]) rotate([0, 90, 0]) linear_extrude(h) children();
    // draw in (y, z), seen from the front
    module face(x, h) disc(x, h) rotate(90) children();
    color(dark)   disc(-front_t - 0.6, 0.3) circle(r = bore_r + 0.4, $fn = 96);
    color(blue)   face(-front_t - 0.4, 0.3) circle(r = 19.5, $fn = 96);
    color(yellow) face(-front_t - 0.2, 0.3) for (sy = [-1, 1]) difference() {
        translate([sy * 7, 4]) circle(r = 5.6, $fn = 40);
        translate([sy * 7, 9.2]) rotate(sy * 18) square([16, 6], center = true);
    }
    color(dark) face(-front_t, 0.3) for (sy = [-1, 1]) {
        translate([sy * 6.2, 2.8]) circle(r = 2.2, $fn = 24);
        translate([sy * 7, 10.6]) rotate(sy * 18) square([12, 1.6], center = true);
    }
    color(dark) face(-front_t - 0.2, 0.3) translate([-10.5, -11.5]) offset(r = 1.8) square([21, 8]);
    color("white") face(-front_t, 0.3) difference() {
        translate([-10, -11]) offset(r = 1.4) square([20, 7]);
        translate([-12, -7.8]) square([24, 0.7]);
        for (i = [-3 : 3]) translate([i * 3.1 - 0.35, -14]) square([0.7, 12]);
    }
}

module robot_head() {
    translate([neck_shaft_x - shaft_x, 0, head_z]) {
        color(shell) head_m5dial();
        dial_face();
    }
}

// ── Arms (the printed one-piece arms, posed on the shoulder servos) ────────
function inv(M) = let(R = [[M[0][0], M[1][0], M[2][0]], [M[0][1], M[1][1], M[2][1]], [M[0][2], M[1][2], M[2][2]]],
                      t = [M[0][3], M[1][3], M[2][3]], n = -(R * t))
    [[R[0][0], R[0][1], R[0][2], n[0]], [R[1][0], R[1][1], R[1][2], n[1]], [R[2][0], R[2][1], R[2][2], n[2]], [0, 0, 0, 1]];

// left arm on the +y side, turned th degrees about the shoulder shaft
module arm_on_shoulder(th) {
    c = cos(th); s = sin(th);
    multmatrix([[c, -s, 0, arm_shaft[0]], [0, 0, -1, hub_y + hub_t], [s, c, 0, arm_shaft[1]], [0, 0, 0, 1]])
        multmatrix(inv(PLACE_ARM)) arm_left_assembled();
}

module robot() {
    color(shell) torso();
    robot_head();
    color(joint) arm_on_shoulder(arm_angle);
    color(joint) mirror([0, 1, 0]) arm_on_shoulder(arm_angle);
}

// ── Laptop screen (prop, sits in the printed laptop's screen recess) ───────
module laptop_screen() {
    translate([0, lap_front + lid_t / 2, desk_h + lap_t / 2]) rotate([lid_lean, 0, 0])
        translate([-lap_w / 2 + 3.5, lid_t / 2 - 0.45, 4.5]) cube([lap_w - 7, 0.4, lid_h - 8]);
}

// ── Scene ──────────────────────────────────────────────────────────────────
translate([0, -torso_y, -desk_h]) {
    color(wood) difference() {
        desk_box();
        if (cutaway) translate([-desk_w / 2 - 1.5, desk_wall, -1]) cube([desk_wall + 2, inner_d, desk_h - top_t + 1]);
    }
    color(wood2) desk_base();
    pca9685_model();
    color(alu) laptop_part();
    color("#4f8de8") laptop_screen();
    torso_on_desk() robot();
}
