// Test block for the servo shaft socket in the head post (HeadM5Dial).
//
// The head prints face down, so its socket prints sideways, and sideways holes
// come out bigger than upright ones. This block prints its sockets the same
// way, with the same depth, screw hole and screwdriver channel as the head.
//
// Sizes are the distance across the hex flats in mm (the head uses 3.86).
// Press each socket onto the servo shaft and pick the one that grips firmly
// and sits all the way down. Then tell Claude the size, or set shaft_r in
// HeadM5Dial.scad to half of it.
//
// Print as exported, labels up. No supports needed.

sizes        = [3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9];
socket_depth = 2.4;    // same as HeadM5Dial.scad
web_t        = 1.5;
screw_r      = 0.8;
access_r     = 2.5;
block_len    = 7;      // along the socket axis
block_h      = 12;     // print height; the sockets sit in the middle
pitch        = 9;
margin       = 5;
text_size    = 2.5;
text_deep    = 0.5;

block_w = 2 * margin + (len(sizes) - 1) * pitch;

function fmt(v) = v == floor(v) ? str(v, ".0") : str(v);
function sock_y(i) = margin + i * pitch;

// Socket in the head's own frame (axis along z, opening at z = 0), turned
// into print orientation the same way the head is (rotate [0, 90, 0]).
module head_socket(flats) {
    rotate([0, 90, 0]) {
        translate([0, 0, -0.01])
            rotate([0, 0, 30]) cylinder(r = flats / 2 / cos(30), h = socket_depth + 0.01, $fn = 6);
        translate([0, 0, -1]) cylinder(r = screw_r, h = block_len + 2, $fn = 24);
        translate([0, 0, socket_depth + web_t]) cylinder(r = access_r, h = block_len, $fn = 40);
    }
}

difference() {
    cube([block_len, block_w, block_h]);
    for (i = [0 : len(sizes) - 1])
        translate([0, sock_y(i), block_h / 2]) head_socket(sizes[i]);
    for (i = [0 : len(sizes) - 1])
        translate([block_len / 2, sock_y(i), block_h - text_deep])
            linear_extrude(text_deep + 1)
                rotate([0, 0, 90])
                    text(fmt(sizes[i]), size = text_size, halign = "center", valign = "center");
}
