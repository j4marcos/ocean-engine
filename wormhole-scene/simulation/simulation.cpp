#include "wormhole3d_simulation.h"
#include "wormhole3d_globals.h"
#include <algorithm>
#include <cmath>

float clampf(const float v, const float lo, const float hi) {
    return std::max(lo, std::min(v, hi));
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
    const float influence = std::exp(-(d * d) / (hole.radius * hole.radius));
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

float SignedDistanceScene(const Vec3& p) {
    float d = SignedDistanceFloor(p);
    for (size_t i = 0; i < gSpheres.size(); ++i) {
        d = std::min(d, SignedDistanceSphere(p, gSpheres[i]));
    }
    for (size_t i = 0; i < gBoxes.size(); ++i) {
        d = std::min(d, SignedDistanceAabb(p, gBoxes[i]));
    }
    return d;
}

Vec3 sceneColorAt(const Vec3& p) {
    float bestD = SignedDistanceFloor(p);
    Vec3 color = {0.35f, 0.37f, 0.41f};

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const float d = SignedDistanceSphere(p, gSpheres[i]);
        if (d < bestD) {
            bestD = d;
            color = gSpheres[i].color;
        }
    }

    for (size_t i = 0; i < gBoxes.size(); ++i) {
        const float d = SignedDistanceAabb(p, gBoxes[i]);
        if (d < bestD) {
            bestD = d;
            color = gBoxes[i].color;
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

Vec3 skyColor(const Vec3& dir) {
    const float t = 0.5f * (dir.y + 1.0f);
    return {
        0.03f + 0.20f * t,
        0.04f + 0.22f * t,
        0.07f + 0.34f * t
    };
}

Vec3 traceRay(const Vec3& origin, Vec3 dir) {
    Vec3 position = origin;
    const float stepLength = 0.15f;
    const float maxDist = 75.0f;
    const int maxSteps = 420;
    const float hitEps = 0.08f;
    const float exitMargin = 0.04f;

    for (int i = 0; i < maxSteps; ++i) {
        const Vec3 accel = warpField(position);
        dir = normalize3(add3(dir, scale3(accel, stepLength * 0.85f)));

        Vec3 nextPos = add3(position, scale3(dir, stepLength));

        const float prevDistA = length3(sub3(position, gWormhole.holeA.center));
        const float nextDistA = length3(sub3(nextPos, gWormhole.holeA.center));
        const float prevDistB = length3(sub3(position, gWormhole.holeB.center));
        const float nextDistB = length3(sub3(nextPos, gWormhole.holeB.center));

        if (prevDistA >= gWormhole.holeA.coreRadius && nextDistA < gWormhole.holeA.coreRadius) {
            nextPos = teleportToOppositeSide(nextPos, gWormhole.holeA, gWormhole.holeB, exitMargin);
        } else if (prevDistB >= gWormhole.holeB.coreRadius && nextDistB < gWormhole.holeB.coreRadius) {
            nextPos = teleportToOppositeSide(nextPos, gWormhole.holeB, gWormhole.holeA, exitMargin);
        }

        position = nextPos;

        const float d = SignedDistanceScene(position);
        if (d < hitEps) {
            const Vec3 n = estimateNormal(position);
            const Vec3 lightDir = normalize3({0.62f, 0.74f, 0.23f});
            const float lambert = clampf(dot3(n, lightDir), 0.0f, 1.0f);
            const Vec3 base = sceneColorAt(position);
            const float amb = 0.22f;
            const float shade = amb + lambert * 0.78f;
            return {base.x * shade, base.y * shade, base.z * shade};
        }

        if (length3(sub3(position, origin)) > maxDist) {
            break;
        }
    }

    return skyColor(dir);
}
