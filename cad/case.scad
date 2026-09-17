// Tarot pad enclosure: handheld case (snap-fit front + back) that slides into a
// separate desk stand.
//
// Render one part at a time with `part`, or leave it "all" for a layout preview.
//
// MEASURED VALUES marked TODO are estimates and need calipers on the real
// hardware before printing. Everything else comes from Adafruit's published
// dimensions or this project's brief.

part = "all";  // [all, front, back, stand]

/* ---------------------------------------------------------------- hardware */

// Wing PCB, excluding the mounting tabs (brief: 79.5 x 38 x 6.8 mm).
wing_l = 79.5;
wing_w = 38;
wing_t = 6.8;      // PCB plus the display module on the front face

// Feather sits on the back of the Wing (brief: 52.3 x 22.7 x 7.2 mm).
feather_l = 52.3;
feather_w = 22.7;

// TODO measure: display face to the back of the Feather, headers seated.
// Estimated from Wing 6.8 + header stack ~8 + Feather PCB and parts ~3.
stack_t = 18;

// TODO measure: e-ink active area and where it sits on the PCB. 67.6 x 29.2 mm
// is computed from 296 x 128 px on a 2.9" diagonal; the offsets assume the
// active area is centred across the width and shifted away from the ribbon.
window_l = 67.6;
window_w = 29.2;
window_offset_l = 4.0;   // from the PCB edge opposite the ribbon tab
window_offset_w = 0;     // + moves the window away from the button edge

// TODO measure: buttons A/B/C along one long edge, centres from the same datum.
button_positions = [26, 40, 54];
button_from_edge = 4.5;  // centre distance in from the long edge
button_hole_d = 5.0;     // clearance for a fingertip or a printed plunger

// TODO measure: USB-C opening on the Feather's short edge.
usb_w = 9.5;             // connector 8.9 wide plus clearance
usb_h = 4.0;             // connector 3.3 high plus clearance
usb_centre_w = 19;       // across the case width
usb_from_back = 3.0;     // centre above the inside of the back wall

// Battery: 36 x 17 x 7.8 mm (Adafruit 3898) on its extension cable, so it can
// sit anywhere inside. Pocket is in the back shell beside the Feather.
batt_l = 36;
batt_w = 17;
batt_t = 7.8;

/* ------------------------------------------------------------------- case */

wall = 2.0;          // shell wall thickness
bezel = 2.5;         // front lip over the display edge
fit = 0.35;          // clearance around the PCB
front_depth = 4.5;   // inside depth of the front shell
corner_r = 3.5;      // outside corner radius
$fn = 48;

// Snap-fit: cantilever hooks on the back shell catch windows in the front.
clip_w = 10;
clip_t = 1.6;
clip_hook = 0.9;     // how far the hook sticks out
clip_drop = 2.2;     // hook centre below the front shell's rim

inner_l = wing_l + 2 * fit;
inner_w = wing_w + 2 * fit;
back_depth = stack_t - front_depth + wall;
outer_l = inner_l + 2 * wall;
outer_w = inner_w + 2 * wall;

// Rounded slab, corners rounded on the l/w plane only.
module slab(l, w, h, r) {
  hull() for (x = [r, l - r], y = [r, w - r]) translate([x, y, 0]) cylinder(r = r, h = h);
}

// Hollow shell: outer slab minus an inner cavity, open at the top.
module shell(l, w, h, depth) {
  difference() {
    slab(l, w, h, corner_r);
    translate([wall, wall, h - depth]) slab(l - 2 * wall, w - 2 * wall, depth + 1, max(0.1, corner_r - wall));
  }
}

module clip_positions() {
  for (x = [outer_l * 0.25, outer_l * 0.75], y = [0, 1])
    translate([x, y ? outer_w - wall : wall, 0]) mirror([0, y, 0]) children();
}

/* --------------------------------------------------------------- ornament */

// Metallic PLA cannot be polished, so the relief has to carry the look: icons
// stand well clear of the hammered texture. Keep both off the bezel lip and the
// snap surfaces, where any relief would change the fit.
icon_relief = 1.3;       // raised height of sun/moon/stars
texture_depth = 0.3;     // hammered dimples, shallow by comparison
texture = true;          // false renders much faster while iterating
dimple_d = 3.2;          // planishing marks, roughly 2-4 mm on real work
dimple_pitch = 3.6;
dimple_jitter = 0.9;

sun_d = 13;              // rayed disc at the top end
moon_d = 9;              // crescent at the bottom end
star_d = 5.5;            // corner stars
star_inset = 5.0;        // corner star centre, in from each outside edge

// RWS sun: a disc with alternating straight and wavy rays. The wavy ones are
// approximated by narrower, shorter triangles between the straight ones.
module sun_2d(d, rays = 8) {
  r = d / 2;
  circle(r = r * 0.52);
  for (i = [0 : rays - 1]) {
    rotate(i * 360 / rays) polygon([[-r * 0.16, 0], [r * 0.16, 0], [0, r]]);
    rotate(i * 360 / rays + 180 / rays) polygon([[-r * 0.09, 0], [r * 0.09, 0], [0, r * 0.72]]);
  }
}

module moon_2d(d) {
  difference() {
    circle(d = d);
    translate([d * 0.34, 0]) circle(d = d * 0.95);
  }
}

// Eight-pointed star, as on the RWS Star card.
module star_2d(d, points = 8, inner = 0.38) {
  r = d / 2;
  polygon([for (i = [0 : 2 * points - 1])
             let(a = i * 180 / points, rr = (i % 2 == 0) ? r : r * inner)
               [rr * cos(a), rr * sin(a)]]);
}

// Icons raised from the front face, laid out on the frame around the window.
// The front face is the z = 0 plane and the shell rises from it, so relief
// stands proud in -z.
module front_icons() {
  window_x = wall + fit + window_offset_l;
  translate([0, 0, -icon_relief]) linear_extrude(icon_relief + 0.01) {
    // Sun at the ribbon-tab end, which is the top of the card on screen.
    translate([outer_l - (outer_l - window_x - window_l) / 2, outer_w / 2]) sun_2d(sun_d);
    translate([window_x / 2, outer_w / 2]) moon_2d(moon_d);
    for (x = [star_inset, outer_l - star_inset], y = [star_inset, outer_w - star_inset])
      translate([x, y]) star_2d(star_d);
  }
}

// Hammered dimples over the front face, skipping the window and the icons.
module front_texture() {
  // A dimple of width dimple_d and depth texture_depth is the cap of a sphere
  // of this radius; the centre sits outside the part so only the cap bites in.
  r = (dimple_d * dimple_d / (8 * texture_depth)) + texture_depth / 2;
  nx = floor(outer_l / dimple_pitch);
  ny = floor(outer_w / dimple_pitch);
  for (i = [1 : nx - 1], j = [1 : ny - 1]) {
    jx = rands(-dimple_jitter, dimple_jitter, 1, i * 977 + j)[0];
    jy = rands(-dimple_jitter, dimple_jitter, 1, i * 131 + j * 17)[0];
    translate([i * dimple_pitch + jx, j * dimple_pitch + jy, texture_depth - r])
      sphere(r = r, $fn = 24);
  }
}

module front() {
  difference() {
    union() {
      shell(outer_l, outer_w, front_depth + wall, front_depth);
      front_icons();
    }
    if (texture)
      difference() {
        front_texture();
        // Keep the texture off the icons.
        front_icons();
      }

    // Display window, through the front face.
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
}

module back() {
  difference() {
    shell(outer_l, outer_w, back_depth, back_depth - wall);

    // USB-C opening in the short wall.
    translate([-1, wall + usb_centre_w - usb_w / 2, wall + usb_from_back - usb_h / 2])
      cube([wall + 2, usb_w, usb_h]);

    // Button openings through the long wall.
    for (p = button_positions)
      translate([wall + fit + p, -1, back_depth - button_from_edge])
        rotate([-90, 0, 0]) cylinder(d = button_hole_d, h = wall + 2);
  }

  // Battery pocket walls, beside the Feather.
  translate([outer_l - wall - batt_l - 2, wall, wall])
    difference() {
      slab(batt_l + 2 * 1.2, batt_w + 2 * 1.2, batt_t, 1);
      translate([1.2, 1.2, -1]) slab(batt_l, batt_w, batt_t + 2, 1);
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
stand_grip = 22;         // how far up the case the sleeve holds it
stand_wall = 3;
stand_slop = 0.6;        // slide clearance around the case

module stand() {
  slot_l = outer_l + 2 * stand_slop;
  slot_t = stack_t + 2 * wall + 2 * stand_slop;
  base_l = slot_l + 2 * stand_wall;
  base_d = slot_t + 2 * stand_wall + 12;   // depth on the desk, for stability

  difference() {
    hull() {
      slab(base_l, base_d, stand_wall, 4);
      rotate([stand_angle, 0, 0])
        translate([0, 0, 0]) slab(base_l, slot_t + 2 * stand_wall, stand_grip, 4);
    }
    // Slot the case slides down into.
    rotate([stand_angle, 0, 0])
      translate([stand_wall, stand_wall, stand_wall])
        slab(slot_l, slot_t, stand_grip + 20, 3);
    // Opening at the front so the lower part of the display stays visible.
    rotate([stand_angle, 0, 0])
      translate([stand_wall + 8, -1, stand_wall + 6])
        cube([slot_l - 16, stand_wall + 2, stand_grip]);
  }
}

/* ----------------------------------------------------------------- render */

if (part == "front") front();
else if (part == "back") back();
else if (part == "stand") stand();
else {
  front();
  translate([0, outer_w + 10, 0]) back();
  translate([0, -(stack_t + 2 * wall + 2 * stand_wall + 24), 0]) stand();
}
