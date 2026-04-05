#ifndef WORMHOLE3D_GPU_SHADERS_H
#define WORMHOLE3D_GPU_SHADERS_H

// GLSL 1.20 — primitivas SDF (manter alinhado com simulation.cpp)
static const char kGlslSdfPrimitives[] = R"GLSL(

float length3(vec3 v) { return length(v); }
vec3 normalize3(vec3 v) {
    float len = length(v);
    return len > 1e-6 ? v / len : vec3(0.0);
}
float clampf(float v, float lo, float hi) { return clamp(v, lo, hi); }

float sdfSphere(vec3 p, vec3 c, float r) {
    return length3(p - c) - r;
}

float sdfAabb(vec3 p, vec3 center, vec3 halfSize) {
    vec3 q = abs(p - center) - halfSize;
    vec3 qmax = max(q, vec3(0.0));
    float outside = length3(qmax);
    float inside = min(max(q.x, max(q.y, q.z)), 0.0);
    return outside + inside;
}

float sdfFloor(vec3 p) {
    return p.y + 1.15;
}

)GLSL";

// Fragment principal (prefixar em C++ com "#version 120\n" antes de kGlslSdfPrimitives + este corpo)
static const char kGlslRaycastFragment[] = R"GLSL(

uniform vec3 uCamPos;
uniform vec3 uRayForward;
uniform vec3 uRayRight;
uniform vec3 uRayUp;
uniform vec2 uResolution;
uniform float uAspect;
uniform float uTanHalfFov;

uniform vec3 uHoleA_center;
uniform float uHoleA_radius;
uniform float uHoleA_coreRadius;
uniform float uHoleA_strength;

uniform vec3 uHoleB_center;
uniform float uHoleB_radius;
uniform float uHoleB_coreRadius;
uniform float uHoleB_strength;

uniform sampler2D uSceneData;
uniform float uSceneInvW;
uniform int uObjectCount;

uniform int uPointCount;
uniform float uPointRange[8];
uniform vec3 uPointPos[8];
uniform vec3 uPointCol[8];

uniform vec3 uSunDir;
uniform float uSunDiffuse;
uniform float uAmbient;
uniform float uPointLightScale;
uniform float uSkyDayFactor;
uniform float uSceneTimeSec;

vec3 cubicBezier(float t, vec3 p0, vec3 p1, vec3 p2, vec3 p3) {
    float u = 1.0 - t;
    float uu = u * u;
    float tt = t * t;
    return p0 * (u * uu) + p1 * (3.0 * uu * t) + p2 * (3.0 * u * tt) + p3 * (tt * t);
}

// Curvas iguais a `sceneRegisterBirdPaths()` em scene_world.cpp (GLSL 1.20 não lê gBirdBezier).
vec3 birdPos(int bi) {
    float tAnim = mod(uSceneTimeSec * 0.12, 1.0);
    float tb = mod(tAnim + float(bi) * 0.31, 1.0);
    if (bi == 0) {
        return cubicBezier(tb,
            vec3(-6.0, 5.0, -4.0), vec3(-2.0, 7.0, -5.0), vec3(3.0, 6.0, -7.0), vec3(8.0, 4.5, -9.0));
    }
    if (bi == 1) {
        return cubicBezier(tb,
            vec3(5.0, 6.0, -6.0), vec3(1.0, 8.0, -7.0), vec3(-4.0, 7.0, -8.0), vec3(-9.0, 5.0, -10.0));
    }
    return cubicBezier(tb,
        vec3(0.0, 4.0, -3.0), vec3(4.0, 9.0, -6.0), vec3(-3.0, 8.0, -9.0), vec3(6.0, 5.0, -11.0));
}

float sdfBirds(vec3 p) {
    const float br = 0.09;
    float d = 1e10;
    for (int bi = 0; bi < 3; bi++) {
        d = min(d, sdfSphere(p, birdPos(bi), br));
    }
    return d;
}

vec3 warpFieldFromHole(vec3 p, vec3 center, float radius, float strength) {
    vec3 toCenter = center - p;
    float d = length3(toCenter);
    float softened = d + 0.04;
    float influence = exp(-(d * d) / (radius * radius));
    float magnitude = (strength * influence) / (softened * softened);
    return normalize3(toCenter) * magnitude;
}

vec3 warpField(vec3 p) {
    return warpFieldFromHole(p, uHoleA_center, uHoleA_radius, uHoleA_strength)
         + warpFieldFromHole(p, uHoleB_center, uHoleB_radius, uHoleB_strength);
}

float sdfScene(vec3 p) {
    float d = sdfFloor(p);
    d = min(d, sdfBirds(p));
    for (int i = 0; i < 96; i++) {
        if (i >= uObjectCount) {
            break;
        }
        float u = (float(i) + 0.5) * uSceneInvW;
        float v0 = (0.0 + 0.5) / 3.0;
        float v1 = (1.0 + 0.5) / 3.0;
        float v2 = (2.0 + 0.5) / 3.0;
        vec4 t0 = texture2D(uSceneData, vec2(u, v0));
        vec4 t1 = texture2D(uSceneData, vec2(u, v1));
        vec4 t2 = texture2D(uSceneData, vec2(u, v2));
        vec3 center = vec3(t0.g, t0.b, t0.a);
        if (t0.r < 0.5) {
            float rad = t1.r;
            d = min(d, sdfSphere(p, center, rad));
        } else {
            vec3 halfSize = vec3(t1.r, t1.g, t1.b);
            d = min(d, sdfAabb(p, center, halfSize));
        }
    }
    return d;
}

vec3 sceneColorAt(vec3 p) {
    float bestD = sdfFloor(p);
    vec3 color = vec3(0.35, 0.37, 0.61);
    {
        float db = sdfBirds(p);
        if (db < bestD) {
            bestD = db;
            color = vec3(0.18, 0.16, 0.14);
        }
    }
    for (int i = 0; i < 96; i++) {
        if (i >= uObjectCount) {
            break;
        }
        float u = (float(i) + 0.5) * uSceneInvW;
        float v0 = (0.0 + 0.5) / 3.0;
        float v1 = (1.0 + 0.5) / 3.0;
        float v2 = (2.0 + 0.5) / 3.0;
        vec4 t0 = texture2D(uSceneData, vec2(u, v0));
        vec4 t1 = texture2D(uSceneData, vec2(u, v1));
        vec4 t2 = texture2D(uSceneData, vec2(u, v2));
        vec3 center = vec3(t0.g, t0.b, t0.a);
        if (t0.r < 0.5) {
            float di = sdfSphere(p, center, t1.r);
            if (di < bestD) {
                bestD = di;
                color = vec3(t1.g, t1.b, t1.a);
            }
        } else {
            vec3 halfSize = vec3(t1.r, t1.g, t1.b);
            float di = sdfAabb(p, center, halfSize);
            if (di < bestD) {
                bestD = di;
                color = vec3(t1.a, t2.r, t2.g);
            }
        }
    }
    return color;
}

vec3 estimateNormal(vec3 p) {
    float e = 0.01;
    float dx = sdfScene(p + vec3(e, 0.0, 0.0)) - sdfScene(p - vec3(e, 0.0, 0.0));
    float dy = sdfScene(p + vec3(0.0, e, 0.0)) - sdfScene(p - vec3(0.0, e, 0.0));
    float dz = sdfScene(p + vec3(0.0, 0.0, e)) - sdfScene(p - vec3(0.0, 0.0, e));
    return normalize3(vec3(dx, dy, dz));
}

vec3 teleportToOppositeSide(
    vec3 entryPoint,
    vec3 srcCenter, float srcCore,
    vec3 dstCenter, float dstCore,
    float margin
) {
    vec3 normal = normalize3(entryPoint - srcCenter);
    if (length3(normal) <= 1e-6) {
        normal = vec3(1.0, 0.0, 0.0);
    }
    return dstCenter + normal * (-(dstCore + margin));
}

vec3 skyColor(vec3 dir) {
    float t = 0.5 * (dir.y + 1.0);
    vec3 nightSky = vec3(
        0.008 + 0.028 * t,
        0.012 + 0.042 * t,
        0.028 + 0.085 * t
    );
    vec3 daySky = vec3(
        0.12 + 0.42 * t,
        0.18 + 0.48 * t,
        0.35 + 0.38 * t
    );
    vec3 base = mix(nightSky, daySky, uSkyDayFactor);
    vec3 toA = normalize3(uHoleA_center - uCamPos);
    vec3 toB = normalize3(uHoleB_center - uCamPos);
    float da = max(dot(dir, toA), 0.0);
    float db = max(dot(dir, toB), 0.0);
    vec3 portalTint = vec3(0.08, 0.1, 0.22);
    float blend = pow(da, 12.0) * 0.35 + pow(db, 12.0) * 0.35;
    return mix(base, portalTint + base * 0.2, blend);
}

bool isInfiniteFloorAt(vec3 p) {
    float df = sdfFloor(p);
    float dScene = sdfScene(p);
    return abs(df - dScene) < 0.04;
}

// |o + t*d - c|^2 = r^2 com d não necessariamente unitário (drivers antigos / precisão).
float raySphereMinT(vec3 o, vec3 d, vec3 c, float r, float tMin) {
    vec3 L = o - c;
    float a = dot(d, d);
    if (a < 1e-12) {
        return -1.0;
    }
    float halfB = dot(d, L);
    float disc = halfB * halfB - a * (dot(L, L) - r * r);
    if (disc < 0.0) {
        return -1.0;
    }
    float s = sqrt(disc);
    float tA = (-halfB - s) / a;
    float tB = (-halfB + s) / a;
    if (tA >= tMin) {
        return tA;
    }
    if (tB >= tMin) {
        return tB;
    }
    return -1.0;
}

// Retorna t de entrada ou -1.0 (evita parâmetro `out`: vários drivers GLSL 1.20 falham no fragment).
float rayAabbEnterT(vec3 o, vec3 d, vec3 c, vec3 h, float tMinEps, float maxDist) {
    float t0 = tMinEps;
    float t1 = maxDist;
    for (int axis = 0; axis < 3; axis++) {
        float oa = axis == 0 ? o.x : (axis == 1 ? o.y : o.z);
        float da = axis == 0 ? d.x : (axis == 1 ? d.y : d.z);
        float mn = axis == 0 ? (c.x - h.x) : (axis == 1 ? (c.y - h.y) : (c.z - h.z));
        float mx = axis == 0 ? (c.x + h.x) : (axis == 1 ? (c.y + h.y) : (c.z + h.z));
        if (abs(da) < 1e-8) {
            if (oa < mn || oa > mx) {
                return -1.0;
            }
        } else {
            float invD = 1.0 / da;
            float tNear = (mn - oa) * invD;
            float tFar = (mx - oa) * invD;
            if (tNear > tFar) {
                float tmp = tNear;
                tNear = tFar;
                tFar = tmp;
            }
            t0 = max(t0, tNear);
            t1 = min(t1, tFar);
            if (t0 > t1) {
                return -1.0;
            }
        }
    }
    float tCand = t0;
    if (tCand < tMinEps) {
        tCand = t1;
    }
    if (tCand < tMinEps || tCand > maxDist) {
        return -1.0;
    }
    return tCand;
}

float sunShadowStraight(vec3 p, vec3 n, vec3 sunDir) {
    float kBias = 0.06;
    float kTMin = 0.04;
    float kPlaneMin = 0.22;
    float kMax = 400.0;
    float kFloorY = -1.15;
    vec3 o = p + n * kBias + sunDir * (kBias * 0.5);

    if (abs(sunDir.y) > 1e-6) {
        float tPlane = (kFloorY - o.y) / sunDir.y;
        if (tPlane >= kPlaneMin && tPlane < kMax) {
            return 0.0;
        }
    }

    for (int i = 0; i < 96; i++) {
        if (i >= uObjectCount) {
            break;
        }
        float u = (float(i) + 0.5) * uSceneInvW;
        float v0 = (0.0 + 0.5) / 3.0;
        float v1 = (1.0 + 0.5) / 3.0;
        vec4 t0 = texture2D(uSceneData, vec2(u, v0));
        vec4 t1 = texture2D(uSceneData, vec2(u, v1));
        vec3 center = vec3(t0.g, t0.b, t0.a);
        if (t0.r < 0.5) {
            float rad = t1.r;
            float tS = raySphereMinT(o, sunDir, center, rad, kTMin);
            if (tS > 0.0 && tS < kMax) {
                return 0.0;
            }
        } else {
            vec3 halfSize = vec3(t1.r, t1.g, t1.b);
            float tB = rayAabbEnterT(o, sunDir, center, halfSize, kTMin, kMax);
            if (tB >= 0.0) {
                return 0.0;
            }
        }
    }
    for (int bi = 0; bi < 3; bi++) {
        vec3 bc = birdPos(bi);
        float tSb = raySphereMinT(o, sunDir, bc, 0.09, kTMin);
        if (tSb > 0.0 && tSb < kMax) {
            return 0.0;
        }
    }
    return 1.0;
}

float pointShadowStraight(vec3 p, vec3 n, vec3 lightPos) {
    vec3 toL = lightPos - p;
    float d = length3(toL);
    if (d < 1e-5) {
        return 1.0;
    }
    vec3 Ld = normalize3(toL);
    float kBias = 0.06;
    float kTMin = 0.04;
    float kPlaneMin = 0.22;
    float kMax = 400.0;
    float kFloorY = -1.15;
    vec3 o = p + n * kBias + Ld * (kBias * 0.5);
    float segLen = dot(lightPos - o, Ld);
    if (segLen <= kTMin) {
        return 1.0;
    }
    float maxT = min(segLen - 0.001, kMax);

    if (abs(Ld.y) > 1e-6) {
        float tPlane = (kFloorY - o.y) / Ld.y;
        if (tPlane >= kPlaneMin && tPlane < maxT) {
            return 0.0;
        }
    }

    for (int i = 0; i < 96; i++) {
        if (i >= uObjectCount) {
            break;
        }
        float u = (float(i) + 0.5) * uSceneInvW;
        float v0 = (0.0 + 0.5) / 3.0;
        float v1 = (1.0 + 0.5) / 3.0;
        vec4 t0 = texture2D(uSceneData, vec2(u, v0));
        vec4 t1 = texture2D(uSceneData, vec2(u, v1));
        vec3 center = vec3(t0.g, t0.b, t0.a);
        if (t0.r < 0.5) {
            float rad = t1.r;
            float tS = raySphereMinT(o, Ld, center, rad, kTMin);
            if (tS > 0.0 && tS < maxT) {
                return 0.0;
            }
        } else {
            vec3 halfSize = vec3(t1.r, t1.g, t1.b);
            float tB = rayAabbEnterT(o, Ld, center, halfSize, kTMin, maxT);
            if (tB >= 0.0) {
                return 0.0;
            }
        }
    }
    for (int bi = 0; bi < 3; bi++) {
        vec3 bc = birdPos(bi);
        float tSb = raySphereMinT(o, Ld, bc, 0.09, kTMin);
        if (tSb > 0.0 && tSb < maxT) {
            return 0.0;
        }
    }
    return 1.0;
}

vec3 shadeSurfaceOpaque(vec3 p, vec3 n, vec3 base) {
    vec3 sunDir = normalize3(uSunDir);
    float sunMul = 1.0;
    if (uSunDiffuse > 0.000001 && dot(n, sunDir) > 0.0) {
        sunMul = sunShadowStraight(p, n, sunDir);
    }
    float sun = clampf(dot(n, sunDir), 0.0, 1.0) * uSunDiffuse * sunMul;
    vec3 acc = base * (uAmbient + sun);
    for (int i = 0; i < 8; i++) {
        if (i >= uPointCount) {
            break;
        }
        vec3 toL = uPointPos[i] - p;
        float d = length3(toL);
        if (d < 1e-5) {
            continue;
        }
        vec3 Ld = normalize3(toL);
        float ndl = clampf(dot(n, Ld), 0.0, 1.0);
        float plMul = 1.0;
        if (ndl > 0.000001) {
            plMul = pointShadowStraight(p, n, uPointPos[i]);
        }
        float edge = 1.0 - clampf(d / (uPointRange[i] * 2.0), 0.0, 1.0);
        float atten = edge * edge / (1.0 + d * d * 0.032);
        vec3 tint = uPointCol[i] * (ndl * atten * 0.52 * uPointLightScale * plMul);
        acc += base * tint;
    }
    return clamp(acc, vec3(0.0), vec3(1.0));
}

vec3 applyOceanTint(vec3 c) {
    vec3 t = vec3(0.78, 0.92, 1.05);
    return min(c * t, vec3(1.0));
}

// 2.º segmento: raio vindo do plano infinito — iluminação completa, sem nova reflexão no chão.
vec3 traceRayOpaqueSegment(vec3 origin, vec3 dirIn) {
    vec3 position = origin;
    vec3 dir = dirIn;
    float stepLength = 0.15;
    float maxSphereStep = 0.25;
    float maxDist = 75.0;
    float hitEps = 0.08;
    float warpHitDirEps = 0.02;
    float exitMargin = 0.04;

    for (int i = 0; i < 420; i++) {
        float dScene = sdfScene(position);
        if (dScene < hitEps) {
            vec3 n = estimateNormal(position);
            float wDistA = length3(position - uHoleA_center);
            float wDistB = length3(position - uHoleB_center);
            bool insideWarp = (wDistA < uHoleA_radius) || (wDistB < uHoleB_radius);
            if (insideWarp) {
                float dn = dot(dir, n);
                if (dn >= -warpHitDirEps) {
                    position = position + dir * (stepLength * 0.35);
                    continue;
                }
            }
            vec3 base = sceneColorAt(position);
            return shadeSurfaceOpaque(position, n, base);
        }

        float distA = length3(position - uHoleA_center);
        float distB = length3(position - uHoleB_center);
        bool insideWarp = (distA < uHoleA_radius) || (distB < uHoleB_radius);

        vec3 nextPos;
        if (!insideWarp) {
            float travel = min(dScene, maxSphereStep);
            if (travel < 1e-5) {
                travel = 1e-5;
            }
            nextPos = position + dir * travel;
        } else {
            vec3 accel = warpField(position);
            dir = normalize3(dir + accel * (stepLength * 0.85));
            nextPos = position + dir * stepLength;
        }

        float prevDistA = distA;
        float nextDistA = length3(nextPos - uHoleA_center);
        float prevDistB = distB;
        float nextDistB = length3(nextPos - uHoleB_center);

        if (prevDistA >= uHoleA_coreRadius && nextDistA < uHoleA_coreRadius) {
            nextPos = teleportToOppositeSide(nextPos,
                uHoleA_center, uHoleA_coreRadius,
                uHoleB_center, uHoleB_coreRadius, exitMargin);
        } else if (prevDistB >= uHoleB_coreRadius && nextDistB < uHoleB_coreRadius) {
            nextPos = teleportToOppositeSide(nextPos,
                uHoleB_center, uHoleB_coreRadius,
                uHoleA_center, uHoleA_coreRadius, exitMargin);
        }

        position = nextPos;

        if (length3(position - origin) > maxDist) {
            break;
        }
    }

    return skyColor(dir);
}

vec3 traceRay(vec3 origin, vec3 dirIn) {
    vec3 position = origin;
    vec3 dir = dirIn;
    float stepLength = 0.15;
    float maxSphereStep = 0.25;
    float maxDist = 75.0;
    float hitEps = 0.08;
    float warpHitDirEps = 0.02;
    float exitMargin = 0.04;
    float kOceanBias = 0.14;

    for (int i = 0; i < 420; i++) {
        float dScene = sdfScene(position);
        if (dScene < hitEps) {
            vec3 n = estimateNormal(position);
            float wDistA = length3(position - uHoleA_center);
            float wDistB = length3(position - uHoleB_center);
            bool insideWarp = (wDistA < uHoleA_radius) || (wDistB < uHoleB_radius);
            if (insideWarp) {
                float dn = dot(dir, n);
                if (dn >= -warpHitDirEps) {
                    position = position + dir * (stepLength * 0.35);
                    continue;
                }
            }
            vec3 base = sceneColorAt(position);
            if (isInfiniteFloorAt(position)) {
                vec3 d = normalize3(dir);
                vec3 refl = d - 2.0 * dot(d, n) * n;
                vec3 p2 = position + n * kOceanBias;
                vec3 reflected = traceRayOpaqueSegment(p2, refl);
                reflected = applyOceanTint(reflected);
                float NdotV = clampf(dot(n, -d), 0.0, 1.0);
                float F = 0.02 + 0.98 * pow(1.0 - NdotV, 4.0);
                float w = F * 0.68;
                vec3 baseLit = shadeSurfaceOpaque(position, n, base);
                return baseLit * (1.0 - w) + reflected * w;
            }
            return shadeSurfaceOpaque(position, n, base);
        }

        float distA = length3(position - uHoleA_center);
        float distB = length3(position - uHoleB_center);
        bool insideWarp = (distA < uHoleA_radius) || (distB < uHoleB_radius);

        vec3 nextPos;
        if (!insideWarp) {
            float travel = min(dScene, maxSphereStep);
            if (travel < 1e-5) {
                travel = 1e-5;
            }
            nextPos = position + dir * travel;
        } else {
            vec3 accel = warpField(position);
            dir = normalize3(dir + accel * (stepLength * 0.85));
            nextPos = position + dir * stepLength;
        }

        float prevDistA = distA;
        float nextDistA = length3(nextPos - uHoleA_center);
        float prevDistB = distB;
        float nextDistB = length3(nextPos - uHoleB_center);

        if (prevDistA >= uHoleA_coreRadius && nextDistA < uHoleA_coreRadius) {
            nextPos = teleportToOppositeSide(nextPos,
                uHoleA_center, uHoleA_coreRadius,
                uHoleB_center, uHoleB_coreRadius, exitMargin);
        } else if (prevDistB >= uHoleB_coreRadius && nextDistB < uHoleB_coreRadius) {
            nextPos = teleportToOppositeSide(nextPos,
                uHoleB_center, uHoleB_coreRadius,
                uHoleA_center, uHoleA_coreRadius, exitMargin);
        }

        position = nextPos;

        if (length3(position - origin) > maxDist) {
            break;
        }
    }

    return skyColor(dir);
}

void main() {
    float px = -((2.0 * (gl_FragCoord.x - 0.5) / uResolution.x - 1.0) * uAspect * uTanHalfFov);
    float py = -((-1.0 + 2.0 * (gl_FragCoord.y - 0.5) / uResolution.y) * uTanHalfFov);
    vec3 dir = normalize3(uRayForward + uRayRight * px + uRayUp * py);
    vec3 c = traceRay(uCamPos, dir);
    gl_FragColor = vec4(c, 1.0);
}
)GLSL";

static const char kVertRaycast[] = R"GLSL(#version 120
attribute vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)GLSL";

#endif
