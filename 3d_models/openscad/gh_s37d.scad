// Tiny Engineer parts adapted from the EMAX ES9051 export to fit a GH-S37D servo.
//
// The ES9051 and GH-S37D are close in size. The main difference is the
// mounting-tab span, so each servo pocket's tab slot is lengthened at both
// ends. Everything else is the unchanged ES9051 mesh.
//
// Change the sizes below to match your servo (measure with calipers), then
// run ./build.sh to regenerate the STLs.

// ── GH-S37D sizes (mm) ──────────────────────────────────────────────────────
servo_body_width  = 20;    // body length, tabs not included
servo_mount_width = 5.45;  // how far each tab sticks out from the body (5.2 from listings was too short)
clearance         = 0.3;   // gap per side, same as the original model

// ES9051 values the base meshes were exported with (do not change)
base_body_width   = 20;
base_mount_width  = 4.6;

// ── Tab slots in the ES9051 meshes ─────────────────────────────────────────
// Found by measuring the meshes. All slots run along X.
// [center_x, half_span, [y_min, y_max], [z_min, z_max]]
// The y/z ranges include the open side the servo is dropped in from.
slots = [
    ["Chest", [
        [-0.950, 14.900, [-21.30, -19.60], [16.30, 27.82]],   // arm servo
        [-0.950, 14.900, [19.60, 21.30], [16.30, 27.82]],     // arm servo
        [-0.950, 14.900, [-4.70, 4.70], [19.90, 27.82]]       // neck servo
    ]],
];

new_half_span = servo_body_width / 2 + servo_mount_width + clearance;

function slots_for(part) = [for (s = slots) if (s[0] == part) s[1]][0];

// Cut both ends of each tab slot out to the new span.
// If the new span is shorter, nothing is cut (the servo still fits).
module tab_slot_extensions(part) {
    eps = 0.01;
    for (s = slots_for(part)) {
        cx = s[0]; hs = s[1]; y = s[2]; z = s[3];
        if (new_half_span > hs)
            for (dir = [-1, 1]) {
                x0 = cx + dir * (hs - eps);
                x1 = cx + dir * new_half_span;
                // eps overlap on y/z keeps the cut from sharing faces with
                // the slot walls, which gives clean manifold output
                translate([min(x0, x1), y[0] - eps, z[0] - eps])
                    cube([abs(x1 - x0), y[1] - y[0] + 2 * eps, z[1] - z[0] + 2 * eps]);
            }
    }
}

module adapted_part(part) {
    difference() {
        import(str("base/es9051/", part, ".stl"), convexity = 10);
        tab_slot_extensions(part);
    }
}
