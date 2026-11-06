// Right arm in one piece: mirror of the left arm. See arm_onepiece.scad.
include <arm_onepiece.scad>

mirror([0, 1, 0]) arm_left_print();
