// Desk for the robot: a closed box as wide as the shoulders, with the PCA9685
// servo board inside. Used by DeskBox.scad, DeskBase.scad, Laptop.scad
// and RobotScene.scad.
//
// - DeskBox: the top and all four walls; open only at the bottom. The torso's
//   lower body drops 8 mm into a socket in the top, with no screws. The
//   socket stops it on a 45-degree ledge, and small ribs in the socket walls
//   make it a snug fit. The back wall has a slot for the servo power cable
//   and the Dial's USB cable. Printed upside down (top on the bed), so
//   nothing needs supports.
// - DeskBase: the bottom plate, flush with the box's outside. A rim on it
//   presses up inside the walls (with crush ribs), closing the bottom. The
//   PCA9685 presses onto four pins on the plate, and pairs of slots take zip
//   ties for the wires. Lift the box off to get at the wiring.
//
// - Laptop (Laptop.scad): one piece with the lid open, and a tab underneath
//   that drops into a slot in the desk top in front of the robot.
//
// Cables: the servo cables come down out of the torso's open bottom, straight
// into the desk. The Dial's cables come down the groove in the torso's back,
// which carries on past the ledge into the desk.
//
// Desk frame: +x across (the robot's width), y = 0 at the front edge and
// y = desk_d at the back, z = 0 on the table, the top surface at z = desk_h.
// The robot faces -y.

include <torso_body.scad>

// ── Desk ───────────────────────────────────────────────────────────────────
desk_w      = 72;      // shoulder wide
desk_d      = 80;
desk_h      = 50;      // as low as the PCA9685 + plugs + wire bend room allow (about 49)
top_t       = 6;
desk_wall   = 4;       // thick walls: sturdier and easier to print
lip         = 2;       // top overhang past the walls
torso_back_gap = 7;    // top behind the torso, clear of the back wall

// ── Socket for the torso ───────────────────────────────────────────────────
sock_clear  = 0.15;    // per side
collar_t    = 2;       // wall around the socket, under the top
ledge       = 2.5;     // 45-degree ledge the torso rests on (= body wall)
rib_r       = 0.5;     // crush ribs; they stick out rib_r - sock_clear
sock_floor  = desk_h - sink;    // bottom of the torso when seated

// ── Base and PCA9685 (Adafruit 16-channel) ─────────────────────────────────
base_t      = 2.5;
base_clear  = 0.3;
pca_pcb     = [25.4, 62.5, 1.6];   // across, along y, thickness
pca_holes   = [19.0, 55.9];        // mounting hole spacing
pca_pin_d   = 2.3;                 // board holes are about 2.5 mm
pca_standoff = 5;
pca_header  = 6;                   // right-angle 6-pin header past the front end
pca_plug_h  = 14;                  // servo plugs above the PCB
pca_x       = -10;                 // board center; servo plugs face +x
cable_slot  = [14, 11.5, 14, 8];   // back wall: center x, center z, width, height
rim_t       = 1.6;                 // rim on the bottom plate, inside the walls
rim_h       = 3;
rim_clear   = 0.15;                // per side; ribs on the rim press into the walls
tie_slots   = [[14, 22], [14, 42], [14, 62], [-26, 70]];   // zip tie slot pairs (x, y)

// ── Laptop and its slot ────────────────────────────────────────────────────
lap_w       = 56;
lap_d       = 37;
lap_t       = 3;       // base thickness
lap_front   = 1.5;     // front edge of the base, from the desk's front edge
lid_h       = 36;
lid_t       = 2.4;
lid_lean    = 18;      // lid tilt away from the robot, past vertical
tab_w       = 8;       // tab under the base (front to back), full laptop width
tab_h       = 4.5;     // less than top_t, so the base sits on the desk
tab_clear   = 0.15;    // per side; ribs in the slot press into the tab

inner_w     = desk_w - 2 * desk_wall;
inner_d     = desk_d - 2 * desk_wall;
base_y0     = desk_wall + base_clear;
rim_in      = rim_clear + rim_t;   // rim's inner face, in from the walls' inner faces
pca_y0      = base_y0 + 1 + pca_header;        // front end of the PCB
pca_y       = pca_y0 + pca_pcb[1] / 2;          // board center

// torso origin (its Chest bottom, frame center) in the desk frame
torso_y     = desk_d - torso_back_gap - chest_half[0];
torso_z     = sock_floor + body_h;

// fail the render if things don't fit
collar_back = torso_y + chest_half[0] + sock_clear + collar_t;
assert(collar_back <= desk_d - desk_wall, "socket collar hits the back wall");
assert(chest_half[1] + sock_clear + collar_t <= inner_w / 2, "socket collar hits the side walls");
assert(pca_x - pca_pcb[0] / 2 - 2 >= -inner_w / 2 + rim_in && pca_x + pca_pcb[0] / 2 + 2 <= inner_w / 2 - rim_in, "PCA9685 too wide for the base");
assert(pca_y - pca_holes[1] / 2 - 2.5 >= desk_wall + rim_in && pca_y + pca_holes[1] / 2 + 2.5 <= desk_d - desk_wall - rim_in, "PCA9685 standoffs hit the rim");
assert(base_t + rim_h + 1 <= base_t + pca_standoff + pca_pcb[2], "rim too tall: the PCB's header hangs over it");
assert(pca_y0 + pca_pcb[1] + 1 <= desk_d - desk_wall, "PCA9685 too long for the base");
assert(base_t + pca_standoff + pca_pcb[2] + pca_plug_h + 15 <= sock_floor - ledge, "PCA9685 plugs too close to the socket");

lap_back    = lap_front + lap_d;
tab_y       = lap_front + lap_d / 2;   // tab and slot center
assert(lap_back + 0.5 <= torso_y - body_front - sock_clear, "laptop hits the robot");
assert(tab_h < top_t, "laptop tab is longer than the desk top is thick");

// torso outline in the desk frame (2D)
module socket_outline(grow = 0) {
    translate([0, torso_y]) rotate(-90) body_footprint(-grow);
}

module desk_box() {
    difference() {
        union() {
            // top, with vertical corners rounded and a small chamfer
            hull() {
                translate([0, 0, desk_h - top_t + 0.6]) linear_extrude(top_t - 1.2) top_plate_2d(0);
                translate([0, 0, desk_h - top_t]) linear_extrude(top_t) top_plate_2d(0.6);
            }
            // four walls, standing on the bottom plate
            translate([0, 0, base_t]) linear_extrude(desk_h - top_t - base_t + 0.01) difference() {
                translate([-desk_w / 2, 0]) square([desk_w, desk_d]);
                translate([-inner_w / 2, desk_wall]) square([inner_w, inner_d]);
            }
            // collar around the socket, under the top
            translate([0, 0, sock_floor - ledge])
                linear_extrude(desk_h - top_t - sock_floor + ledge + 0.01) socket_outline(sock_clear + collar_t);
        }
        // socket: straight down to the torso's bottom, then the 45-degree ledge
        translate([0, 0, sock_floor]) linear_extrude(desk_h) socket_outline(sock_clear);
        hull() {
            translate([0, 0, sock_floor]) linear_extrude(0.01) socket_outline(sock_clear);
            translate([0, 0, sock_floor - ledge]) linear_extrude(0.01) socket_outline(sock_clear - ledge);
        }
        translate([0, 0, -1]) linear_extrude(sock_floor + 1) socket_outline(sock_clear - ledge);
        // slot for the laptop's tab
        translate([-lap_w / 2 - tab_clear, tab_y - tab_w / 2 - tab_clear, desk_h - top_t - 1])
            cube([lap_w + 2 * tab_clear, tab_w + 2 * tab_clear, top_t + 2]);
        // cable slot in the back wall
        translate([cable_slot[0], desk_d + 1, cable_slot[1]]) rotate([90, 0, 0])
            linear_extrude(desk_wall + 2) offset(r = 2) offset(delta = -2)
                square([cable_slot[2], cable_slot[3]], center = true);
        // drawer line and handle slot on the front
        translate([-desk_w / 2 + 4, -1, desk_h - top_t - 16]) cube([desk_w - 8, 1.6, 0.8]);
        translate([-8, -1, desk_h - top_t - 9]) cube([16, 1.6, 2]);
    }
    socket_ribs();
    laptop_slot_ribs();
}

module laptop_slot_ribs() {
    for (x = [-lap_w / 4, lap_w / 4], sy = [-1, 1])
        translate([x, tab_y + sy * (tab_w / 2 + tab_clear), desk_h - top_t])
            cylinder(r = rib_r, h = top_t, $fn = 16);
}

// Laptop, in the desk frame, sitting in its slot. The keyboard side faces the
// robot (+y); the lid is hinged at the front edge and leans away from it.
module laptop_part() {
    hinge = [lap_front + lid_t / 2, desk_h + lap_t / 2];   // y, z
    difference() {
        union() {
            // base
            translate([-lap_w / 2, lap_front, desk_h]) cube([lap_w, lap_d, lap_t]);
            // tab, with a small lead-in chamfer at the bottom
            hull() {
                translate([-lap_w / 2, tab_y - tab_w / 2, desk_h - tab_h + 0.5]) cube([lap_w, tab_w, tab_h - 0.5 + 0.01]);
                translate([-lap_w / 2, tab_y - tab_w / 2 + 0.5, desk_h - tab_h]) cube([lap_w, tab_w - 1, 0.5]);
            }
            // lid
            translate([0, hinge[0], hinge[1]]) rotate([lid_lean, 0, 0])
                translate([-lap_w / 2, -lid_t / 2, 0]) cube([lap_w, lid_t, lid_h]);
        }
        // keys and trackpad, recessed into the base
        for (r = [0 : 4], c = [0 : 11])
            translate([-lap_w / 2 + 4.5 + c * 4, lap_front + 5 + r * 3.7, desk_h + lap_t - 0.4]) cube([3.2, 2.9, 1]);
        translate([-10, lap_front + 5 + 5 * 3.7, desk_h + lap_t - 0.4]) cube([20, 2.9, 1]);
        translate([-9, lap_back - 9, desk_h + lap_t - 0.3]) cube([18, 6, 1]);
        // screen, recessed into the lid's inner side, and a logo on the back
        translate([0, hinge[0], hinge[1]]) rotate([lid_lean, 0, 0]) {
            translate([-lap_w / 2 + 3, lid_t / 2 - 0.5, 4]) cube([lap_w - 6, 1, lid_h - 7]);
            translate([0, -lid_t / 2 + 0.3, lid_h / 2 + 1]) rotate([90, 0, 0]) cylinder(r = 3, h = 1, $fn = 32);
        }
    }
}

module top_plate_2d(inset) {
    offset(r = 3 - inset) offset(delta = -3)
        translate([-desk_w / 2 - lip, -lip]) square([desk_w + 2 * lip, desk_d + 2 * lip]);
}

// small vertical ribs on the socket walls; they press into the torso a bit
module socket_ribs() {
    hy = chest_half[0] + body_front;   // torso depth
    w  = chest_half[1] + sock_clear;
    y_front = torso_y - body_front - sock_clear;
    y_back  = torso_y + chest_half[0] + sock_clear;
    for (p = [[-15, y_front], [15, y_front], [15, y_back], [-20, y_back], [-w, torso_y], [w, torso_y]])
        translate([p[0], p[1], sock_floor]) cylinder(r = rib_r, h = desk_h - sock_floor, $fn = 16);
}

module desk_base() {
    difference() {
        union() {
            // plate, flush with the box's outside, with a small chamfer underneath
            hull() {
                translate([-desk_w / 2 + 0.5, 0.5, 0]) cube([desk_w - 1, desk_d - 1, 0.5]);
                translate([-desk_w / 2, 0, 0.5]) cube([desk_w, desk_d, base_t - 0.5]);
            }
            // rim that fits inside the walls
            translate([0, 0, base_t - 0.01]) linear_extrude(rim_h) difference() {
                translate([-inner_w / 2 + rim_clear, desk_wall + rim_clear]) square([inner_w - 2 * rim_clear, inner_d - 2 * rim_clear]);
                translate([-inner_w / 2 + rim_in, desk_wall + rim_in]) square([inner_w - 2 * rim_in, inner_d - 2 * rim_in]);
            }
        }
        // zip tie slots: pairs, 5 mm apart across
        for (t = tie_slots, dx = [-2.5, 2.5])
            translate([t[0] + dx - 0.9, t[1] - 1.75, -1]) cube([1.8, 3.5, base_t + 2]);
    }
    // crush ribs on the rim's outside, two per side
    for (f = [0.3, 0.7]) {
        for (sx = [-1, 1]) translate([sx * (inner_w / 2 - rim_clear), desk_wall + f * inner_d, base_t]) cylinder(r = rib_r, h = rim_h, $fn = 16);
        for (y = [desk_wall + rim_clear, desk_d - desk_wall - rim_clear]) translate([-inner_w / 2 + f * inner_w, y, base_t]) cylinder(r = rib_r, h = rim_h, $fn = 16);
    }
    // standoffs with pins for the PCA9685
    for (sx = [-1, 1], sy = [-1, 1])
        translate([pca_x + sx * pca_holes[0] / 2, pca_y + sy * pca_holes[1] / 2, base_t - 0.01]) {
            cylinder(d = 5, h = pca_standoff, $fn = 32);
            cylinder(d = pca_pin_d, h = pca_standoff + pca_pcb[2] + 1, $fn = 24);
            translate([0, 0, pca_standoff + pca_pcb[2] + 1])
                cylinder(d1 = pca_pin_d, d2 = pca_pin_d - 0.8, h = 0.6, $fn = 24);
        }
}

// PCA9685 board model, for checking and the scene (not printed)
module pca9685_model() {
    translate([pca_x, pca_y, base_t + pca_standoff]) {
        color("#1f4fa8") translate(-[pca_pcb[0], pca_pcb[1], 0] / 2) cube(pca_pcb);
        color("#2b2d31") translate([-4, -4, 1.6]) cube([8, 8, 1.4]);
        color("#c9ccd1") translate([-6, -20, 1.6]) cylinder(d = 8, h = 10, $fn = 24);
        color("#2fb352") translate([-8, -8, 1.6]) cube([7, 10, 10]);
        for (g = [0 : 3]) translate([pca_pcb[0] / 2 - 9, -26 + g * 14.5, 1.6]) {
            color("#2b2d31") cube([7.6, 10.2, pca_plug_h]);
            color("#f1d23a") translate([2.5, 0, pca_plug_h - 1]) cube([2.6, 10.2, 1]);
        }
        color("#2b2d31") translate([-7.6, -pca_pcb[1] / 2 - pca_header, 1.6]) cube([15.2, pca_header, 2.5]);
    }
}

// the torso placed in the desk frame
module torso_on_desk() {
    translate([0, torso_y, torso_z]) rotate(-90) children();
}
