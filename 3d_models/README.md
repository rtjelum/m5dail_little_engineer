# 3D models (Tiny Engineer, GH-S37D)

Printable parts for the [Tiny Engineer](https://github.com/jamro/tiny-engineer/tree/main/3d_models) desk robot, set up for **GH-S37D** micro servos (3.7 g, 20 × 8.75 × 22 mm).

**Printable STLs are in [`parts/gh-s37d/stl/`](parts/gh-s37d/stl/).**

The robot is made of these parts, with three servos (two arms and the neck):

- `Torso.stl`
- `HeadM5Dial.stl`
- `ArmLeft.stl` and `ArmRight.stl`
- `DeskBox.stl`, `DeskBase.stl` and `Laptop.stl`

`HeadSocketTest.stl` is a small test print for the arms' shaft socket (see below).

They are made with OpenSCAD. The torso's servo block and the arms start from the EMAX ES9051 export (`openscad/base/es9051/`), which is within about 0.5 mm of the GH-S37D except for the mounting-tab span. The three servo pockets in the torso have their tab slots lengthened by 0.85 mm at each end, to 31.5 mm. The rest of the original Tiny Engineer parts aren't used and are only kept in `openscad/base/es9051/`.

## M5Dial head

`HeadM5Dial.stl` is a fixed head with an M5Stack Dial as its face. It replaces the original `Head`, `Hat` and `Neck`, and the head (pitch) servo. The head sits directly on the neck servo, with no neck or post in between, and turns left and right with it.

- **Shape:** a 58 × 61 × 36 mm box with an open back. The Dial goes in from the back, screen first. Its 45 mm screen cap rests against the inside of the 4 mm front plate, and the screen shows through the 44.5 mm hole. Both orange rings sit inside the box with 1 mm of air at the sides and top (1.4 mm below), so they can turn. The servo's cross horn lies in an X-shaped pocket in a 1 mm square pad at the back of the floor, under the back half of the Dial. Its hub points down into a 5.5 mm hole through the floor, and the servo shaft comes up into it from below.
- **Look:** all outside edges are rounded, with a seam groove 8 mm behind the face, for a boxy robot look. The front edge starts at 45 degrees, so it still prints face down without supports.
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

## Torso

`Torso.stl` replaces the original `Chest`, `Belly`, `BellyJoint`, `Bell`, `BottomCover` and `Chair`, with a boxy robot look. The robot now uses three servos: two arms and the neck.

- **Servos and joints unchanged:** the top 25.7 mm is the Chest exactly as before, with the GH-S37D tab slots. The arm servos lie in it with their shafts out the sides, the neck servo stands in the middle, and the arms and the M5Dial head fit as before.
- **Lower body:** a hollow 32 mm box under the Chest, with a seam line, panel lines, vents and a speaker grille. Its front is set back 3 mm from the Chest's, with a 45-degree slope up to the Chest so there's no overhang to print, and the Chest's side scoops are kept, so the arms have as much room as before. With the longer upper arms (see below), the arms clear the body all the way down to the typing pose, 70 degrees below horizontal toward the front. Past about 72 degrees, the forearm touches the front corner of the lower body.
- **Cables:** the servo cables go down through the Chest's holes and out the open bottom into the desk. A 6.5 × 3.5 mm groove runs down the back for the Dial's cables. It carries on into the desk socket.
- **Print:** as exported, open bottom down, no supports. The inside has a 45-degree roof.

## Desk

The robot stands in a closed desk box, as wide as its shoulders (72 × 80 mm, 50 mm tall), with the PCA9685 servo board inside.

- **`DeskBox.stl`:** the top and all four walls; only the bottom is open. The torso drops 8 mm into a socket in the top and stops on a 45-degree ledge, with no screws. Six small ribs in the socket walls press into the torso by 0.35 mm to hold it snug. The back wall has a 14 × 8 mm slot for the servo power cable and the Dial's USB cable. Print as exported: upside down, top on the bed, no supports.
- **`DeskBase.stl`:** the bottom plate, flush with the outside of the box. A 3 mm rim on it presses up inside the walls, with small crush ribs, and closes the bottom. The PCA9685 presses onto four 2.3 mm pins on 5 mm standoffs, long side front to back, with the servo plugs toward the robot's left. Four pairs of slots take zip ties to hold the wires down. Lift the box off the plate to get at the wiring. Print plate down.
- **`Laptop.stl`:** one piece with the lid open, keys, trackpad and a screen recess. A full-width tab underneath drops into a slot in the desk top in front of the robot, held by small ribs like the torso. Print as exported, lying on its side, no supports.
- **Walls:** 4 mm thick, so the box is sturdy and prints with solid walls.
- **Fit:** inside, the box is 64 mm wide, 72 mm deep and 44 mm tall. That's as low as it goes: the board with its plugs needs about 23 mm, plus about 15 mm above the plugs for the wires to bend. The board with plugs on needs 25.4 × 68.5 × 20.6 mm. `desk_parts.scad` stops with an error if a size change makes the board, socket or base stop fitting.

## Display scene

`openscad/RobotScene.scad` puts the printable parts together: `Torso`, `HeadM5Dial`, the one-piece arms in a typing pose, `DeskBox`, `DeskBase`, `Laptop` and a model of the PCA9685. The screens and the face on the Dial are props. It isn't printable, so `build.sh` skips it. Open it in OpenSCAD and press F5, or see [`renders/`](renders/). Render with `-D cutaway=true` to hide a desk wall and show the board, or change `arm_angle`.

Two values in the scene are estimates: how high the head sits on the neck servo, and where the arm hub sits on the shoulder shaft. With the arms at 70 degrees below horizontal (`arm_angle = -70`), the claws rest on the keyboard. The arms don't touch the torso or the desk at that pose, and they stay at least 2.5 mm from the laptop screen as they swing up and down.

## One-piece arms

`ArmLeft.stl` and `ArmRight.stl` replace the original `UpperArm` (×2), `ForearmLeft`, `ForearmRight` and the two elbow screws.

- **Longer upper arm:** the upper arm is 12 mm longer than the original (32.6 mm instead of 20.6 mm from the shoulder hub to the end of the bar), so the claws reach the laptop keys. Change `upper_ext` in `openscad/arm_onepiece.scad`. Longer arms pass too close to the laptop screen (0.5 mm at 15 mm longer), and shorter ones need a steeper angle, where the forearm hits the torso.
- **Elbow:** fixed, at the original angle. The upper arm sits where it sat in the forearm's tilted elbow pocket, with the forearm leaning in toward the body. The upper arm is turned 90° from the first version, after a test fit.
- **Square elbow:** the round elbow ball is replaced by a flat-faced block, made as the hull of the end of the upper arm and the start of the forearm. Both bars run straight into it, and the forearm is the original, with nothing cut off (only its ball is removed). The block's underside is flat and level with the arm's flat print side. Change `elbow_grow` (how much bigger than the bars the block is) in `openscad/arm_onepiece.scad`.
- **Robot hands:** the original human-style hands are replaced by simple robot claws: a square wrist collar, a block palm, two thick prongs with bent tips, and a thumb. Every part is at least 3 mm thick, with no small details, so they print easily. Sizes are at the top of the "Robot hand" section in `openscad/arm_onepiece.scad`.
- **Shoulder socket:** the 3.4 mm hex that fit best in `HeadSocketTest`, with the same 2.4 mm depth, servo screw hole and screw head recess. It prints sideways, like in the test, so the fit should carry over.
- **Print:** as exported. The upper arm lies flat with the hub on its side, and the forearm rises gently from the elbow. **Turn on supports for the hand** (tree supports work well). To make it lie flat, the hub is flattened on the bed side, and the elbow block's underside is level with it.
- **Mount:** center the shoulder servo, press the socket onto the shaft with the arm in the pose you want, and fasten it with the servo screw from the outside.

To change the socket, edit `socket_flats` (and the other socket sizes) in `openscad/arm_onepiece.scad` and run `build.sh`.

### Head socket test

`HeadSocketTest.stl` is a 7 × 73 × 12 mm block with hex sockets in 8 sizes (3.2–3.9 mm across the flats), printed sideways. Sideways holes come out bigger than upright ones. The arms' shoulder socket uses the 3.4 mm size that fit best and prints sideways too. If the arms are loose or tight on the shaft, print this test, press each socket onto the servo shaft, and pick the one that grips firmly, sits all the way down and doesn't crack. Print as exported, labels up, no supports.

## Folders

- `openscad/`: OpenSCAD source for the STLs
  - `gh_s37d.scad`: servo sizes (edit these) and the tab-slot positions
  - `HeadM5Dial.scad`: the fixed M5Dial head (doesn't use `gh_s37d.scad`)
  - `Torso.scad`, `torso_body.scad`: the boxy torso (Chest + lower body)
  - `DeskBox.scad`, `DeskBase.scad`, `Laptop.scad`, `desk_parts.scad`: the desk with the PCA9685 inside, and the laptop
  - `RobotScene.scad`: display scene of the whole robot (not printed)
  - `HeadSocketTest.scad`: servo shaft socket test (sizes the arms' socket)
  - `horn_mount.scad`: cross-horn recess used by the head
  - `ArmLeft.scad`, `ArmRight.scad`, `arm_onepiece.scad`: one-piece arms
  - `base/es9051/`: original ES9051 meshes (only `Chest` and `ForearmLeft` are used)
  - `build.sh`: writes all STLs to `parts/gh-s37d/stl/`
- `parts/gh-s37d/stl/`: the printable STLs
- `renders/`: pictures of the display scene (front, side, cutaway) and of `DeskBase`

## Rebuild after measuring your servo

1. Measure the tab span (tip to tip across both mounting tabs) with calipers.
2. In `openscad/gh_s37d.scad`, set `servo_mount_width = (span - 20) / 2`.
3. Run `openscad/build.sh` (needs `openscad` on your PATH).

## Caveats

- The tab span is set to 30.9 mm. The seller listing's 30.4 mm was too short for a real servo, so this was raised by 0.5 mm without measuring. Measure yours if it still doesn't fit.
- The screw pilot holes for the servo tabs are still at the ES9051 positions, about 0.4 mm inward from the GH-S37D tab holes. M2 self-tapping screws should still bite. If they don't line up, use the tiny screws that came with the servo or skip the tab screws.
- The wall left beyond some slot ends is about 0.6 mm (was 1.5 mm): one side of each slot in the torso's servo block. That is about one line wide on a 0.4 mm nozzle. Use the Arachne wall generator (the default in Bambu Studio) and check in the slicer preview that those walls are printed.
- The GH-S37D body is about 0.4 mm shorter than the ES9051's, so it sits on its tabs with a small gap below. That's fine.

## License

CERN-OHL-S-2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE). The source location is https://github.com/jamro/tiny-engineer/tree/main/3d_models. The Tiny Engineer name and logo are not licensed.
