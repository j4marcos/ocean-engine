#include "wormhole3d_simulation.h"
#include "wormhole3d_globals.h"
#include "scene_prefabs.h"
#include "scene_moving.h"
#include "scene/bvh.h"
#include "cpu_texture.h"

#include <algorithm>
#include <cmath>

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

float length3(const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 add3(const Vec3& a, const Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 sub3(const Vec3& a, const Vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 scale3(const Vec3& v, const float s) {
    return {v.x * s, v.y * s, v.z * s};
}

float dot3(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 normalize3(const Vec3& v) {
    const float len = length3(v);
    if (len <= 1e-6f) {
        return {0.0f, 0.0f, 0.0f};
    }
    return {v.x / len, v.y / len, v.z / len};
}

Vec3 abs3(const Vec3& v) {
    return {std::fabs(v.x), std::fabs(v.y), std::fabs(v.z)};
}

Vec3 max3(const Vec3& v, const float m) {
    return {std::max(v.x, m), std::max(v.y, m), std::max(v.z, m)};
}

Vec3 calculateBezierPoint(const float t, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3) {
    const float u = 1.0f - t;
    const float tt = t * t;
    const float uu = u * u;
    const float uuu = uu * u;
    const float ttt = tt * t;

    Vec3 p;
    p.x = uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x;
    p.y = uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y;
    p.z = uuu * p0.z + 3 * uu * t * p1.z + 3 * u * tt * p2.z + ttt * p3.z;
    return p;
}

Vec3 calculateBezierDerivative(const float t, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3) {
    const float u = 1.0f - t;
    const float uu = u * u;
    const float tt = t * t;
    Vec3 d;
    d.x = 3.0f * uu * (p1.x - p0.x) + 6.0f * u * t * (p2.x - p1.x) + 3.0f * tt * (p3.x - p2.x);
    d.y = 3.0f * uu * (p1.y - p0.y) + 6.0f * u * t * (p2.y - p1.y) + 3.0f * tt * (p3.y - p2.y);
    d.z = 3.0f * uu * (p1.z - p0.z) + 6.0f * u * t * (p2.z - p1.z) + 3.0f * tt * (p3.z - p2.z);
    return d;
}

void carBeachMotionSample(const int carIndex, Vec3& outAnchor, float& outForwardX, float& outForwardZ) {
    const float yA = carAnchorYWorld();
    const BezierPath4& path = gBezierCarBeach;
    static constexpr float kPhase[kMovingCarCount] = {0.0f, 1.0f / 3.0f, 2.0f / 3.0f};
    float u = std::fmod(gSceneTimeSec / kCarBeachLapSec + kPhase[carIndex], 1.0f);
    if (u < 0.0f) {
        u += 1.0f;
    }
    const float twoPi = 2.0f * 3.14159265359f;
    const float tParam = u + kCarBeachEaseMiddle * std::sin(twoPi * u) / twoPi;

    const Vec3 p = calculateBezierPoint(tParam, path.p0, path.p1, path.p2, path.p3);
    Vec3 d = calculateBezierDerivative(tParam, path.p0, path.p1, path.p2, path.p3);
    d.y = 0.0f;
    const float lenH = std::sqrt(d.x * d.x + d.z * d.z);
    if (lenH <= 1e-5f) {
        outForwardX = 1.0f;
        outForwardZ = 0.0f;
    } else {
        outForwardX = d.x / lenH;
        outForwardZ = d.z / lenH;
    }
    const float laneShift = kCarLaneZ[carIndex] - kCarRoadPathCenterZ;
    outAnchor = {p.x, yA, p.z + laneShift};
}

void movingCarsCompute(Vec3 centers[3]) {
    for (int i = 0; i < kMovingCarCount; ++i) {
        float tx = 0.0f;
        float tz = 0.0f;
        carBeachMotionSample(i, centers[i], tx, tz);
    }
}

Vec3 rayForward() {
    const float cp = std::cos(gCamera.pitchVerticalDegree);
    return normalize3({
        std::sin(gCamera.yawHorizontalDegree) * cp,
        std::sin(gCamera.pitchVerticalDegree),
        -std::cos(gCamera.yawHorizontalDegree) * cp
    });
}

Vec3 rayRight() {
    const Vec3 f = rayForward();
    return normalize3({f.z, 0.0f, -f.x});
}

Vec3 rayUp() {
    const Vec3 r = rayRight();
    const Vec3 f = rayForward();
    return normalize3({
        r.y * f.z - r.z * f.y,
        r.z * f.x - r.x * f.z,
        r.x * f.y - r.y * f.x
    });
}

Vec3 warpFieldFromHole(const Vec3& p, const WarpHole3D& hole) {
    const Vec3 toCenter = sub3(hole.center, p);
    const float d = length3(toCenter);
    const float softened = d + 0.04f;
    const float influence = std::exp(-(d * d) / (hole.warpRadius * hole.warpRadius));
    const float magnitude = (hole.strength * influence) / (softened * softened);
    return scale3(normalize3(toCenter), magnitude);
}

Vec3 warpField(const Vec3& p) {
    return add3(warpFieldFromHole(p, gWormhole.holeA), warpFieldFromHole(p, gWormhole.holeB));
}

float SignedDistanceSphere(const Vec3& p, const Sphere& s) {
    return length3(sub3(p, s.center)) - s.radius;
}

float SignedDistanceAabb(const Vec3& p, const Aabb& b) {
    const Vec3 q = sub3(abs3(sub3(p, b.center)), b.halfSize);
    const Vec3 qmax = max3(q, 0.0f);
    const float outside = length3(qmax);
    const float inside = std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
    return outside + inside;
}

float SignedDistanceFloor(const Vec3& p) {
    return p.y + 1.15f;
}

void movingBezierSpheresCompute(Vec3 spheres[3]) {
    const float t = std::fmod(gSceneTimeSec * 0.12f, 1.0f);
    for (int b = 0; b < kMovingBezierSphereCount; ++b) {
        const float tb = std::fmod(t + static_cast<float>(b) * 0.31f, 1.0f);
        const BezierPath4& path = gBezierMovingSpheres[static_cast<size_t>(b)];
        spheres[b] = calculateBezierPoint(tb, path.p0, path.p1, path.p2, path.p3);
    }
}

float SignedDistanceScene(const Vec3& p) {
    return SignedDistanceSceneBvh(p);
}

float SignedDistanceSceneBvh(const Vec3& p) {
    float d = SignedDistanceFloor(p);

    // Ainda não temos BVH na CPU, fallback para todos objetos
    // TODO: Implementar traversal de BVH aqui
    for (size_t i = 0; i < gSpheres.size(); ++i) {
        d = std::min(d, SignedDistanceSphere(p, gSpheres[i]));
    }
    for (size_t i = 0; i < gBoxes.size(); ++i) {
        d = std::min(d, SignedDistanceAabb(p, gBoxes[i]));
    }
    return d;
}

Vec3 sceneColorAt(const Vec3& p) {
    return sceneColorAtBvh(p);
}

// Classificação de AABB (espelho das funções no raster)
static inline bool simIsBuilding(const Aabb& b) {
    return b.halfSize.x >= 1.0f && b.halfSize.x <= 3.0f
        && b.halfSize.z >= 1.0f && b.halfSize.z <= 3.0f
        && b.halfSize.y > 1.5f;
}
static inline bool simIsMountain(const Aabb& b) {
    return b.halfSize.x > 5.0f && b.halfSize.y > 1.0f;
}
static inline bool simIsTiny(const Aabb& b) {
    return b.halfSize.x < 0.01f && b.halfSize.y < 0.01f && b.halfSize.z < 0.01f;
}

Vec3 sceneColorAtBvh(const Vec3& p) {
    float bestD = SignedDistanceFloor(p);
    Vec3 color = {kSceneFloorMaterial.r, kSceneFloorMaterial.g, kSceneFloorMaterial.b};

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const float d = SignedDistanceSphere(p, gSpheres[i]);
        if (d < bestD) {
            bestD = d;
            const RGBA& c = gSpheres[i].color;
            color = {c.r, c.g, c.b};
        }
    }

    for (size_t i = 0; i < gBoxes.size(); ++i) {
        const Aabb& box = gBoxes[i];
        if (simIsTiny(box)) continue;
        const float d = SignedDistanceAabb(p, box);
        if (d < bestD) {
            bestD = d;
            const RGBA& c = box.color;

            if (simIsBuilding(box) && gCpuBrickDiffuse.loaded()) {
                // UV tiling ~1.5m por tile, baseado na face tocada
                float u = 0.0f, v = 0.0f;
                aabbSurfaceUV(
                    p.x, p.y, p.z,
                    box.center.x, box.center.y, box.center.z,
                    box.halfSize.x, box.halfSize.y, box.halfSize.z,
                    1.5f, u, v);
                const CpuTexSample s = sampleTexBilinear(gCpuBrickDiffuse, u, v);
                color = {s.r, s.g, s.b};
            } else if (simIsMountain(box) && gCpuTerrainDiffuse.loaded()) {
                // UV tiling ~4m por tile
                float u = 0.0f, v = 0.0f;
                aabbSurfaceUV(
                    p.x, p.y, p.z,
                    box.center.x, box.center.y, box.center.z,
                    box.halfSize.x, box.halfSize.y, box.halfSize.z,
                    4.0f, u, v);
                const CpuTexSample s = sampleTexBilinear(gCpuTerrainDiffuse, u, v);
                color = {s.r, s.g, s.b};
            } else {
                // Cor sólida para postes, lajes, barcos, etc.
                color = {c.r, c.g, c.b};
            }
        }
    }

    return color;
}

Vec3 estimateNormal(const Vec3& p) {
    const float e = 0.01f;
    const float dx = SignedDistanceScene({p.x + e, p.y, p.z}) - SignedDistanceScene({p.x - e, p.y, p.z});
    const float dy = SignedDistanceScene({p.x, p.y + e, p.z}) - SignedDistanceScene({p.x, p.y - e, p.z});
    const float dz = SignedDistanceScene({p.x, p.y, p.z + e}) - SignedDistanceScene({p.x, p.y, p.z - e});
    return normalize3({dx, dy, dz});
}

namespace {

constexpr float kFloorY = -1.15f;
constexpr float kShadowBias = 0.06f;
constexpr float kShadowTMinPrim = 0.04f;
constexpr float kShadowPlaneMinT = 0.22f;
constexpr float kShadowMaxDist = 400.0f;

float raySphereMinT(const Vec3& o, const Vec3& d, const Vec3& c, const float r, const float tMin) {
    const Vec3 L = sub3(o, c);
    const float b = dot3(d, L);
    const float disc = b * b - (dot3(L, L) - r * r);
    if (disc < 0.0f) {
        return -1.0f;
    }
    const float s = std::sqrt(disc);
    const float tA = -b - s;
    const float tB = -b + s;
    if (tA >= tMin) {
        return tA;
    }
    if (tB >= tMin) {
        return tB;
    }
    return -1.0f;
}

bool rayAabbHitSegment(const Vec3& o, const Vec3& d, const Vec3& c, const Vec3& h, const float tMinEps, const float maxDist, float& tHit) {
    float t0 = tMinEps;
    float t1 = maxDist;
    for (int axis = 0; axis < 3; ++axis) {
        const float oa = axis == 0 ? o.x : (axis == 1 ? o.y : o.z);
        const float da = axis == 0 ? d.x : (axis == 1 ? d.y : d.z);
        const float mn = axis == 0 ? (c.x - h.x) : (axis == 1 ? (c.y - h.y) : (c.z - h.z));
        const float mx = axis == 0 ? (c.x + h.x) : (axis == 1 ? (c.y + h.y) : (c.z + h.z));
        if (std::fabs(da) < 1e-8f) {
            if (oa < mn || oa > mx) {
                return false;
            }
        } else {
            const float invD = 1.0f / da;
            float tNear = (mn - oa) * invD;
            float tFar = (mx - oa) * invD;
            if (tNear > tFar) {
                std::swap(tNear, tFar);
            }
            t0 = std::max(t0, tNear);
            t1 = std::min(t1, tFar);
            if (t0 > t1) {
                return false;
            }
        }
    }
    float tCand = t0;
    if (tCand < tMinEps) {
        tCand = t1;
    }
    if (tCand < tMinEps || tCand > maxDist) {
        return false;
    }
    tHit = tCand;
    return true;
}

/** Raio reto em direção ao sol: bloqueio por chão (plano), esferas e AABBs (sem sphere tracing). */
bool directionalShadowOccluded(const Vec3& p, const Vec3& n, const Vec3& sunDir) {
    const Vec3 o = add3(p, add3(scale3(n, kShadowBias), scale3(sunDir, kShadowBias * 0.5f)));

    if (std::fabs(sunDir.y) > 1e-6f) {
        const float tPlane = (kFloorY - o.y) / sunDir.y;
        if (tPlane >= kShadowPlaneMinT && tPlane < kShadowMaxDist) {
            return true;
        }
    }

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const Sphere& s = gSpheres[i];
        const float t = raySphereMinT(o, sunDir, s.center, s.radius, kShadowTMinPrim);
        if (t > 0.0f && t < kShadowMaxDist) {
            return true;
        }
    }
    for (size_t i = 0; i < gBoxes.size(); ++i) {
        const Aabb& b = gBoxes[i];
        float tBox = 0.0f;
        if (rayAabbHitSegment(o, sunDir, b.center, b.halfSize, kShadowTMinPrim, kShadowMaxDist, tBox)) {
            return true;
        }
    }
    return false;
}

/** Raio de p em direção à luz pontual: bloqueio antes de alcançar a fonte (mesma geometria que o sol). */
bool pointLightShadowOccluded(const Vec3& p, const Vec3& n, const Vec3& lightPos, float distToLight) {
    if (distToLight < kShadowPlaneMinT) {
        return false;
    }
    const Vec3 toL = sub3(lightPos, p);
    const Vec3 Ld = normalize3(toL);
    const Vec3 o = add3(p, add3(scale3(n, kShadowBias), scale3(Ld, kShadowBias * 0.5f)));
    const float segLen = dot3(sub3(lightPos, o), Ld);
    if (segLen <= kShadowTMinPrim) {
        return false;
    }
    const float maxT = std::min(segLen - 1e-3f, kShadowMaxDist);

    if (std::fabs(Ld.y) > 1e-6f) {
        const float tPlane = (kFloorY - o.y) / Ld.y;
        if (tPlane >= kShadowPlaneMinT && tPlane < maxT) {
            return true;
        }
    }

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const Sphere& s = gSpheres[i];
        // Esfera grande do terreno/ilha: raios longos até o farol atravessam-na em falso positivo.
        if (s.radius >= 45.0f) {
            continue;
        }
        const float t = raySphereMinT(o, Ld, s.center, s.radius, kShadowTMinPrim);
        if (t > 0.0f && t < maxT) {
            return true;
        }
    }
    for (size_t i = 0; i < gBoxes.size(); ++i) {
        const Aabb& b = gBoxes[i];
        float tBox = 0.0f;
        if (rayAabbHitSegment(o, Ld, b.center, b.halfSize, kShadowTMinPrim, maxT, tBox)) {
            return true;
        }
    }
    return false;
}

} // namespace

Vec3 teleportToOppositeSide(
    const Vec3& entryPoint,
    const WarpHole3D& source,
    const WarpHole3D& destination,
    const float margin
) {
    Vec3 normal = normalize3(sub3(entryPoint, source.center));
    if (length3(normal) <= 1e-6f) {
        normal = {1.0f, 0.0f, 0.0f};
    }
    return add3(destination.center, scale3(normal, -(destination.coreRadius + margin)));
}

void cameraApplyWormholeTeleportIfNeeded(const Vec3& positionBefore, Vec3& positionAfter) {
    static constexpr float kExitMargin = 0.04f;
    const float prevDistA = length3(sub3(positionBefore, gWormhole.holeA.center));
    const float nextDistA = length3(sub3(positionAfter, gWormhole.holeA.center));
    const float prevDistB = length3(sub3(positionBefore, gWormhole.holeB.center));
    const float nextDistB = length3(sub3(positionAfter, gWormhole.holeB.center));

    if (prevDistA >= gWormhole.holeA.coreRadius && nextDistA < gWormhole.holeA.coreRadius) {
        positionAfter = teleportToOppositeSide(positionAfter, gWormhole.holeA, gWormhole.holeB, kExitMargin);
    } else if (prevDistB >= gWormhole.holeB.coreRadius && nextDistB < gWormhole.holeB.coreRadius) {
        positionAfter = teleportToOppositeSide(positionAfter, gWormhole.holeB, gWormhole.holeA, kExitMargin);
    }
}

void computeDayNightLighting(DayNightLighting& out) {
    static DayNightLighting cache;
    static int cacheMs = -1;
    if (gDayNightEffectiveMs == cacheMs) {
        out = cache;
        return;
    }
    cacheMs = gDayNightEffectiveMs;

    const int cycle = gDayNightCycleMs > 0 ? gDayNightCycleMs : 120000;
    const int modT = cycle > 0 ? ((gDayNightEffectiveMs % cycle) + cycle) % cycle : 0;
    const float phase = static_cast<float>(modT) / static_cast<float>(cycle);
    const float angle = 2.0f * 3.14159265f * phase - 1.57079633f;
    const float sx = std::cos(angle);
    const float sy = std::sin(angle);
    const float sz = gSunDirWobbleAmp * std::sin(2.0f * angle);
    cache.sunDir = normalize3({sx, sy, sz});
    const float sunElev = sy;
    const float day = clampf((sunElev + gSkyDayHorizonBias) / gSkyDaySpan, 0.0f, 1.0f);
    cache.skyDayFactor = day;
    const float sunUp = std::max(0.0f, sunElev);
    cache.sunDiffuse = sunUp * gSunDiffuseScale * (0.35f + 0.65f * day);
    cache.ambient = gAmbientBase + gAmbientDayScale * day;
    cache.pointLightScale = 1.0f - gPointLightNightMix * day;

    out = cache;
}

Vec3 skyColor(const Vec3& dir) {
    DayNightLighting dn;
    computeDayNightLighting(dn);
    const float t = 0.5f * (dir.y + 1.0f);
    const Vec3 nightSky = {
        0.008f + 0.028f * t,
        0.012f + 0.042f * t,
        0.028f + 0.085f * t
    };
    const Vec3 daySky = {
        0.12f + 0.42f * t,
        0.18f + 0.48f * t,
        0.35f + 0.38f * t
    };
    Vec3 base = {
        nightSky.x + (daySky.x - nightSky.x) * dn.skyDayFactor,
        nightSky.y + (daySky.y - nightSky.y) * dn.skyDayFactor,
        nightSky.z + (daySky.z - nightSky.z) * dn.skyDayFactor
    };
    const Vec3 toA = normalize3(sub3(gWormhole.holeA.center, gCamera.position));
    const Vec3 toB = normalize3(sub3(gWormhole.holeB.center, gCamera.position));
    const float da = std::max(dot3(dir, toA), 0.0f);
    const float db = std::max(dot3(dir, toB), 0.0f);
    const Vec3 portalTint = {0.08f, 0.1f, 0.22f};
    const float blend = std::pow(da, 12.0f) * 0.35f + std::pow(db, 12.0f) * 0.35f;
    const Vec3 portalMix = add3(portalTint, scale3(base, 0.2f));
    return {
        base.x * (1.0f - blend) + portalMix.x * blend,
        base.y * (1.0f - blend) + portalMix.y * blend,
        base.z * (1.0f - blend) + portalMix.z * blend
    };
}

static bool isInfiniteFloorAt(const Vec3& p) {
    const float df = SignedDistanceFloor(p);
    const float dScene = SignedDistanceScene(p);
    return std::fabs(df - dScene) < kOceanFloorMatchEps;
}

static Vec3 shadeSurfaceOpaque(const Vec3& p, const Vec3& n, const Vec3& base) {
    DayNightLighting dn;
    computeDayNightLighting(dn);
    float sunMul = 1.0f;
    if (dn.sunDiffuse > 1e-6f && dot3(n, dn.sunDir) > 0.0f) {
        if (directionalShadowOccluded(p, n, dn.sunDir)) {
            sunMul = 0.0f;
        }
    }
    const float sun = clampf(dot3(n, dn.sunDir), 0.0f, 1.0f) * dn.sunDiffuse * sunMul;
    Vec3 acc = scale3(base, dn.ambient + sun);
    for (const auto& L : gPointLights) {
        const Vec3 toL = sub3(L.position, p);
        const float d = length3(toL);
        if (d < 1e-5f) {
            continue;
        }
        const Vec3 Ld = normalize3(toL);
        const float ndl = clampf(dot3(n, Ld), 0.0f, 1.0f);
        float plMul = 1.0f;
        if (ndl > 1e-6f && pointLightShadowOccluded(p, n, L.position, d)) {
            plMul = 0.0f;
        }
        const float edge = 1.0f - clampf(d / (L.range * 2.0f), 0.0f, 1.0f);
        const float dr = d / std::max(L.range, 1e-4f);
        const float atten = edge * edge / (1.0f + dr * dr);
        const Vec3 tint = scale3(L.color, ndl * atten * 0.52f * dn.pointLightScale * plMul);
        acc = add3(acc, {base.x * tint.x, base.y * tint.y, base.z * tint.z});
    }

    return {clampf(acc.x, 0.0f, 1.0f), clampf(acc.y, 0.0f, 1.0f), clampf(acc.z, 0.0f, 1.0f)};
}

static Vec3 applyOceanTint(const Vec3& c) {
    return {
        clampf(c.x * kOceanReflectionTint.x, 0.0f, 1.0f),
        clampf(c.y * kOceanReflectionTint.y, 0.0f, 1.0f),
        clampf(c.z * kOceanReflectionTint.z, 0.0f, 1.0f)
    };
}

Vec3 traceRay(const Vec3& origin, Vec3 dir, const int bounce) {
    Vec3 position = origin;
    const float stepLength = 0.15f;
    const float maxSphereStep = 0.25f;
    const float maxDist = 75.0f;
    const int maxSteps = 420;
    const float hitEps = 0.08f;
    const float warpHitDirEps = 0.02f;
    const float exitMargin = 0.04f;

    // Early exit: raio não intersecta a bounding sphere → devolve céu imediatamente
    if (bounce == 0 && gUseBvh && scene::gSceneBoundingSphereRadius > 1.0f) {
        const float tScene = scene::rayIntersectsSceneBoundingSphere(origin, dir);
        if (tScene < 0.0f) {
            return skyColor(dir);
        }
    }

    for (int i = 0; i < maxSteps; ++i) {
        // Usa sempre SignedDistanceSceneBvh local (que já faz o fallback completo)
        const float dScene = SignedDistanceSceneBvh(position);

        if (dScene < hitEps) {
            const Vec3 n = estimateNormal(position);
            const float distA = length3(sub3(position, gWormhole.holeA.center));
            const float distB = length3(sub3(position, gWormhole.holeB.center));
            const bool insideWarp =
                distA < gWormhole.holeA.warpRadius || distB < gWormhole.holeB.warpRadius;
            if (insideWarp) {
                const float dn = dot3(dir, n);
                if (dn >= -warpHitDirEps) {
                    position = add3(position, scale3(dir, stepLength * 0.35f));
                    continue;
                }
            }

            const Vec3 base = sceneColorAtBvh(position);

            if (bounce == 0 && isInfiniteFloorAt(position)) {
                const Vec3 d = normalize3(dir);
                const Vec3 refl = sub3(d, scale3(n, 2.0f * dot3(d, n)));
                const Vec3 p2 = add3(position, scale3(n, kOceanReflectBias));
                Vec3 reflected = traceRay(p2, refl, 1);
                reflected = applyOceanTint(reflected);
                const float NdotV = clampf(dot3(n, scale3(d, -1.0f)), 0.0f, 1.0f);
                const float F =
                    kOceanFresnelBase + (1.0f - kOceanFresnelBase) * std::pow(1.0f - NdotV, kOceanFresnelPower);
                const float w = F * kOceanFresnelMix;
                const Vec3 baseLit = shadeSurfaceOpaque(position, n, base);
                return {
                    clampf(baseLit.x * (1.0f - w) + reflected.x * w, 0.0f, 1.0f),
                    clampf(baseLit.y * (1.0f - w) + reflected.y * w, 0.0f, 1.0f),
                    clampf(baseLit.z * (1.0f - w) + reflected.z * w, 0.0f, 1.0f)
                };
            }
            return shadeSurfaceOpaque(position, n, base);
        }

        const float distA = length3(sub3(position, gWormhole.holeA.center));
        const float distB = length3(sub3(position, gWormhole.holeB.center));
        const bool insideWarp =
            distA < gWormhole.holeA.warpRadius || distB < gWormhole.holeB.warpRadius;

        Vec3 nextPos;
        if (!insideWarp) {
            float travel = std::min(dScene, maxSphereStep);
            if (travel < 1e-5f) {
                travel = 1e-5f;
            }
            nextPos = add3(position, scale3(dir, travel));
        } else {
            const Vec3 accel = warpField(position);
            dir = normalize3(add3(dir, scale3(accel, stepLength * 0.85f)));
            nextPos = add3(position, scale3(dir, stepLength));
        }

        const float prevDistA = distA;
        const float nextDistA = length3(sub3(nextPos, gWormhole.holeA.center));
        const float prevDistB = distB;
        const float nextDistB = length3(sub3(nextPos, gWormhole.holeB.center));

        if (prevDistA >= gWormhole.holeA.coreRadius && nextDistA < gWormhole.holeA.coreRadius) {
            nextPos = teleportToOppositeSide(nextPos, gWormhole.holeA, gWormhole.holeB, exitMargin);
        } else if (prevDistB >= gWormhole.holeB.coreRadius && nextDistB < gWormhole.holeB.coreRadius) {
            nextPos = teleportToOppositeSide(nextPos, gWormhole.holeB, gWormhole.holeA, exitMargin);
        }

        position = nextPos;

        if (length3(sub3(position, origin)) > maxDist) {
            break;
        }
    }

    if (bounce > 0) {
        return applyOceanTint(skyColor(dir));
    }
    return skyColor(dir);
}

void updateSceneBvh() {
    scene::buildBvh(scene::gSceneBvh, gSpheres, gBoxes);
    scene::computeSceneBoundingSphere(gSpheres, gBoxes);
}
