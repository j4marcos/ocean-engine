#pragma once

struct Vec3 {
    float x;
    float y;
    float z;
};

struct RGBA {
    float r;
    float g;
    float b;
    float a;
};

struct WarpHole3D {
    Vec3 center;
    float warpRadius;
    float coreRadius;
    float strength;
};


struct Wormhole3D {
    WarpHole3D holeA;
    WarpHole3D holeB;
};

struct Sphere {
    Vec3 center;
    float radius;
    RGBA color;
};

struct Aabb {
    Vec3 center;
    Vec3 halfSize;
    RGBA color;
};

struct Camera {
    Vec3 position;
    float yawHorizontalDegree;
    float pitchVerticalDegree;
    float fovViewDegree;
};

// Luz pontual para raycast (postes, etc.): cor linear e alcance de atenuação.
struct PointLight {
    Vec3 position;
    Vec3 color;
    float range;
};

/** Curva de Bézier cúbica (figuras móveis genéricas): controlos em `scene_world.cpp`. */
struct BezierPath4 {
    Vec3 p0;
    Vec3 p1;
    Vec3 p2;
    Vec3 p3;
};
