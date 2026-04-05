#pragma once

#include "wormhole3d_types.h"

float clampf(float v, float lo, float hi);
float length3(const Vec3& v);
Vec3 add3(const Vec3& a, const Vec3& b);
Vec3 sub3(const Vec3& a, const Vec3& b);
Vec3 scale3(const Vec3& v, float s);
float dot3(const Vec3& a, const Vec3& b);
Vec3 normalize3(const Vec3& v);
Vec3 abs3(const Vec3& v);
Vec3 max3(const Vec3& v, float m);

Vec3 calculateBezierPoint(float t, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3);

Vec3 rayForward();
Vec3 rayRight();
Vec3 rayUp();

Vec3 warpFieldFromHole(const Vec3& p, const WarpHole3D& hole);
Vec3 warpField(const Vec3& p);

float SignedDistanceSphere(const Vec3& p, const Sphere& s);
float SignedDistanceAabb(const Vec3& p, const Aabb& b);
float SignedDistanceFloor(const Vec3& p);
float SignedDistanceScene(const Vec3& p);
Vec3 sceneColorAt(const Vec3& p);
Vec3 estimateNormal(const Vec3& p);

Vec3 teleportToOppositeSide(
    const Vec3& entryPoint,
    const WarpHole3D& source,
    const WarpHole3D& destination,
    float margin);

struct DayNightLighting {
    Vec3 sunDir{};
    float sunDiffuse = 0.1f;
    float ambient = 0.055f;
    float pointLightScale = 1.0f;
    /** 0 = noite, 1 = dia (céu e clear). */
    float skyDayFactor = 0.0f;
};

/** Parâmetros coerentes com `gSceneTimeMs` e `gDayNightCycleMs`. */
void computeDayNightLighting(DayNightLighting& out);

Vec3 skyColor(const Vec3& dir);
Vec3 traceRay(const Vec3& origin, Vec3 dir);
