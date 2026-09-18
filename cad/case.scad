// Tarot pad enclosure: handheld case (snap-fit front + back) that slides into a
// separate desk stand.
//
// Render one part at a time with `part`, or leave it "all" for a layout preview.
//   openscad -o stl/front.stl -D 'part="front"' case.scad
//
// Plain shells, no relief on the front: raised ornament needs too much support
// to print. Getting the fit right comes first.
//
// MEASURED VALUES marked TODO are estimates and need calipers on the real
// hardware before printing. Everything else comes from Adafruit's published
// dimensions or this project's brief.

part = "all";  // [all, front, back, stand]

/* ---------------------------------------------------------------- hardware */

// Wing PCB outline INCLUDING the mounting tabs, measured 2026-09-17. The
// brief's 79.5 x 38 mm is the body without tabs, and the tabs fouled the first
// printed front, so the cavity is sized to the full outline.
wing_l = 80;
wing_w = 47;

// Feather sits on the back of the Wing (brief: 52.3 x 22.7 x 7.2 mm).
feather_l = 52.3;
feather_w = 22.7;

// Measured 2026-09-17: display glass to the highest point on the back, over
// the Feather with headers and JST plug. Sets the back depth and stand slot.
stack_t = 20;

// Visible display area. Length and the offset from the ribbon-end edge were
// measured 2026-09-17; the width is derived from the 296 x 128 pixel aspect
// (70 * 128 / 296) and the area is assumed centred across the board.
window_l = 70;
window_w = 30.3;
window_offset_l = 8.0;   // measured, from the PCB edge at the ribbon end
window_offset_w = 0;     // + moves the window away from the button edge

// Buttons A/B/C along one long edge, measured 2026-09-17: A is 20 mm from the
// end of the board, B and C each 20 mm further along.
button_positions = [20, 40, 60];
// TODO confirm: button centres below the display glass. The switches are
// side-actuated on the Wing's component side, so this is the Wing's 6.8 mm
// thickness plus about half a 3.5 mm switch body - estimated from photos.
button_from_face = 8.5;
button_hole_d = 5.0;     // clearance for a fingertip or a printed plunger

// USB-C opening on the Feather's short edge.
usb_w = 9.5;             // connector 8.9 wide plus clearance
usb_h = 4.0;             // connector 3.3 high plus clearance
// Measured 2026-09-17: 18 mm from the outer edge of the screw tab to the start
// of the connector, across the board width.
usb_from_tab_edge = 18;
usb_from_back = 1.0;     // measured, centre above the back of the Wing PCB

// Battery: 36 x 17 x 7.8 mm (Adafruit 3898) on its extension cable, so it sits
// loose inside. The printed pocket walls fouled the boards and were removed.

/* ------------------------------------------------------------------- case */

wall = 2.0;          // shell wall thickness
bezel = 2.5;         // front lip over the display edge
fit = 1.0;           // clearance around the PCB (0.35 printed too tight)
front_depth = 4.5;   // inside depth of the front shell
corner_r = 3.5;      // outside corner radius
$fn = 48;

// Mounting tabs: posts in the front shell poke through the PCB's corner holes
// to locate the boards. The hole pattern is from the brief and matches the
// measured 80 x 47 outline with the centres ~2.5-2.9 mm in from each edge.
tab_pitch_l = 74.3;
tab_pitch_w = 42;
post_d = 2.3;        // clearance in the 2.5 mm holes
post_above_lip = 2.0;  // how far a post stands above the PCB's underside

// Snap-fit: cantilever hooks on the back shell catch slots in the front walls.
clip_w = 10;
clip_t = 1.6;
clip_hook = 0.9;     // how far the hook sticks out
clip_drop = 2.2;     // hook centre below the front shell's rim

inner_l = wing_l + 2 * fit;
inner_w = wing_w + 2 * fit;
// The display face rests on the bezel lip, so the boards need bezel + stack_t
// of interior depth. The first print was ~0.5 mm short and sat on the Feather,
// which made the unit rock; back_clear adds room so nothing touches the floor.
back_clear = 2.0;
back_depth = bezel + stack_t + back_clear - front_depth + wall;
body_l = inner_l + 2 * wall;
body_w = inner_w + 2 * wall;

// Rounded slab, corners rounded on the l/w plane only.
module slab(l, w, h, r) {
  hull() for (x = [r, l - r], y = [r, w - r]) translate([x, y, 0]) cylinder(r = r, h = h);
}

// Hollow shell, open at the top: outer slab minus an inner cavity.
module shell(h, depth) {
  difference() {
    slab(body_l, body_w, h, corner_r);
    translate([wall, wall, h - depth])
      slab(body_l - 2 * wall, body_w - 2 * wall, depth + 1, max(0.1, corner_r - wall));
  }
}

module clip_positions() {
  for (x = [body_l * 0.25, body_l * 0.75], y = [0, 1])
    translate([x, y ? body_w - wall : wall, 0]) mirror([0, y, 0]) children();
}

module front() {
  difference() {
    shell(front_depth + wall, front_depth);

    // Display window through the front face, which is the z = 0 plane.
    translate([wall + fit + window_offset_l,
               wall + fit + (wing_w - window_w) / 2 + window_offset_w, -1])
      slab(window_l, window_w, wall + 2, 1.5);

    // Slots the back shell's hooks latch into.
    clip_positions()
      translate([-clip_w / 2, -0.1, front_depth + wall - clip_drop - clip_t])
        cube([clip_w, wall + 0.2, clip_t + 0.4]);
  }

  // Bezel lip: a ledge the display edge rests against, so the PCB cannot move
  // toward the window.
  difference() {
    translate([wall, wall, wall]) slab(inner_l, inner_w, bezel, 1);
    translate([wall + bezel, wall + bezel, wall - 1])
      slab(inner_l - 2 * bezel, inner_w - 2 * bezel, bezel + 2, 1);
  }

  // Locating posts through the tab holes. The PCB rests on the bezel lip, so a
  // post runs from the cavity floor to just above the tabs.
  for (x = [(body_l - tab_pitch_l) / 2, (body_l + tab_pitch_l) / 2],
       y = [(body_w - tab_pitch_w) / 2, (body_w + tab_pitch_w) / 2])
    translate([x, y, wall]) cylinder(d = post_d, h = bezel + post_above_lip);
}

module back() {
  difference() {
    shell(back_depth, back_depth - wall);

    // USB-C opening in the short wall.
    translate([-1, wall + fit + usb_from_tab_edge, wall + usb_from_back - usb_h / 2])
      cube([wall + 2, usb_w, usb_h]);

    // Button openings through the long wall. The shells meet rim to rim, so a
    // depth measured from the display face maps to the back shell like this.
    for (p = button_positions)
      translate([wall + fit + p, -1, back_depth - (button_from_face - front_depth - wall)])
        rotate([-90, 0, 0]) cylinder(d = button_hole_d, h = wall + 2);
  }

  // Snap hooks.
  clip_positions()
    translate([-clip_w / 2, 0, back_depth - clip_t - 0.2]) {
      cube([clip_w, clip_t, clip_drop + clip_t]);
      translate([0, -clip_hook, clip_drop]) cube([clip_w, clip_hook + clip_t, clip_t]);
    }
}

/* ------------------------------------------------------------------ stand */

stand_angle = 20;        // lean back from vertical
stand_grip = 26;         // how far up the case the sleeve holds it
stand_wall = 3;
stand_slop = 0.6;        // slide clearance around the case

module stand() {
  slot_l = body_w + 2 * stand_slop;              // case width
  slot_t = stack_t + 2 * wall + 2 * stand_slop;  // case thickness
  base_l = slot_l + 2 * stand_wall;
  base_d = slot_t + 2 * stand_wall + 14;         // depth on the desk, for stability

  difference() {
    hull() {
      slab(base_l, base_d, stand_wall, 4);
      rotate([stand_angle, 0, 0]) slab(base_l, slot_t + 2 * stand_wall, stand_grip, 4);
    }
    // Slot the case slides down into.
    rotate([stand_angle, 0, 0])
      translate([stand_wall, stand_wall, stand_wall])
        slab(slot_l, slot_t, stand_grip + 20, 3);
    // Cut the front of the sleeve down so the lower display edge stays visible.
    rotate([stand_angle, 0, 0])
      translate([stand_wall + 10, -1, stand_wall + 8])
        cube([slot_l - 20, stand_wall + 2, stand_grip]);
  }
}

/* ----------------------------------------------------------------- render */

if (part == "front") front();
else if (part == "back") back();
else if (part == "stand") stand();
else {
  front();
  translate([0, body_w + 10, 0]) back();
  translate([0, -(stack_t + 2 * wall + 2 * stand_wall + 26), 0]) stand();
}
