#pragma once

struct Vec3 {
    float x;
    float y;
    float z;
};

struct WarpHole3D {
    Vec3 center;
    float radius;
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
    Vec3 color;
};

struct Aabb {
    Vec3 center;
    Vec3 halfSize;
    Vec3 color;
};

struct Camera {
    Vec3 position;
    float yawHorizontalDegree;
    float pitchVerticalDegree;
    float fovViewDegree;
};
