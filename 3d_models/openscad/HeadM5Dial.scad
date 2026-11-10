// Fixed head with an M5Stack Dial as the robot's face.
//
// Replaces Head, Hat and Neck, and the head (pitch) servo. The head sits
// directly on the neck (yaw) servo. The servo's cross horn lies inside the
// head in an X-shaped pocket in a square pad on top of the floor (see
// horn_mount.scad for the horn shape), so the head can't
// twist. The horn's hub points down into a 5.5 mm hole through the floor,
// and the servo shaft comes up into it from below.
//
// The head is a box with an open back. The Dial goes in from the back, screen
// first: its 45 mm front cap rests against the inside of the front plate and
// the screen shows through the 44.5 mm hole. The orange rings sit inside the
// box with 1 mm of air all around, so they can turn. The Dial goes in with its
// USB-C port facing down, and the plug goes up into it through a slot in the
// floor under the front half of the Dial. The slot also opens out the bottom
// of the front plate. The horn sits behind it, at the back of the floor.
//
// Assembly: put the cross horn into the pocket, hub pointing down. Center the
// neck servo, press the head onto it facing forward, and fasten the center
// screw through the hole in the top of the head. Then slide the Dial in from
// the back and plug in the cables.
//
// Head frame: +x front, +y robot width, +z up. The front face is at x = 0
// and the neck servo shaft axis is at x = shaft_x, y = 0. The underside of
// the floor is at z = 0.
//
// Outside (styled like improve_robot.jpg): all edges are rounded, with a seam
// groove around the head behind the face. The front edge is rounded with a 45-degree
// start, so it prints face down without supports.
//
// Output is in print orientation: face down. No supports needed.

include <horn_mount.scad>

// ── Dial (from M5Stack's 3D model) ─────────────────────────────────────────
bore_r          = 22.25;  // screen hole in the front plate: 44.5 mm
ring_r          = 25.41;  // largest orange ring
ring_air        = 1;      // air around the orange rings
dial_len        = 30.32;  // front of the screen cap to the back of the Dial

// ── Head box ───────────────────────────────────────────────────────────────
front_t         = 4;      // front plate thickness
wall            = 2.5;
floor_t         = 3;
head_r          = 5;      // corner radius (seen from the front)
back_room       = 1.7;    // extra depth behind the Dial
front_round     = 4;      // radius of the rounded front edge
back_round      = 1.5;    // radius of the rounded back edge
seam_x          = 8;      // seam groove, this far behind the front
seam_w          = 0.8;
seam_depth      = 0.6;

// ── Horn mount on the floor ────────────────────────────────────────────────
horn_wall       = 1.5;    // pad material around the horn pocket
horn_pocket     = 1;      // height of the square pad = depth of the horn pocket
horn_stack      = 3.2;    // horn plus center screw head, above the floor
stack_air       = 0.8;    // air between the screw head and the Dial

// ── USB-C slot in the floor ────────────────────────────────────────────────
usb_w           = 15;     // slot width (across the robot)

// ── Derived ────────────────────────────────────────────────────────────────
head_bottom = 0;                                              // underside of the floor
dial_zc     = floor_t + horn_stack + stack_air + ring_r;     // Dial center height
head_w      = 2 * (ring_r + ring_air + wall);
head_h      = dial_zc + ring_r + ring_air + wall;
head_d      = front_t + dial_len + back_room;
head_top    = head_bottom + head_h;
horn_pad    = horn_extent + horn_wall;
shaft_x     = -head_d + horn_pad;                             // horn pad ends at the back of the head
usb_slot_x  = shaft_x + horn_pad;                             // USB slot runs from here to the front

// front view (y, z) rounded rectangle
module head_outline(inset = 0) {
    translate([-head_w / 2 + inset, head_bottom + inset])
        offset(r = max(head_r - inset, 0.01))
            offset(delta = -max(head_r - inset, 0.01))
                square([head_w - 2 * inset, head_h - 2 * inset]);
}

// extrude a front-view (y, z) shape along x, from x0 to x1
module along_x(x0, x1) {
    translate([x0, 0, 0]) rotate([90, 0, 90]) linear_extrude(x1 - x0) children();
}

// how far the outline is inset at distance d from a rounded edge of radius r.
// Starts at 45 degrees and blends into the circle, so it prints on the bed.
function edge_inset(d, r) =
    d < 0.293 * r ? 0.586 * r - d : r - sqrt(r * r - (r - d) * (r - d));

// outside of the head: rounded front and back edges
module head_outer() {
    n = 10;
    hull() {
        for (i = [0 : n]) let(d = i / n * front_round)
            along_x(-d - 0.01, -d) head_outline(edge_inset(d, front_round));
        for (i = [0 : n]) let(d = i / n * back_round)
            along_x(-head_d + d, -head_d + d + 0.01) head_outline(back_round - sqrt(back_round * back_round - (back_round - d) * (back_round - d)));
    }
}

module seam_groove() {
    along_x(-seam_x - seam_w / 2, -seam_x + seam_w / 2)
        difference() { head_outline(-1); head_outline(seam_depth); }
}

module head_box() {
    difference() {
        head_outer();
        // hollow inside, open at the back; floor and front plate stay solid
        difference() {
            along_x(-head_d - 1, -front_t) head_outline(wall);
            translate([-head_d - 2, -head_w, head_bottom - 1])
                cube([head_d + 2, 2 * head_w, floor_t + 1]);
        }
    }
}

module screen_hole() {
    along_x(-front_t - 1, 1) translate([0, dial_zc]) circle(r = bore_r, $fn = 160);
}

module usb_slot() {
    // through the floor under the front half of the Dial, from the horn pad
    // to the front, and out the bottom of the front plate
    translate([usb_slot_x, -usb_w / 2, -1])
        cube([-usb_slot_x + 1, usb_w, floor_t + 2]);
}

// square pad on top of the floor, around the horn
module horn_rim() {
    translate([shaft_x - horn_pad, -horn_pad, head_bottom + floor_t - 0.01])
        cube([2 * horn_pad, 2 * horn_pad, horn_pocket + 0.01]);
}

module servo_mount() {
    translate([shaft_x, 0, head_bottom]) {
        // horn pocket in the pad, down to the top of the floor
        translate([0, 0, floor_t]) linear_extrude(horn_pocket + 1) horn_x_2d();
        // hub hole through the floor, and screwdriver access through the top
        translate([0, 0, -1]) cylinder(d = hub_hole_d, h = head_h + 2, $fn = 48);
    }
}

module head_m5dial() {
    difference() {
        union() {
            head_box();
            horn_rim();
        }
        screen_hole();
        usb_slot();
        servo_mount();
        seam_groove();
    }
}

// Print orientation: front face down on the bed.
if (is_undef(head_m5dial_no_output)) rotate([0, 90, 0]) head_m5dial();
