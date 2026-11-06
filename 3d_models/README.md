# 3D models (Tiny Engineer, GH-S37D)

Printable parts for the [Tiny Engineer](https://github.com/jamro/tiny-engineer/tree/main/3d_models) desk robot, set up for **GH-S37D** micro servos (3.7 g, 20 × 8.75 × 22 mm).

**Printable STLs are in [`parts/gh-s37d/stl/`](parts/gh-s37d/stl/).** Print `ServoSizingTester.stl` first.

They are made with OpenSCAD from the EMAX ES9051 export, which is within about 0.5 mm of the GH-S37D except for the mounting-tab span. Each of the five servo pockets (Chest ×3, Head, Chair) has its tab slot lengthened by 0.85 mm at each end, to 31.5 mm. All other parts are the ES9051 files unchanged.

## M5Dial head

`HeadM5Dial.stl` is a fixed head with an M5Stack Dial as its face. It replaces `Head.stl`, `Hat.stl` and `Neck.stl`, and the head (pitch) servo, so the robot uses four servos. The head sits directly on the neck servo, with no neck or post in between, and turns left and right with it.

- **Shape:** a 58 × 61 × 36 mm box with an open back. The Dial goes in from the back, screen first. Its 45 mm screen cap rests against the inside of the 4 mm front plate, and the screen shows through the 44.5 mm hole. Both orange rings sit inside the box with 1 mm of air at the sides and top (1.4 mm below), so they can turn. The servo's cross horn lies in an X-shaped pocket in a 1 mm square pad at the back of the floor, under the back half of the Dial. Its hub points down into a 5.5 mm hole through the floor, and the servo shaft comes up into it from below.
- **Print** face down, as exported, with no supports.
- **Assembly:**
  1. Center the neck servo.
  2. Put the servo's cross horn into the X-shaped pocket inside the head, hub pointing down.
  3. Press the head down so the shaft goes up through the floor into the horn, with the head facing forward. The pocket stops the head from twisting on the horn.
  4. Fasten it with the center screw through the 5.5 mm hole in the top of the head.
  5. Slide the Dial in from the back, screen first, with USB-C facing down.
- **Cables:** the USB-C plug goes up into the Dial through a 15 mm wide slot in the floor under the front half of the Dial. The slot runs from the horn pad to the front plate and opens out the bottom of the plate, so the plug can come in at an angle. Port A/B face the open back.
- **Fit:** checked against M5Stack's 3D model of the Dial ([K130 structure files](https://github.com/m5stack/M5_Hardware/tree/master/Products/K130_Dial/Structures)): the orange rings have at least 1 mm of air to the walls (0.65 mm to the front plate, set by the Dial's own shape), the Dial clears the horn and center screw, and there's a straight path for a 12.4 × 6.5 mm USB-C plug. Settings to change in `openscad/HeadM5Dial.scad`:
  - `horn_pocket`: height of the square pad, which is also the depth of the horn pocket
  - `bore_r`: radius of the screen hole in the front plate
  - `ring_air`: air around the orange rings (the box grows with it)
  - `usb_w` / `usb_slot_back`: for a bigger plug, or to move the slot further back

## One-piece arms

`ArmLeft.stl` and `ArmRight.stl` replace `UpperArm.stl` (×2), `ForearmLeft.stl`, `ForearmRight.stl` and the two elbow screws.

- **Elbow:** the original joint, fixed. The upper arm sits in the forearm's tilted elbow pocket at the original angle, with the forearm leaning in toward the body. The upper arm is turned 90° from the first version, after a test fit.
- **Shoulder socket:** the 3.4 mm hex that fit best in `HeadSocketTest`, with the same 2.4 mm depth, servo screw hole and screw head recess. It prints sideways, like in the test, so the fit should carry over.
- **Print:** as exported. The upper arm lies flat with the hub on its side, and the forearm rises gently from the elbow. **Turn on supports for the hand** (tree supports work well). To make it lie flat, the hub and the underside of the elbow ball are flattened on the bed side.
- **Mount:** center the shoulder servo, press the socket onto the shaft with the arm in the pose you want, and fasten it with the servo screw from the outside.

To change the socket, edit `socket_flats` (and the other socket sizes) in `openscad/arm_onepiece.scad` and run `build.sh`.

### Face plate test

`FacePlateTest.stl` finds how thick the head's face plate can be before it blocks the USB-C plug. It's a ring with the same hole as the head, 1 to 6 mm thick in six numbered steps.

1. Press the Dial into the hole so its flange sits on the flat front face.
2. Turn the Dial so its USB-C port faces a step, and try to plug in the cable from behind.
3. The thickest step where the plug still goes in fully is the most the face plate can be. Set `front_t` in `openscad/HeadM5Dial.scad` to that number and run `build.sh`.

Print front face down; it takes a few minutes.

### Horn mount test

`HornMountTest.stl` checks the cross-horn recess before you print the whole head. It's a 20 mm block with the same X-shaped recess, 5.5 mm hub hole and pilot holes as in the head's floor, printed the same way. The horn should drop in without forcing and not twist, and the pilot holes should line up with the holes in its arms.

The horn sizes are estimates. If it doesn't fit, measure the horn and change the sizes in `openscad/horn_mount.scad` (arm length from the center, arm width at the hub and at the tip, thickness, and the hole positions), then run `build.sh`. The head updates too.

### Shaft socket tests

Holes come out a different size depending on how they're printed, so there are two tests, each printed like the part it's for:

- `ShaftSocketTest.stl` is for `UpperArm`, whose socket prints upright. It's a 42 × 90 mm plate with hex sockets in 9 sizes (4.2–5.0 mm across the flats) and 3 depths (3, 4 and 5 mm). A first round with 3.5–4.2 mm was all too tight. Print with the sockets facing up.
- `HeadSocketTest.stl` was for the head post's old hex socket, which the cross-horn mount has replaced. The socket prints sideways because the head prints face down. Sideways holes come out bigger: the head's 3.86 mm socket was too loose. It's a 7 × 73 × 12 mm block with 8 sizes (3.2–3.9 mm) and the same depth, screw hole and screwdriver channel as the head. Print as exported, labels up.

Press each socket onto the servo shaft. Pick the one that grips firmly, sits all the way down and doesn't crack. Neither test needs supports.

### Test stand

`NeckTestStand.stl` lets you try the M5Dial head without printing the robot. It's an 80 × 80 mm base with a pocket that holds one GH-S37D servo shaft-up.

1. Thread the servo cable through the tunnel at the end of the pocket.
2. Drop the servo in so it rests on its tabs, and fasten the tabs with two M2 screws.
3. Press the head onto the shaft and fasten it with the servo screw through the top of the head.

Print base down; it needs no supports.

## Folders

- `openscad/`: OpenSCAD source for the STLs
  - `gh_s37d.scad`: servo sizes (edit these) and the tab-slot positions
  - `Chest.scad`, `Head.scad`, `Chair.scad`, `ServoSizingTester.scad`: the adapted parts
  - `HeadM5Dial.scad`: the fixed M5Dial head (doesn't use `gh_s37d.scad`)
  - `NeckTestStand.scad`: test stand for the head
  - `FacePlateTest.scad`: test ring for the face plate thickness
  - `ShaftSocketTest.scad`, `HeadSocketTest.scad`: servo shaft socket tests for `UpperArm` and the old head post
  - `horn_mount.scad`: cross-horn recess shared by the head and `HornMountTest.scad`
  - `ArmLeft.scad`, `ArmRight.scad`, `arm_onepiece.scad`: one-piece arms
  - `base/es9051/`: original ES9051 meshes
  - `build.sh`: writes all STLs to `parts/gh-s37d/stl/`
- `parts/gh-s37d/stl/`: the printable STLs
- `cad/`, `fusion/`: the original Fusion 360 model and add-in, with a `GH-S37D` preset added to `servos.json`. Only needed if you have Fusion and want a fully re-sized model.

## Rebuild after measuring your servo

1. Measure the tab span (tip to tip across both mounting tabs) with calipers.
2. In `openscad/gh_s37d.scad`, set `servo_mount_width = (span - 20) / 2`.
3. Run `openscad/build.sh` (needs `openscad` on your PATH).

## Caveats

- The tab span is set to 30.9 mm. The seller listing's 30.4 mm was too short for a real servo, so this was raised by 0.5 mm without measuring. Measure yours if it still doesn't fit.
- The screw pilot holes for the servo tabs are still at the ES9051 positions, about 0.4 mm inward from the GH-S37D tab holes. M2 self-tapping screws should still bite. If they don't line up, use the tiny screws that came with the servo or skip the tab screws.
- The wall left beyond some slot ends is about 0.6 mm (was 1.5 mm): one side of each Chest slot, the Head slot and the tester. That is about one line wide on a 0.4 mm nozzle. Use the Arachne wall generator (the default in Bambu Studio) and check in the slicer preview that those walls are printed.
- The GH-S37D body is about 0.4 mm shorter than the ES9051's, so it sits on its tabs with a small gap below. That's fine.

## Fusion route (optional)

If you have Fusion 360, you can export a fully re-sized model instead:

1. In Fusion, add the add-in: **Shift+S → Add-Ins → +** and select `fusion/TinyEngineerTools/`, then **Run**.
2. Open `cad/TinyEngineer.f3d`.
3. **Utilities → Add-ins → Tiny Engineer Parts Exporter**. Under **Servos**, untick everything except **GH-S37D**. Pick formats.
4. Choose this repo's `3d_models/parts/` as the export folder. Files land in `parts/gh-s37d/`.

Full docs: [parametric design](https://github.com/jamro/tiny-engineer/blob/main/docs/3d/parametric-design.md).

### Fusion preset values (estimated)

Sellers only publish the overall size (20 × 8.75 × 22 mm, about 30.4 mm across the mounting tabs). The other values are estimates based on the similar EMAX ES9051 preset:

| Parameter | Value | Source |
|---|---|---|
| `servo_body_width` | 20 mm | listings |
| `servo_body_depth` | 8.75 mm | listings |
| `servo_mount_width` | 5.2 mm | listings: (30.4 − 20) / 2 |
| `servo_body_height` + `servo_gearbox_height` + `servo_shaft_housing_height` | 14.5 + 4 + 3.5 = 22 mm | listings give 22 mm total; split estimated |
| `servo_mount_thickness` | 1 mm | estimate |
| `servo_gearbox_padding` | 8 mm | estimate (ES9051) |
| `servo_shaft_position` | 4.4 mm | estimate (ES9051) |
| `servo_shaft_length` | 2.9 mm | estimate (ES9051) |
| `servo_shaft_radius` | 1.9 mm | estimate |
| `servo_mount_hole_radius` | 1.1 mm | estimate |

If the Fusion-exported `ServoSizingTester` doesn't fit, measure with calipers, fix the values in `servos.json` (see the diagram in the docs), reload the add-in and export again. Print `ScrewSizingTest` too, for the M2 screw holes.

## License

CERN-OHL-S-2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE). The source location is https://github.com/jamro/tiny-engineer/tree/main/3d_models. The Tiny Engineer name and logo are not licensed.
