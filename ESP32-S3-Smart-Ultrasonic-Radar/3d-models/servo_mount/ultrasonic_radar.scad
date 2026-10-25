// Ultrasonic Radar Scanner - parametric printable concept
// Units: mm
// Parts: main_body, rear_cover, sensor_head, servo_bracket
// Render/export one part at a time by changing PART below.

PART = "main_body"; // main_body | rear_cover | sensor_head | servo_bracket

$fn = 64;

// ---------- Main enclosure ----------
W = 100;
D = 80;
H = 70;
WALL = 3;
CORNER = 5;

// Display opening (adjust after measuring your exact TFT module)
DISP_W = 58;
DISP_H = 43;
DISP_Z = 37;
DISP_BOTTOM = 13;

// Internal mounting zones
ESP_X = 56;
ESP_Y = 32;
PWR_X = 48;
PWR_Y = 28;

module rounded_box(size=[10,10,10], r=3) {
    x=size[0]; y=size[1]; z=size[2];
    hull() {
        for (xx=[r,x-r]) for (yy=[r,y-r])
            translate([xx,yy,0]) cylinder(r=r,h=z);
    }
}

module screw_boss(x,y,h=10,d=8,hole=2.6) {
    difference() {
        translate([x,y,0]) cylinder(d=d,h=h);
        translate([x,y,-0.5]) cylinder(d=hole,h=h+1);
    }
}

module main_body() {
    difference() {
        // Outer shell
        rounded_box([W,D,H],CORNER);
        // Inner cavity, leaving 3 mm walls and a 3 mm rear/front structure
        translate([WALL,WALL,WALL])
            rounded_box([W-2*WALL,D-2*WALL,H-2*WALL],max(1,CORNER-2));

        // Front display window
        translate([(W-DISP_W)/2,-1,DISP_BOTTOM])
            cube([DISP_W, WALL+2, DISP_H]);

        // Front lower cable/service opening
        translate([44,-1,5]) cube([12,WALL+2,5]);

        // Rear ventilation slots
        for (z=[42,49,56])
            for (x=[30,48,66])
                translate([x, D-WALL-1, z]) cube([10,WALL+2,2]);

        // Rear USB/power opening
        translate([W-25,D-WALL-1,10]) cube([15,WALL+2,8]);
    }

    // Display bezel lip on inside/front face
    translate([(W-DISP_W)/2-2,1,DISP_BOTTOM-2])
        difference() {
            cube([DISP_W+4,2,DISP_H+4]);
            translate([2,-0.5,2]) cube([DISP_W,3,DISP_H]);
        }

    // Internal mounting pads: generic slots rather than board-specific holes
    // ESP32 area
    for (x=[10, W-10])
        for (y=[45, 68])
            translate([x,y,3]) cylinder(d=7,h=5);
    // Power board area
    for (x=[65,88])
        for (y=[12,35])
            translate([x,y,3]) cylinder(d=7,h=5);

    // Four enclosure screw bosses
    for (x=[8,W-8]) for (y=[8,D-8])
        screw_boss(x,y,8,9,3.0);
}

module rear_cover() {
    T=3;
    difference() {
        rounded_box([W,D,T],CORNER);
        // screw holes
        for (x=[8,W-8]) for (y=[8,D-8])
            translate([x,y,-0.5]) cylinder(d=3.2,h=T+1);
        // USB opening
        translate([W-25,8,-0.5]) cube([15,8,T+1]);
    }
}

// Sensor head sized around the HC-SR04 module.
// Outer: 70 x 30 x 30 mm. Front holes are 17 mm diameter, 25 mm center spacing.
module sensor_head() {
    SW=70; SD=30; SH=30; R=4;
    difference() {
        rounded_box([SW,SD,SH],R);
        // HC-SR04 transducer openings on front
        for (x=[22.5,47.5])
            translate([x,-1,15]) rotate([-90,0,0]) cylinder(d=17,h=SD+2);
        // rear cavity for board/wiring
        translate([5,8,5]) cube([SW-10,SD-6,SH-8]);
        // bottom servo cable clearance
        translate([29,13,-1]) cube([12,6,8]);
    }
    // internal ledges to support sensor board
    translate([8,10,4]) cube([54,3,3]);
    // bottom mounting plate
    translate([22,9,-3]) cube([26,12,3]);
    // central servo-horn hole
    translate([35,15,-3.5]) cylinder(d=5,h=4);
}

module servo_bracket() {
    // Generic SG90/MG90S cradle: 42 x 24 x 25 mm
    BW=42; BD=24; BH=25;
    difference() {
        union() {
            // base
            rounded_box([BW,BD,4],3);
            // side supports
            translate([0,2,0]) cube([4,BD-4,BH]);
            translate([BW-4,2,0]) cube([4,BD-4,BH]);
            // top bridge
            translate([0,2,BH-4]) cube([BW,BD-4,4]);
        }
        // servo body cavity
        translate([4,4,4]) cube([BW-8,BD-8,BH-7]);
        // mounting holes in base
        for (x=[7,BW-7])
            translate([x,BD/2,-0.5]) cylinder(d=3.2,h=5);
        // side screw holes
        translate([-0.5,BD/2,12]) rotate([0,90,0]) cylinder(d=3.0,h=5);
        translate([BW-4.5,BD/2,12]) rotate([0,90,0]) cylinder(d=3.0,h=5);
    }
}

if (PART=="main_body") main_body();
if (PART=="rear_cover") rear_cover();
if (PART=="sensor_head") sensor_head();
if (PART=="servo_bracket") servo_bracket();
