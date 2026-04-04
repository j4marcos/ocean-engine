#ifndef WORMHOLE3D_GPU_SHADERS_H
#define WORMHOLE3D_GPU_SHADERS_H

// GLSL 1.20 (OpenGL 2.1) — manter em sync com traceRay() em wormhole3d.cpp
static const char kVertRaycast[] = R"GLSL(#version 120
attribute vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)GLSL";

static const char kFragRaycast[] = R"GLSL(#version 120

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

const vec3 sphereCenters[3] = vec3[3](
    vec3(-2.2, -0.15, -6.2),
    vec3(0.1, -0.25, -5.6),
    vec3(2.4, 0.0, -7.5)
);
const float sphereRadii[3] = float[3](0.85, 0.75, 0.95);
const vec3 sphereColors[3] = vec3[3](
    vec3(0.85, 0.40, 0.20),
    vec3(0.25, 0.72, 0.92),
    vec3(0.95, 0.84, 0.28)
);

const vec3 boxCenters[3] = vec3[3](
    vec3(-0.9, -0.30, -3.9),
    vec3(1.1, -0.50, -6.4),
    vec3(3.1, -0.60, -10.0)
);
const vec3 boxHalf[3] = vec3[3](
    vec3(0.55, 0.55, 0.55),
    vec3(0.80, 0.35, 0.70),
    vec3(0.50, 0.25, 0.50)
);
const vec3 boxColors[3] = vec3[3](
    vec3(0.90, 0.30, 0.28),
    vec3(0.38, 0.88, 0.40),
    vec3(0.74, 0.74, 0.80)
);

float length3(vec3 v) { return length(v); }
vec3 normalize3(vec3 v) {
    float len = length(v);
    return len > 1e-6 ? v / len : vec3(0.0);
}
float clampf(float v, float lo, float hi) { return clamp(v, lo, hi); }

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

float sdfScene(vec3 p) {
    float d = sdfFloor(p);
    for (int i = 0; i < 3; i++) {
        d = min(d, sdfSphere(p, sphereCenters[i], sphereRadii[i]));
    }
    for (int i = 0; i < 3; i++) {
        d = min(d, sdfAabb(p, boxCenters[i], boxHalf[i]));
    }
    return d;
}

vec3 sceneColorAt(vec3 p) {
    float bestD = sdfFloor(p);
    vec3 color = vec3(0.35, 0.37, 0.41);
    for (int i = 0; i < 3; i++) {
        float d = sdfSphere(p, sphereCenters[i], sphereRadii[i]);
        if (d < bestD) {
            bestD = d;
            color = sphereColors[i];
        }
    }
    for (int i = 0; i < 3; i++) {
        float d = sdfAabb(p, boxCenters[i], boxHalf[i]);
        if (d < bestD) {
            bestD = d;
            color = boxColors[i];
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
    return vec3(
        0.03 + 0.20 * t,
        0.04 + 0.22 * t,
        0.07 + 0.34 * t
    );
}

vec3 traceRay(vec3 origin, vec3 dirIn) {
    vec3 position = origin;
    vec3 dir = dirIn;
    float stepLength = 0.15;
    float maxDist = 75.0;
    float hitEps = 0.08;
    float exitMargin = 0.04;

    for (int i = 0; i < 420; i++) {
        vec3 accel = warpField(position);
        dir = normalize3(dir + accel * (stepLength * 0.85));

        vec3 nextPos = position + dir * stepLength;

        float prevDistA = length3(position - uHoleA_center);
        float nextDistA = length3(nextPos - uHoleA_center);
        float prevDistB = length3(position - uHoleB_center);
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

        float d = sdfScene(position);
        if (d < hitEps) {
            vec3 n = estimateNormal(position);
            vec3 lightDir = normalize3(vec3(0.62, 0.74, 0.23));
            float lambert = clampf(dot(n, lightDir), 0.0, 1.0);
            vec3 base = sceneColorAt(position);
            float amb = 0.22;
            float shade = amb + lambert * 0.78;
            return base * shade;
        }

        if (length3(position - origin) > maxDist) {
            break;
        }
    }

    return skyColor(dir);
}

void main() {
    // NDC com origem em baixo-esquerda; o par (-px,-py) alinha ao frustum gluPerspective + gluLookAt (evita 180° vs raster).
    float px = -((2.0 * (gl_FragCoord.x - 0.5) / uResolution.x - 1.0) * uAspect * uTanHalfFov);
    float py = -((-1.0 + 2.0 * (gl_FragCoord.y - 0.5) / uResolution.y) * uTanHalfFov);
    vec3 dir = normalize3(uRayForward + uRayRight * px + uRayUp * py);
    vec3 c = traceRay(uCamPos, dir);
    gl_FragColor = vec4(c, 1.0);
}
)GLSL";

#endif
