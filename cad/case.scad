// Tarot pad enclosure: handheld case (snap-fit front + back) that slides into a
// separate desk stand.
//
// Render one part at a time with `part`, or leave it "all" for a layout preview.
//   openscad -o stl/front.stl -D 'part="front"' case.scad
//
// Plain shells, no relief on the front: raised ornament needs too much support
// to print. Getting the fit right comes first.
//
// Both shells are modelled opening upward, so the back is turned over about the
// SHORT axis to assemble. That mirrors x, which the buttons and the clips
// survive because both sets are symmetric, but the USB-C opening does not - it
// is cut at the opposite end from where it ends up. Turning it over the other
// way would put the button holes on the wrong long edge.
//
// MEASURED VALUES marked TODO are estimates and need calipers on the real
// hardware before printing. Everything else comes from Adafruit's published
// dimensions or this project's brief.

part = "all";  // [all, front, back, stand, plungers]

/* ---------------------------------------------------------------- hardware */

// Wing PCB outline INCLUDING the mounting tabs, measured 2026-09-17. The
// brief's 79.5 x 38 mm is the body without tabs, and the tabs fouled the first
// printed front, so the cavity is sized to the full outline.
wing_l = 80;
wing_w = 47;
// Board width without the tabs, measured 2026-09-18. The tabs stand 5 mm proud
// of it on each side, and the button faces are flush with the body edge, so
// this is what the shell has to reach to.
wing_body_w = 37;

// Feather sits on the back of the Wing (brief: 52.3 x 22.7 x 7.2 mm).
feather_l = 52.3;
feather_w = 22.7;

// Display glass to the highest point on the back, which is the battery
// connector beside USB-C. Sets the back depth and the stand slot. Measured 20
// on 2026-09-17 and 18 on 2026-09-18; the user chose 18.
stack_t = 18;

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
// Measured 2026-09-18: the switch centres sit 4 mm below the display glass.
// The switches are side-actuated on the Wing's component side, so this covers
// the display panel, the PCB, and half the switch body.
button_from_glass = 4.0;
button_hole_d = 5.0;     // clearance for a fingertip or a printed plunger

// USB-C opening on the Feather's short edge.
usb_w = 9.5;             // connector 8.9 wide plus clearance
usb_h = 4.0;             // connector 3.3 high plus clearance
// From the outer edge of the screw tab to the start of the connector, across
// the board width, on the button side. Measured 18 on 2026-09-17. The 17.5
// fitted on 2026-09-18 moved it the wrong way: the note it came from read the
// error backwards, and lowering this walks the opening TOWARDS the button side
// because the cut starts at wall + fit + usb_from_tab_edge and the buttons are
// at low y. Measured off the 2026-09-19 print photo, scaled by the known 9.5 mm
// opening: 1.54 mm of the connector hid behind the wall on the high-y side and
// 2.15 mm of the opening stood empty on the button side, so the opening wants
// 1.84 mm further from the buttons. Note the clearance is all on one side -
// usb_w is 0.6 wider than the connector and the cut starts at the connector's
// start - so this centres it at 19.34 rather than leaving it flush.
usb_from_tab_edge = 19.3;
usb_from_back = 1.0;     // measured, centre above the back of the Wing PCB
wing_pcb_t = 1.6;        // standard board thickness, not measured
// The glass covers the board body but not the tabs, so the tab surface sits a
// glass thickness behind the face that rests on the bezel lip, and a post has
// to cross that step before it enters a tab hole.
// TODO confirm: glass/panel thickness, estimated.
glass_t = 1.2;
post_above_tab = 1.2;  // how far a post stands into the 1.6 mm tab hole
board_t = glass_t + wing_pcb_t;   // glass face to the back of the Wing PCB
// Which end of the board USB-C is on. Confirmed 2026-09-18: NOT the ribbon end,
// so it sits at the far end from the display's 8 mm margin (window_offset_l).
// The two Feather guide holes were measured from the USB-C end, so their posts
// follow this too.
usb_at_x0 = false;

// Battery: 36 x 17 x 7.8 mm (Adafruit 3898) on its extension cable, so it sits
// loose inside. The printed pocket walls fouled the boards and were removed.
// It needs the full interior depth, so the raised floor that holds the stack
// forward runs only platform_l from the USB-C end.
platform_l = 50;

/* ------------------------------------------------------------------- case */

wall = 2.0;          // shell wall thickness
bezel = 2.5;         // front lip over the display edge
fit = 1.5;           // clearance around the PCB. 0.35 and then 1.0 both printed
                     // too tight: at 1.0 the board barely went in and the hook
                     // arms, which stand clip_t - clip_pocket proud of the wall,
                     // left it only 0.2 mm to slide past.
front_depth = 4.5;   // inside depth of the front shell
corner_r = 3.5;      // outside corner radius
$fn = 48;

// The glass rests on the bezel lip, so the button depth from the outside of
// the case is the measured depth below the glass plus the face plate and lip.
button_from_face = button_from_glass + wall + bezel;

// Glass face to the centre of the USB-C connector, measured 2026-09-18, the
// same convention as button_from_glass. Two earlier attempts were wrong:
// taking usb_from_back from the back shell's floor put it 17 mm too deep, and
// deriving it from the back of the Wing PCB put it up at the joint. The
// connector is on the Feather, deep in the stack, beside the battery connector
// the platform bears on at stack_t = 18.
// Measured 15.0 on 2026-09-18, but the printed opening sat 1 mm too low, so
// this is the fitted value. The 1 mm is unexplained - the buttons and the tab
// supports, which reference the same glass plane, both fit - so it is most
// likely in the measurement or in where the glass actually seats on the lip.
usb_from_glass = 14.0;
usb_from_face = usb_from_glass + wall + bezel;

// Mounting tabs: posts in the front shell poke through the PCB's corner holes
// to locate the boards. The hole pattern is from the brief and matches the
// measured 80 x 47 outline with the centres ~2.5-2.9 mm in from each edge.
tab_pitch_l = 74.3;
tab_pitch_w = 42;
post_d = 2.3;        // clearance in the 2.5 mm holes
tab_post_d = 6.0;    // support pillars under the far tabs, in the back shell

// The two Feather guide holes beside USB-C, measured 2026-09-18: centres 2 mm
// in from the USB-C end edge and 15 mm in from a 47 mm outer (tab-tip) edge,
// the pair symmetric about the centreline. UNUSED: the display glass covers the
// whole board body and stops only at the tabs, so these holes sit under the
// glass and no post can reach them from the front. Kept because the numbers
// were measured, and because posts for them could rise from the back shell.
guide_from_end = 2;
guide_from_side = 15;

// Snap-fit: hooks on the back shell catch slots in the front walls. clip_rise
// fixes both ends of the joint - the hook stands that far above the back's rim
// and the slot is cut the same distance below the front's rim - so the two
// cannot drift apart, which is how they came to miss each other by 1.4 mm.
// clip_rise - clip_t - clip_slack / 2 is the wall left above the slot, and that
// is what actually holds the halves together.
clip_w = 10;         // hook width
clip_t = 1.6;        // arm and lip thickness
clip_hook = 0.6;     // how far the lip stands proud of the arm
clip_rise = 3.3;     // hook height above the back rim, leaving 1.5 mm of catch
clip_root = 2.0;     // how far the arm is rooted below that rim
clip_slack = 0.4;    // slot clearance around the lip
// The arm is 1.6 mm thick but only `fit` clears the PCB edge, and it has to
// pass the bezel lip to reach the slot, so it is recessed into the wall and the
// pocket notches the lip on its way through.
clip_pocket = 0.8;   // wall recess, leaving the arm 0.2 mm clear of the PCB
clip_float = 0.2;    // extra pocket depth so the arm does not rub the skin

inner_l = wing_l + 2 * fit;
inner_w = wing_w + 2 * fit;
// The display face rests on the bezel lip, so the boards need bezel + stack_t
// of interior depth. The first print was ~0.5 mm short and sat on the Feather,
// which made the unit rock; back_clear adds room so nothing touches the floor.
back_clear = 2.0;
back_depth = bezel + stack_t + back_clear - front_depth + wall;
body_l = inner_l + 2 * wall;
body_w = inner_w + 2 * wall;

// The case necks down to the board body between the tabs. Sized to the full
// tab outline, the wall sat 5 mm out from the button faces and a finger had to
// reach 7.5 mm down a 5 mm hole; against the body it is the wall plus fit.
wide_end_l = 12;     // full-width section at each end (the tabs are 5 mm wide)
neck_w = wing_body_w + 2 * fit + 2 * wall;
neck_inset = (body_w - neck_w) / 2;   // how far each long side pulls in
neck_r = 2.0;        // fillet at the reentrant corners of the step

// Rounded rectangle, and the slab that extrudes it.
module rrect(l, w, r) {
  hull() for (x = [r, l - r], y = [r, w - r]) translate([x, y]) circle(r = r);
}

module slab(l, w, h, r) {
  linear_extrude(h) rrect(l, w, r);
}

// The case outline: full width over the tabs at each end, necked down to the
// board body in between. `inset` shrinks it for the cavity and the bezel lip,
// which also pulls the corner and fillet radii in by the same amount.
module case_profile(inset = 0) {
  offset(r = -inset)
    offset(r = -neck_r) offset(r = neck_r)   // fillet the reentrant corners
      union() {
        translate([0, neck_inset]) rrect(body_l, neck_w, corner_r);
        rrect(wide_end_l, body_w, corner_r);
        translate([body_l - wide_end_l, 0]) rrect(wide_end_l, body_w, corner_r);
      }
}

// Hollow shell, open at the top: outer profile minus an inner cavity.
module shell(h, depth) {
  difference() {
    linear_extrude(h) case_profile();
    translate([0, 0, h - depth]) linear_extrude(depth + 1) case_profile(wall);
  }
}

// Snap clips. The button-side pair has to clear the button openings - at the
// quarter points the hooks stood 1.6 mm behind the A and C holes - so it sits
// in the gaps between them. The far side keeps the quarter points.
// Both pairs MUST be symmetric about body_l / 2: the back is turned over about
// the short axis, so x mirrors, and a pair that is not symmetric lands on the
// wrong slots. Written as literals they survived only while they happened to
// add up to body_l, and widening the case by 1 mm broke them.
clip_x_spread = 10;  // from the centre, into the gaps between the buttons
clip_x_button = [body_l / 2 - clip_x_spread, body_l / 2 + clip_x_spread];
clip_x_far = [body_l * 0.25, body_l * 0.75];

module clip_positions() {
  for (x = clip_x_button)
    translate([x, neck_inset + wall, 0]) children();
  for (x = clip_x_far)
    translate([x, body_w - neck_inset - wall, 0]) mirror([0, 1, 0]) children();
}

module front() {
  difference() {
    union() {
      shell(front_depth + wall, front_depth);

      // Bezel lip: a ledge the display edge rests against, so the PCB cannot
      // move toward the window. Inside the difference so the clip pockets cut
      // it too - the arms have to pass through it to reach the slots.
      translate([0, 0, wall]) linear_extrude(bezel)
        difference() {
          case_profile(wall);
          case_profile(wall + bezel);
        }
    }

    // Display window through the front face, which is the z = 0 plane.
    translate([wall + fit + window_offset_l,
               wall + fit + (wing_w - window_w) / 2 + window_offset_w, -1])
      slab(window_l, window_w, wall + 2, 1.5);

    // Pockets that take the hook arms: they recess the arm into the wall so it
    // clears the PCB, and cut the bezel lip where the arm passes it.
    clip_positions()
      translate([-clip_w / 2 - 0.4, -clip_pocket - clip_float,
                 front_depth + wall - clip_rise - clip_slack])
        cube([clip_w + 0.8, clip_pocket + clip_float + clip_t + clip_slack,
              clip_rise + clip_slack + 1]);

    // Slots the lips latch into, through the skin left by the pockets.
    clip_positions()
      translate([-clip_w / 2, -wall - 0.1,
                 front_depth + wall - clip_rise - clip_slack / 2])
        cube([clip_w, wall + 0.2, clip_t + clip_slack]);
  }

  // Locating posts through the tab holes - the only board holes the front can
  // reach. The glass rests on the bezel lip, so a post runs from the cavity
  // floor, past the glass thickness, and into the tab hole behind it.
  for (x = [(body_l - tab_pitch_l) / 2, (body_l + tab_pitch_l) / 2],
       y = [(body_w - tab_pitch_w) / 2, (body_w + tab_pitch_w) / 2])
    translate([x, y, wall])
      cylinder(d = post_d, h = bezel + glass_t + post_above_tab);
}

module back() {
  difference() {
    shell(back_depth, back_depth - wall);

    // USB-C opening in the short wall, at the mirrored end (see the header).
    translate([usb_at_x0 ? body_l - wall - 1 : -1,
               wall + fit + usb_from_tab_edge,
               back_depth - (usb_from_face - front_depth - wall) - usb_h / 2])
      cube([wall + 2, usb_w, usb_h]);

    // Counterbores for the plunger flanges, in the wall's inner face.
    for (p = button_positions)
      translate([wall + fit + p - plunger_flange_w / 2 - plunger_clear,
                 neck_inset + wall - plunger_pocket_t,
                 back_depth - seam_offset - plunger_flange_back - plunger_clear])
        cube([plunger_flange_w + 2 * plunger_clear, plunger_pocket_t + 1,
              plunger_flange_back + plunger_clear + seam_offset]);

    // Button openings through the long wall. The shells meet rim to rim, so a
    // depth measured from the display face maps to the back shell like this.
    for (p = button_positions)
      translate([wall + fit + p, neck_inset - 1, back_depth - (button_from_face - front_depth - wall)])
        rotate([-90, 0, 0]) cylinder(d = button_hole_d, h = wall + 2);
  }

  // Raised floor over the first platform_l from the USB-C end. It fills
  // back_clear so the deepest part of the stack - the battery connector beside
  // USB-C - bears on it and the board cannot lift off the front's bezel lip.
  // The rest of the length stays at full depth for the battery.
  intersection() {
    translate([0, 0, wall]) linear_extrude(back_clear) case_profile(wall);
    translate([usb_at_x0 ? body_l - wall - platform_l : wall, -1, -1])
      cube([platform_l, body_w + 2, back_depth + 2]);
  }

  // Posts under the tabs at the end away from USB-C. The platform holds the
  // USB end; without these the board tips off it. They meet the back of the
  // tabs, so their height leans on board_t and therefore on the estimated
  // glass_t.
  tab_post_top = front_depth + wall + back_depth - (wall + bezel + board_t);
  for (y = [(body_w - tab_pitch_w) / 2, (body_w + tab_pitch_w) / 2])
    translate([usb_at_x0 ? (body_l - tab_pitch_l) / 2 : (body_l + tab_pitch_l) / 2,
               y, wall])
      cylinder(d = tab_post_d, h = tab_post_top - wall);

  // Snap hooks: an arm sunk into the wall, with a lip at the tip that tapers to
  // flush so the halves cam together instead of having to be forced.
  clip_positions()
    translate([-clip_w / 2, -clip_pocket, back_depth - clip_root]) {
      cube([clip_w, clip_t, clip_root + clip_rise]);
      hull() {
        translate([0, -clip_hook, clip_root + clip_rise - clip_t])
          cube([clip_w, clip_hook + clip_t, 0.01]);
        translate([0, 0, clip_root + clip_rise - 0.01])
          cube([clip_w, clip_t, 0.01]);
      }
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

/* --------------------------------------------------------------- plungers */

// The switches cannot be reached with a finger: wall + fit leaves them 3.5 mm
// down a 5 mm hole. A plunger drops into each hole from inside before the board
// goes in, and the flange cannot pass back out through the hole.
//
// The flange sits in a counterbore INSIDE the wall, not in the gap in front of
// the switch. Sitting in the gap, a 1.0 mm flange held every switch down - the
// actuators stand proud of the board edge, so that gap is smaller than fit.
// Inside the wall the rest position no longer depends on that unknown at all.
//
// The flange is a plate, not a disc: the hole is clipped where it crosses the
// shell joint, so a disc big enough to catch would reach past the joint, where
// there is no wall to catch it. It reaches far on the side away from the joint
// and stops short of it on the other.
plunger_clear = 0.2;         // all round, inside the hole
plunger_flange_w = 7.0;      // across the wall; clears the clip pockets by 0.9
plunger_flange_back = 3.5;   // how far the flange reaches away from the joint
plunger_flange_t = 1.0;
plunger_pocket_t = 1.0;      // counterbore depth in the wall's inner face
plunger_proud = 0.5;         // how far it stands out of the case at rest

seam_offset = button_from_face - (front_depth + wall);  // hole is clipped here
plunger_flange_seam = seam_offset - plunger_clear;
plunger_shaft_l = wall - plunger_pocket_t + plunger_proud;

module plunger() {
  translate([-plunger_flange_back, -plunger_flange_w / 2, 0])
    cube([plunger_flange_back + plunger_flange_seam, plunger_flange_w,
          plunger_flange_t]);
  translate([0, 0, plunger_flange_t])
    intersection() {
      cylinder(d = button_hole_d - 2 * plunger_clear, h = plunger_shaft_l);
      translate([-button_hole_d, -button_hole_d, 0])
        cube([button_hole_d + plunger_flange_seam, 2 * button_hole_d,
              plunger_shaft_l]);
    }
}

module plungers() {
  for (i = [0 : 2]) translate([i * (plunger_flange_w + 3), 0, 0]) plunger();
}

/* ----------------------------------------------------------------- render */

if (part == "front") front();
else if (part == "back") back();
else if (part == "stand") stand();
else if (part == "plungers") plungers();
else {
  front();
  translate([0, body_w + 10, 0]) back();
  translate([0, -(stack_t + 2 * wall + 2 * stand_wall + 26), 0]) stand();
}
