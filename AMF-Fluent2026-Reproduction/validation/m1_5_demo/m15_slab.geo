// Isolated M1.5 DEMO: 100 mm x 10 mm 2-D slab, one cell through height.
// The long direction is the intended 1-D conduction direction.
SetFactory("Built-in");
L = 0.100;
H = 0.010;

Point(1) = {0, 0, 0, 1.0};
Point(2) = {L, 0, 0, 1.0};
Point(3) = {L, H, 0, 1.0};
Point(4) = {0, H, 0, 1.0};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};
Line Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

Transfinite Curve {1, 3} = 101 Using Progression 1;
Transfinite Curve {2, 4} = 2 Using Progression 1;
Transfinite Surface {1};
Recombine Surface {1};

Physical Surface("aluminum_demo") = {1};
Physical Curve("symmetry_bottom") = {1};
Physical Curve("cold_wall") = {2};
Physical Curve("symmetry_top") = {3};
Physical Curve("hot_wall") = {4};

Mesh.ElementOrder = 1;
