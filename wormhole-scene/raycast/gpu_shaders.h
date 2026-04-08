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
uniform float uPointRange[32];
uniform vec3 uPointPos[32];
uniform vec3 uPointCol[32];

uniform vec3 uSunDir;
uniform float uSunDiffuse;
uniform float uAmbient;
uniform float uPointLightScale;
uniform float uSkyDayFactor;

// Texturas de superfície (modo GPU)
uniform sampler2D uTexBrick;    // red_brick_03_diff_1k.jpg  — prédios
uniform sampler2D uTexTerrain;  // Terrain002_2K_Color.jpg   — montanhas
uniform int uTexBrickLoaded;    // 1 se a textura foi carregada, 0 caso contrário
uniform int uTexTerrainLoaded;

uniform sampler2D uBvhTex;
uniform float uBvhTexWidth; // A largura da textura que gerámos no Passo 1

// Função para extrair um Nó da textura!
void getBvhNode(float nodeIndex, out vec3 minB, out vec3 maxB, out float leftChild, out float objCount) {
    // Onde estão os dois pixeis deste Nó na fita métrica?
    float texelIndex0 = nodeIndex * 2.0;
    float texelIndex1 = texelIndex0 + 1.0;

    // Converte o índice num UV (entre 0.0 e 1.0). O +0.5 garante que lemos o meio exato do pixel.
    vec2 uv0 = vec2((texelIndex0 + 0.5) / uBvhTexWidth, 0.5);
    vec2 uv1 = vec2((texelIndex1 + 0.5) / uBvhTexWidth, 0.5);

    // Lê os pixeis
    vec4 data0 = texture2D(uBvhTex, uv0);
    vec4 data1 = texture2D(uBvhTex, uv1);

    // Reconstrói a variável
    minB = data0.xyz;
    leftChild = data0.w;
    maxB = data1.xyz;
    objCount = data1.w;
}

bool traverseBVH(vec3 rayOrigin, vec3 rayDir, out float closestT, out float hitBoxIndex) {
    closestT = 999999.0;
    hitBoxIndex = -1.0;
    
    vec3 invRayDir = 1.0 / rayDir; // Para evitar divisão lenta no loop
    
    // O NOSSO "STACK" (Pilha)
    float stack[32]; // 32 níveis de profundidade é suficiente
    int stackPtr = 0;
    
    // Começa pelo Nó Raiz
    stack[0] = 0.0;
    stackPtr = 1;

    // CICLO SEGURO: A placa de vídeo não entra em pânico porque sabe que para no 150.
    for(int step = 0; step < 150; ++step) {
        if (stackPtr <= 0) break; // Se o stack estiver vazio, terminámos!

        // Tirar o último nó do topo
        stackPtr--;
        float nodeIdx = stack[stackPtr];

        vec3 minB, maxB;
        float leftChild, objCount;
        getBvhNode(nodeIdx, minB, maxB, leftChild, objCount);

        // O raio bateu na "Caixa Grande" desta zona do mapa?
        float hitNodeT = intersectAABB(rayOrigin, invRayDir, minB, maxB);
        
        // Se não bateu, ignora este ramo completamente
        if (hitNodeT < 0.0 || hitNodeT >= closestT) continue;

        if (objCount > 0.0) {
            // CHEGÁMOS A UMA FOLHA!
            // Aqui você faz o ciclo para testar os prédios que estão dentro deste Nó.
            // (Lê a textura 'uSceneDataTex' que você já usa atualmente, a partir do 'leftChild')
            
            float startIdx = leftChild;
            float endIdx = leftChild + objCount;
            for(float obj = startIdx; obj < endIdx; obj += 1.0) {
                // ... Seu código atual de interseção com a Caixa / Esfera ...
                // Se bater e for o mais próximo, atualiza o closestT e o hitBoxIndex.
            }
        } 
        else {
            // É UM RAMO: Tem filhos! 
            // Coloca os dois filhos na pilha para serem testados na próxima volta.
            // Nota: Se a árvore for construída ordenadamente, o filho direito é o esquerdo + 1
            stack[stackPtr] = leftChild;
            stackPtr++;
            stack[stackPtr] = leftChild + 1.0;
            stackPtr++;
        }
    }

    return hitBoxIndex != -1.0;
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
    for (int i = 0; i < 128; i++) {
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

// ─── Classificação de AABBs ────────────────────────────────────────────────────
bool isBuilding(vec3 halfSize) {
    return halfSize.x >= 1.0 && halfSize.x <= 3.0
        && halfSize.z >= 1.0 && halfSize.z <= 3.0
        && halfSize.y > 1.5;
}
bool isMountain(vec3 halfSize) {
    return halfSize.x > 5.0 && halfSize.y > 1.0;
}

// UV por face da AABB — posição local, detecta face pelo eixo dominante.
// tileM: quantos metros equivalem a 1 repetição UV.
vec2 aabb_uv(vec3 p, vec3 center, vec3 hs, float tileM) {
    vec3 lp = (p - center) / max(hs, vec3(0.001)); // [-1, 1]
    vec3 ap = abs(lp);
    float u, v;
    if (ap.x >= ap.y && ap.x >= ap.z) {
        // Face X
        u = (lp.z + 1.0) * 0.5 * (hs.z * 2.0 / tileM);
        v = (lp.y + 1.0) * 0.5 * (hs.y * 2.0 / tileM);
    } else if (ap.y >= ap.x && ap.y >= ap.z) {
        // Face Y (topo)
        u = (lp.x + 1.0) * 0.5 * (hs.x * 2.0 / tileM);
        v = (lp.z + 1.0) * 0.5 * (hs.z * 2.0 / tileM);
    } else {
        // Face Z
        u = (lp.x + 1.0) * 0.5 * (hs.x * 2.0 / tileM);
        v = (lp.y + 1.0) * 0.5 * (hs.y * 2.0 / tileM);
    }
    return vec2(u, v);
}

vec3 sceneColorAt(vec3 p) {
    float bestD = sdfFloor(p);
    vec3 color = vec3(0.35, 0.37, 0.61);
    for (int i = 0; i < 128; i++) {
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
            // Esfera — cor sólida
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
                vec3 solidColor = vec3(t1.a, t2.r, t2.g);
                // Aplica textura conforme tipo
                if (isBuilding(halfSize) && uTexBrickLoaded != 0) {
                    vec2 uv = aabb_uv(p, center, halfSize, 1.5);
                    color = texture2D(uTexBrick, uv).rgb;
                } else if (isMountain(halfSize) && uTexTerrainLoaded != 0) {
                    vec2 uv = aabb_uv(p, center, halfSize, 4.0);
                    color = texture2D(uTexTerrain, uv).rgb;
                } else {
                    color = solidColor;
                }
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

// Reflexo só no plano oceano (sdfFloor == sdfScene). Lajes finas: dScene≈0, df≈0,04 — não coincidir (ver kOceanFloorMatchEps em scene_prefabs.h).
bool isInfiniteFloorAt(vec3 p) {
    float df = sdfFloor(p);
    float dScene = sdfScene(p);
    return abs(df - dScene) < 0.001;
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

    for (int i = 0; i < 128; i++) {
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
            if (rad < 45.0) {
                float tS = raySphereMinT(o, sunDir, center, rad, kTMin);
                if (tS > 0.0 && tS < kMax) {
                    return 0.0;
                }
            }
        } else {
            vec3 halfSize = vec3(t1.r, t1.g, t1.b);
            float tB = rayAabbEnterT(o, sunDir, center, halfSize, kTMin, kMax);
            if (tB >= 0.0) {
                return 0.0;
            }
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

    for (int i = 0; i < 128; i++) {
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
            if (rad < 45.0) {
                float tS = raySphereMinT(o, Ld, center, rad, kTMin);
                if (tS > 0.0 && tS < maxT) {
                    return 0.0;
                }
            }
        } else {
            vec3 halfSize = vec3(t1.r, t1.g, t1.b);
            float tB = rayAabbEnterT(o, Ld, center, halfSize, kTMin, maxT);
            if (tB >= 0.0) {
                return 0.0;
            }
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
    for (int i = 0; i < 32; i++) {
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
        float dr = d / max(uPointRange[i], 1e-5);
        float atten = edge * edge / (1.0 + dr * dr);
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
