#ifndef MATH_H
#define MATH_H

#include <cmath>
#include <vector>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

struct vec2 {
    double x, y;
    vec2() : x(0), y(0) {}
    vec2(double _x, double _y) : x(_x), y(_y) {}
};

struct vec3 {
    double x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
};

const double c = 299792458.0;
const double G = 6.67430e-11;

#endif
