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
// Assembled thickness, front face to back floor. The stand slot is sized from
// this and must NOT be rebuilt out of stack_t: the interior is bezel + stack_t
// + back_clear, so a slot cut to stack_t + 2 * wall comes out 4.5 mm too thin
// and the case will not go in at all. Measured 26.5 on the assembled render.
case_t = front_depth + wall + back_depth;

// The case necks down to the board body between the tabs. Sized to the full
// tab outline, the wall sat 5 mm out from the button faces and a finger had to
// reach 7.5 mm down a 5 mm hole; against the body it is the wall plus fit.
wide_end_l = 12;     // full-width section at each end (the tabs are 5 mm wide)
neck_w = wing_body_w + 2 * fit + 2 * wall;
neck_inset = (body_w - neck_w) / 2;   // how far each long side pulls in
neck_r = 2.0;        // fillet at the reentrant corners of the step

/* -------------------------------------------------------------- engraving */

// Recessed motifs, never raised: a recess prints clean on a vertical wall and
// on the build-plate face alike, and the earlier raised trials on the front
// needed support. 0.6 mm is two or three layers deep - enough to read, shallow
// enough that it never reaches the inside of a 2 mm wall.
engrave_d = 0.6;

// The back shell turns over about the short axis to assemble. That is a
// ROTATION, not a mirror, so a glyph laid out in these coordinates reads the
// same way round on the finished case and nothing needs flipping. What does
// move is which END is which: the USB opening is cut at the x0 end here, so the
// top of the case - the end that points up in the stand - is x = body_l.

// Both long walls carry the identical four-suit row. It sits LOW so the button
// side can repeat it unchanged underneath its three holes, whose bottoms are at
// z 15.5. Centred on body_l / 2 at a 15 mm pitch.
wall_icons   = ["cup", "sword", "wand", "pentacle"];
wall_icon_mm = 11;
wall_icon_x  = [21, 36, 51, 66];
wall_icon_z  = 7.5;

// Top end wall, away from USB. 12.5 mm at a 17 mm pitch leaves 4.5 mm between
// glyphs and stays off the corner_r rounding at each side. An earlier 14 mm at
// a 13.5 pitch actually OVERLAPPED.
end_icons   = ["moon", "sun", "star"];
end_icon_mm = 12.5;
end_icon_y  = [10, 27, 44];
end_icon_z  = 10;

// Stand rear, sized to the face the hull actually leaves - see stand().
stand_icons   = ["moon", "sun", "star"];
stand_icon_mm = 14;
stand_icon_x  = [13, 31, 49];

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

    // Engraved motifs. Local +z of engrave() is the face's outward normal, so
    // each rotation below just aims it: -y for the button wall, +y for the far
    // wall, +-x for an end wall.
    for (i = [0:3]) {
      translate([wall_icon_x[i], neck_inset, wall_icon_z])
        rotate([90, 0, 0]) engrave(wall_icons[i], wall_icon_mm);
      translate([wall_icon_x[i], body_w - neck_inset, wall_icon_z])
        rotate([0, 0, 180]) rotate([90, 0, 0]) engrave(wall_icons[i], wall_icon_mm);
    }
    // The end away from USB, which points up in the stand.
    for (i = [0:2])
      translate([usb_at_x0 ? 0 : body_l, end_icon_y[i], end_icon_z])
        rotate([0, 0, usb_at_x0 ? -90 : 90]) rotate([90, 0, 0])
          engrave(end_icons[i], end_icon_mm);
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
stand_slop = 1.0;        // slide clearance around the case. 0.6 printed too tight
// Internal corner radius of the slot. The case's cross-section here has SHARP
// corners: both shells are straight prisms, and corner_r rounds the case's
// VERTICAL edges, not the ones where a side wall meets the front face or the
// back floor. So a rounded slot corner cuts across the case's square one, and
// at 3.0 it blocked the case by 0.394 mm even though every flat had its 0.6 mm.
// The corner clears only while sqrt(2) * (r - stand_slop) <= r, i.e. r <= 3.41
// * stand_slop; 1.5 against a 1.0 slop clears by 0.23 mm.
stand_slot_r = 1.5;

// The USB-C end goes DOWN in the stand (user, 2026-09-19), which keeps the card
// upright at the firmware's setRotation(3). It also puts the port on the slot
// floor, so the unit comes out of the stand to charge.
// With that end down, this is how far the window's lower edge sits above the
// case's bottom. The front of the sleeve has to stop below it or it covers the
// bottom of the card - at the old fixed 8 mm it buried 2.5 mm of window.
window_from_usb_end = body_l - (wall + fit + window_offset_l + window_l);  // 5.5
stand_lip = window_from_usb_end - 1.5;

module stand() {
  slot_l = body_w + 2 * stand_slop;    // case width
  slot_t = case_t + 2 * stand_slop;    // case THICKNESS - see case_t
  sleeve_t = slot_t + 2 * stand_wall;
  base_l = slot_l + 2 * stand_wall;
  base_d = sleeve_t + 14;              // depth on the desk, for stability
  // The sleeve turns by MINUS stand_angle. Turning it the other way put the
  // top over on the same side as the cut-down wall, which tipped the display
  // 20 degrees face-down instead of leaning it back, and left the base's extra
  // depth on the side the case does not lean towards. This way the display
  // faces -y and leans back, and base_d runs behind it where the weight goes.
  // The lift sets the sleeve's rear bottom edge on the desk and the hull fills
  // the wedge under the raised front.
  lift = sleeve_t * sin(stand_angle);
  // The rear exterior: base back edge -> sleeve top edge.
  rear_y0 = base_d;              rear_z0 = stand_wall;
  rear_y1 = sleeve_t * cos(stand_angle) + stand_grip * sin(stand_angle);
  rear_z1 = stand_grip * cos(stand_angle);
  rear_len  = norm([rear_y1 - rear_y0, rear_z1 - rear_z0]);
  rear_lean = atan2(rear_y0 - rear_y1, rear_z1 - rear_z0);

  difference() {
    hull() {
      slab(base_l, base_d, stand_wall, 4);
      translate([0, 0, lift]) rotate([-stand_angle, 0, 0])
        slab(base_l, sleeve_t, stand_grip, 4);
    }
    translate([0, 0, lift]) rotate([-stand_angle, 0, 0]) {
      // Slot the case slides down into.
      translate([stand_wall, stand_wall, stand_wall])
        slab(slot_l, slot_t, stand_grip + 20, stand_slot_r);
      // Cut the front of the sleeve down so the lower display edge stays visible.
      translate([stand_wall + 10, -1, stand_wall + stand_lip])
        cube([slot_l - 20, stand_wall + 2, stand_grip]);
    }

    // Engraved motifs on the rear. The face they land on is NOT the sleeve's
    // back wall - the hull buries that - but the plane the hull leaves between
    // the base's back edge and the top of the sleeve. Confirmed against the
    // rendered STL: one planar strip, normal (0, 0.95, 0.32).
    for (i = [0:2])
      translate([stand_icon_x[i], (rear_y0 + rear_y1) / 2, (rear_z0 + rear_z1) / 2])
        rotate([0, 0, 180]) rotate([90 - rear_lean, 0, 0])
          engrave(stand_icons[i], stand_icon_mm);
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
plunger_clear = 0.2;         // all round, inside the hole. Printed and confirmed
                             // good: the plunger slides freely with the front off.
// Clearance at the SEAM only, between the plunger's top faces and the front
// shell's rim. Separate from plunger_clear because 0.2 printed as a pinch: with
// the front on, its rim clamped the plunger and no button would move, while the
// same plunger slid freely with the front off. The model was right - the faces
// were 0.2 apart - but 0.2 is not a printable sliding gap across a joint between
// two separately printed parts, where elephant's foot on the rim or one proud
// top layer on the flange closes it. Only the plunger shrinks, so this reprints
// three small parts and leaves the front and back alone.
plunger_seam_clear = 0.6;
plunger_flange_w = 7.0;      // across the wall; clears the clip pockets by 0.9
plunger_flange_back = 3.5;   // how far the flange reaches away from the joint
plunger_flange_t = 1.0;
plunger_pocket_t = 1.0;      // counterbore depth in the wall's inner face
plunger_proud = 0.5;         // how far it stands out of the case at rest

seam_offset = button_from_face - (front_depth + wall);  // hole is clipped here
plunger_flange_seam = seam_offset - plunger_seam_clear;
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

/* ----------------------------------------------------------- glyph library */

// Each glyph is drawn in a 100 x 100 box centred on the origin, so scaling by
// size / 100 puts a `size` mm motif on the origin. Kept as primitives rather
// than imported SVG so the file stays self-contained and parametric.

module g_sun() {
  circle(d = 36);
  for (i = [0:7]) rotate(i * 45)
    hull() { translate([26, 0]) circle(d = 7); translate([45, 0]) circle(d = 7); }
}

module g_moon() {
  difference() { circle(r = 36); translate([15, 7]) circle(r = 30); }
}

module g_star() {
  polygon([for (i = [0:15]) let(r = i % 2 == 0 ? 46 : 20, a = 90 + i * 22.5)
            [r * cos(a), r * sin(a)]]);
}

module g_pentacle() {
  pts = [for (i = [0:4]) let(a = 90 + i * 144) [28.25 * cos(a), 28.25 * sin(a)]];
  difference() { circle(r = 47); circle(r = 39.5); }
  for (i = [0:4]) hull() {
    translate(pts[i]) circle(d = 7.5);
    translate(pts[(i + 1) % 5]) circle(d = 7.5);
  }
}

module g_cup() {
  intersection() {
    translate([0, 24]) scale([24, 26]) circle(r = 1, $fn = 96);
    translate([0, -6]) square([60, 60], center = true);
  }
  translate([0, -9]) square([9, 18], center = true);
  hull() for (t = [-1, 1]) translate([t * 14.25, -21.5]) circle(d = 7.5);
}

module g_sword() {
  polygon([[0, 42], [6, 26], [6, -4], [-6, -4], [-6, 26]]);
  hull() for (t = [-1, 1]) translate([t * 20, -7.75]) circle(d = 8);
  translate([0, -21.5]) square([9, 20], center = true);
  translate([0, -37]) circle(d = 14);
}

// The sparked wand, chosen 2026-09-19: a staff and a four-point spark. The
// budding staff of the Rider-Waite pip read as a plant, so it was dropped.
module g_wand() {
  hull() for (t = [-1, 1]) translate([0, t * 19 - 14]) circle(d = 10);
  polygon([for (i = [0:7]) let(r = i % 2 == 0 ? 17 : 6, a = 90 + i * 45)
            [r * cos(a), 26 + r * sin(a)]]);
}

module glyph(name) {
  if (name == "sun") g_sun();
  else if (name == "moon") g_moon();
  else if (name == "star") g_star();
  else if (name == "pentacle") g_pentacle();
  else if (name == "cup") g_cup();
  else if (name == "sword") g_sword();
  else if (name == "wand") g_wand();
  else assert(false, str("unknown glyph: ", name));
}

// A glyph as a solid to subtract. Local +z is the OUTWARD normal of the face it
// lands on, so the solid runs from the surface down to engrave_d, and stands
// 0.01 mm proud of it so the cut never leaves a coincident face.
module engrave(name, size) {
  translate([0, 0, -engrave_d])
    linear_extrude(engrave_d + 0.01) scale(size / 100) glyph(name);
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
