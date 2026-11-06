// Test plate for the servo shaft socket used by UpperArm.
//
// Printed like UpperArm, with the sockets standing upright. (The head post
// prints its socket sideways, which changes the size; see HeadSocketTest.scad.)
//
// Hex sockets in 9 sizes (rows) and 3 depths (columns). Size is the distance
// across the flats in mm; the original parts use 3.86. Depth is in mm.
// Press each socket onto the servo shaft and pick the one that grips firmly
// without cracking and sits fully down. Then tell Claude the size and depth,
// or use them in the parts.
//
// Print with the socket openings facing up. No supports needed.

sizes      = [4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8, 4.9, 5.0];  // across flats, mm (3.5-4.2 were all too tight)
depths     = [3, 4, 5];                                  // socket depth, mm
web        = 1;      // floor under the deepest socket (as in UpperArm)
screw_r    = 0.8;    // servo screw hole through the floor
chamfer    = 0.4;    // lead-in at the opening
pitch_x    = 10;     // column spacing
pitch_y    = 9;      // row spacing
label_w    = 12;     // left margin for size labels
header_h   = 8;      // top margin for depth labels
margin     = 5;
text_size  = 3;
text_deep  = 0.5;

plate_t = max(depths) + web;
plate_w = label_w + (len(depths) - 1) * pitch_x + 2 * margin;
plate_h = margin + (len(sizes) - 1) * pitch_y + margin + header_h;

function fmt(v) = v == floor(v) ? str(v, ".0") : str(v);   // 4 -> "4.0"
function col_x(j) = label_w + margin + j * pitch_x;
function row_y(i) = margin + i * pitch_y;

module hex_socket(flats, depth) {
    r = flats / 2 / cos(30);   // corner radius
    translate([0, 0, plate_t - depth])
        cylinder(r = r, h = depth + 1, $fn = 6);
    // lead-in chamfer
    translate([0, 0, plate_t - chamfer])
        cylinder(r1 = r, r2 = r + chamfer, h = chamfer + 0.01, $fn = 6);
    // screw hole through the floor
    translate([0, 0, -1]) cylinder(r = screw_r, h = plate_t + 2, $fn = 24);
}

module label(s, x, y) {
    translate([x, y, plate_t - text_deep])
        linear_extrude(text_deep + 1)
            text(s, size = text_size, halign = "center", valign = "center");
}

difference() {
    cube([plate_w, plate_h, plate_t]);
    for (i = [0 : len(sizes) - 1], j = [0 : len(depths) - 1])
        translate([col_x(j), row_y(i), 0]) hex_socket(sizes[i], depths[j]);
    for (i = [0 : len(sizes) - 1])
        label(fmt(sizes[i]), label_w / 2 + 1, row_y(i));
    for (j = [0 : len(depths) - 1])
        label(str("d", depths[j]), col_x(j), plate_h - header_h / 2 - 1);
}
