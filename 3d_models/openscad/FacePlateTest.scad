// Test ring for finding the face plate thickness of the M5Dial head.
//
// The Dial presses into the hole with its flange on the flat front face.
// The ring gets thicker in six steps around the hole (numbers on the back,
// in mm). Turn the Dial so its USB-C port faces a step and try the plug.
// The thickest step where the plug still goes in fully is the most the face
// plate can be (front_t in HeadM5Dial.scad).
//
// Print front face down. No supports needed.

bore_r     = 22.25;  // same hole as the head (Dial thread is 44 mm)
outer_r    = 34;
steps      = [1, 2, 3, 4, 5, 6];  // thickness of each step, mm
label_size = 4.5;
label_r    = 29.5;   // radius of the step numbers
label_deep = 0.6;

n = len(steps);
sector = 360 / n;

module sector_2d(a0, a1, r) {
    // pie slice from angle a0 to a1
    polygon(concat([[0, 0]], [for (a = [a0 : (a1 - a0) / 30 : a1]) r * [cos(a), sin(a)]]));
}

difference() {
    union() {
        for (i = [0 : n - 1])
            linear_extrude(steps[i])
                intersection() {
                    circle(r = outer_r, $fn = 180);
                    // 0.5 deg overlap so neighboring steps fuse into one solid
                    sector_2d(i * sector - 0.5, (i + 1) * sector + 0.5, outer_r + 1);
                }
    }
    translate([0, 0, -1]) cylinder(r = bore_r, h = max(steps) + 2, $fn = 180);
    // step numbers, engraved into the back of each step
    for (i = [0 : n - 1]) {
        a = (i + 0.5) * sector;
        translate([label_r * cos(a), label_r * sin(a), steps[i] - label_deep])
            linear_extrude(label_deep + 1)
                text(str(steps[i]), size = label_size, halign = "center", valign = "center");
    }
}
