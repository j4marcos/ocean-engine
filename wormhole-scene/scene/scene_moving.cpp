#include "scene_moving.h"

#include "scene_prefabs.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"

#include <algorithm>
#include <cmath>

float gStreetCenterZ = 0.4f;

float carAnchorYWorld() {
    return kSceneGroundY + std::max(kCarRearHalfY, kCarFrontHalfY);
}

void carRigidState(const int carIndex, const Vec3&, CarRigidState& out) {
    carBeachMotionSample(carIndex, out.anchor, out.forwardX, out.forwardZ);
}

float carForwardXWorld(const int carIndex) {
    Vec3 a;
    float tx = 0.0f;
    float tz = 0.0f;
    carBeachMotionSample(carIndex, a, tx, tz);
    return tx;
}

Vec3 carRearBoxCenterWorld(const CarRigidState& s) {
    return {
        s.anchor.x - s.forwardX * kCarHalfToRearCenter,
        kSceneGroundY + kCarRearHalfY,
        s.anchor.z - s.forwardZ * kCarHalfToRearCenter,
    };
}

Vec3 carFrontBoxCenterWorld(const CarRigidState& s) {
    return {
        s.anchor.x + s.forwardX * kCarHalfToFrontCenter,
        kSceneGroundY + kCarFrontHalfY,
        s.anchor.z + s.forwardZ * kCarHalfToFrontCenter,
    };
}

Vec3 carRearBoxCenterWorld(const int carIndex, const Vec3& anchor) {
    CarRigidState st;
    carRigidState(carIndex, anchor, st);
    return carRearBoxCenterWorld(st);
}

Vec3 carFrontBoxCenterWorld(const int carIndex, const Vec3& anchor) {
    CarRigidState st;
    carRigidState(carIndex, anchor, st);
    return carFrontBoxCenterWorld(st);
}

void carWritePointLights(const CarRigidState& st, PointLight out[4]) {
    const float px = -st.forwardZ;
    const float pz = st.forwardX;
    const Vec3 fc = carFrontBoxCenterWorld(st);
    const Vec3 rc = carRearBoxCenterWorld(st);
    const float zPairFront = kCarFrontHalfZ * 0.4f;
    const float zPairRear = kCarRearHalfZ * 0.4f;
    const float headAlong = kCarFrontHalfX + kCarHeadlightOut;
    const float headX = fc.x + st.forwardX * headAlong;
    const float headZ = fc.z + st.forwardZ * headAlong;
    const float tailAlong = kCarRearHalfX + kCarTaillightOut;
    const float tailX = rc.x - st.forwardX * tailAlong;
    const float tailZ = rc.z - st.forwardZ * tailAlong;
    out[0] = {{headX + px * zPairFront, fc.y, headZ + pz * zPairFront}, kCarHeadlightRgb, kCarHeadlightRange};
    out[1] = {{headX - px * zPairFront, fc.y, headZ - pz * zPairFront}, kCarHeadlightRgb, kCarHeadlightRange};
    out[2] = {{tailX + px * zPairRear, rc.y, tailZ + pz * zPairRear}, kCarTaillightRgb, kCarTaillightRange};
    out[3] = {{tailX - px * zPairRear, rc.y, tailZ - pz * zPairRear}, kCarTaillightRgb, kCarTaillightRange};
}

void carWritePointLights(const int carIndex, const Vec3& anchor, PointLight out[4]) {
    (void)anchor;
    CarRigidState st{};
    carRigidState(carIndex, anchor, st);
    carWritePointLights(st, out);
}

Vec3 lighthouseTowerCenterWorld() {
    const Vec3& b = gWormhole.holeB.center;
    return {b.x, kSceneGroundY + kLighthouseTowerHalfY, b.z};
}

Vec3 lighthouseHeadCenterWorld() {
    const Vec3& b = gWormhole.holeB.center;
    const float towerTopY = kSceneGroundY + 2.0f * kLighthouseTowerHalfY;
    return {b.x, towerTopY + kLighthouseHeadHalf, b.z};
}

float lighthouseYawRad() {
    return gSceneTimeSec * kLighthouseYawSpeed;
}

namespace {

/** R^T: vetor no espaço local da cabeça → deslocamento em mundo (p = hc + R^T * pl). */
void localDeltaToWorld(const float yaw, const Vec3& pl, Vec3& out) {
    const float c = std::cos(-yaw);
    const float s = std::sin(-yaw);
    out.x = c * pl.x - s * pl.z;
    out.y = pl.y;
    out.z = s * pl.x + c * pl.z;
}

/** Placa (AABB no espaço local) → AABB em eixos mundo (envoltória da rotação em Y). */
void lighthousePlateWorldAabb(const Vec3& hc, const float yaw, const Vec3& lc, const Vec3& half, Aabb& out) {
    float minx = 1e30f;
    float miny = 1e30f;
    float minz = 1e30f;
    float maxx = -1e30f;
    float maxy = -1e30f;
    float maxz = -1e30f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sy = -1; sy <= 1; sy += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                const Vec3 pl = {
                    lc.x + static_cast<float>(sx) * half.x,
                    lc.y + static_cast<float>(sy) * half.y,
                    lc.z + static_cast<float>(sz) * half.z,
                };
                Vec3 w{};
                localDeltaToWorld(yaw, pl, w);
                w.x += hc.x;
                w.y += hc.y;
                w.z += hc.z;
                minx = std::min(minx, w.x);
                miny = std::min(miny, w.y);
                minz = std::min(minz, w.z);
                maxx = std::max(maxx, w.x);
                maxy = std::max(maxy, w.y);
                maxz = std::max(maxz, w.z);
            }
        }
    }
    out.center = {(minx + maxx) * 0.5f, (miny + maxy) * 0.5f, (minz + maxz) * 0.5f};
    out.halfSize = {(maxx - minx) * 0.5f, (maxy - miny) * 0.5f, (maxz - minz) * 0.5f};
}

} // namespace

void syncDynamicPrimitivesToScene() {
    Vec3 bezSph[3];
    movingBezierSpheresCompute(bezSph);
    for (int i = 0; i < kMovingBezierSphereCount; ++i) {
        const int ix = gMovingBezierSphereIndex[static_cast<size_t>(i)];
        if (ix >= 0 && static_cast<size_t>(ix) < gSpheres.size()) {
            Sphere& s = gSpheres[static_cast<size_t>(ix)];
            s.center = bezSph[static_cast<size_t>(i)];
            s.radius = kMovingSphereRadius;
            s.color = kMovingSphereMaterial;
        }
    }

    static constexpr Vec3 kHideCenter = {0.0f, -500.0f, 0.0f};
    static constexpr Vec3 kHideHalf = {1e-3f, 1e-3f, 1e-3f};

    if (!gSceneVehiclesEnabled) {
        for (int ci = 0; ci < kMovingCarCount; ++ci) {
            const int ir = gCarRearBoxIndex[static_cast<size_t>(ci)];
            const int iff = gCarFrontBoxIndex[static_cast<size_t>(ci)];
            if (ir >= 0 && static_cast<size_t>(ir) < gBoxes.size()) {
                gBoxes[static_cast<size_t>(ir)].center = kHideCenter;
                gBoxes[static_cast<size_t>(ir)].halfSize = kHideHalf;
            }
            if (iff >= 0 && static_cast<size_t>(iff) < gBoxes.size()) {
                gBoxes[static_cast<size_t>(iff)].center = kHideCenter;
                gBoxes[static_cast<size_t>(iff)].halfSize = kHideHalf;
            }
        }
    } else {
        Vec3 anchor[3];
        movingCarsCompute(anchor);
        for (int ci = 0; ci < kMovingCarCount; ++ci) {
            CarRigidState st{};
            carRigidState(ci, anchor[static_cast<size_t>(ci)], st);
            const Vec3 r = carRearBoxCenterWorld(st);
            const Vec3 f = carFrontBoxCenterWorld(st);
            const int ir = gCarRearBoxIndex[static_cast<size_t>(ci)];
            const int iff = gCarFrontBoxIndex[static_cast<size_t>(ci)];
            if (ir >= 0 && static_cast<size_t>(ir) < gBoxes.size()) {
                Aabb& br = gBoxes[static_cast<size_t>(ir)];
                br.center = r;
                br.halfSize = {kCarRearHalfX, kCarRearHalfY, kCarRearHalfZ};
                br.color = kCarRearBoxColor;
            }
            if (iff >= 0 && static_cast<size_t>(iff) < gBoxes.size()) {
                Aabb& bf = gBoxes[static_cast<size_t>(iff)];
                bf.center = f;
                bf.halfSize = {kCarFrontHalfX, kCarFrontHalfY, kCarFrontHalfZ};
                bf.color = kCarFrontBoxColor;
            }
        }
    }

    const Vec3 tc = lighthouseTowerCenterWorld();
    const Vec3 hc = lighthouseHeadCenterWorld();
    const float yaw = lighthouseYawRad();
    const RGBA kLhTowerCol = {0.9f, 0.89f, 0.86f, 1.0f};

    if (gLighthouseTowerBoxIndex >= 0 && static_cast<size_t>(gLighthouseTowerBoxIndex) < gBoxes.size()) {
        Aabb& tb = gBoxes[static_cast<size_t>(gLighthouseTowerBoxIndex)];
        tb.center = tc;
        tb.halfSize = {kLighthouseTowerHalfXZ, kLighthouseTowerHalfY, kLighthouseTowerHalfXZ};
        tb.color = kLhTowerCol;
    }

    const float h = kLighthouseHeadHalf;
    const float t = kLighthouseFaceThickness;
    const struct {
        Vec3 c;
        Vec3 halfSize;
    } plates[] = {
        {{-h, 0.0f, 0.0f}, {t, h, h}},
        {{0.0f, 0.0f, h}, {h, h, t}},
        {{0.0f, 0.0f, -h}, {h, h, t}},
        {{0.0f, h, 0.0f}, {h, t, h}},
        {{0.0f, -h, 0.0f}, {h, t, h}},
    };
    for (int p = 0; p < kLighthouseHeadPlateCount; ++p) {
        const int ib = gLighthouseHeadPlateBoxIndex[static_cast<size_t>(p)];
        if (ib < 0 || static_cast<size_t>(ib) >= gBoxes.size()) {
            continue;
        }
        Aabb& box = gBoxes[static_cast<size_t>(ib)];
        lighthousePlateWorldAabb(hc, yaw, plates[static_cast<size_t>(p)].c, plates[static_cast<size_t>(p)].halfSize, box);
        box.color = kLighthouseHeadPlateColor;
    }
}

void boatsUpdateDynamicGeometry() {
    constexpr float ph = 1.0f;
    if (!gSceneVehiclesEnabled) {
        constexpr float hx = 0.0f;
        constexpr float hz = 0.0f;
        constexpr float deckY = -400.0f;
        const float hullCy = deckY - kBoatHullHalfY;
        for (int b = 0; b < kNumBoats; ++b) {
            if (gBoatHullBoxIndex[static_cast<size_t>(b)] < 0) {
                continue;
            }
            gBoxes[static_cast<size_t>(gBoatHullBoxIndex[static_cast<size_t>(b)])].center = {hx, hullCy, hz};
            gBoxes[static_cast<size_t>(gBoatPoleBoxIndex[static_cast<size_t>(b)])].center = {hx, deckY + ph, hz};
            const float bulbCy = postBulbCenterYFromBaseY(deckY, ph);
            gSpheres[static_cast<size_t>(gBoatBulbSphereIndex[static_cast<size_t>(b)])].center = {hx, bulbCy, hz};
            const size_t li = static_cast<size_t>(kIdxFirstBoatPointLight + b);
            if (gPointLights.size() > li) {
                gPointLights[li].range = 0.0f;
            }
        }
        return;
    }

    constexpr float kTwoPiOver3 = 2.0f * 3.14159265359f / 3.0f;
    const Vec3& hb = gWormhole.holeB.center;
    const float cx = hb.x;
    const float cz = hb.z;
    const float t = gSceneTimeSec;
    const float orbitAng = kBoatOrbitSpeed * t;
    const float driftT = kBoatDriftFreq * t;
    const float yawT = kBoatYawFreq * t;
    const float bobT = kBoatBobFreq * t;

    for (int b = 0; b < kNumBoats; ++b) {
        if (gBoatHullBoxIndex[static_cast<size_t>(b)] < 0) {
            continue;
        }
        const float bf = static_cast<float>(b);
        const float phase = bf * kTwoPiOver3;
        float ang = phase + orbitAng;
        float x = cx + kBoatOrbitRadius * std::cos(ang);
        float z = cz + kBoatOrbitRadius * std::sin(ang);
        x += kBoatDriftAmp * std::sin(driftT + bf * 1.9f);
        x += kBoatDriftAmp * 0.42f * std::sin(driftT * 1.65f + bf * 2.7f);
        z += kBoatDriftAmp * 0.88f * std::cos(driftT * 0.82f + bf * 2.3f);
        z += kBoatDriftAmp * 0.38f * std::cos(driftT * 1.4f + bf * 1.1f);
        const float bob = kBoatBobAmp * std::sin(bobT + bf * 0.7f);
        const float deckY = kBoatDeckTopY + bob;
        const float hullCy = deckY - kBoatHullHalfY;
        const float tiltX = kBoatYawTiltAmp * std::sin(yawT + bf * 2.0f);
        const float tiltZ = kBoatYawTiltAmp * 0.92f * std::cos(yawT * 0.86f + bf * 1.4f);
        const float tiltX2 = kBoatYawTiltAmp * 0.55f * std::sin(yawT * 1.55f + bf * 0.6f);
        const float tiltZ2 = kBoatYawTiltAmp * 0.5f * std::cos(yawT * 1.35f + bf * 2.2f);

        const float mx = tiltX + tiltX2;
        const float mz = tiltZ + tiltZ2;
        gBoxes[static_cast<size_t>(gBoatHullBoxIndex[static_cast<size_t>(b)])].center = {x, hullCy, z};
        gBoxes[static_cast<size_t>(gBoatPoleBoxIndex[static_cast<size_t>(b)])].center = {x + mx, deckY + ph, z + mz};
        const float bulbCy = postBulbCenterYFromBaseY(deckY, ph);
        gSpheres[static_cast<size_t>(gBoatBulbSphereIndex[static_cast<size_t>(b)])].center = {x + mx, bulbCy, z + mz};

        const size_t li = static_cast<size_t>(kIdxFirstBoatPointLight + b);
        if (gPointLights.size() > li) {
            const float ly = postPointLightYFromBaseY(deckY, ph);
            gPointLights[li].position = {x + mx, ly, z + mz};
            gPointLights[li].range = kBoatMastLampRange;
        }
    }
}

void sceneUpdateDynamicElements() {
    if (gPointLights.size() <= static_cast<size_t>(kIdxLighthousePointLight)) {
        return;
    }

    boatsUpdateDynamicGeometry();
    syncDynamicPrimitivesToScene();

    if (!gSceneVehiclesEnabled) {
        size_t li = static_cast<size_t>(kIdxFirstCarPointLight);
        for (int ci = 0; ci < kMovingCarCount; ++ci) {
            for (int j = 0; j < 4; ++j) {
                gPointLights[li].range = 0.0f;
                li++;
            }
        }
    } else {
        CarRigidState carStates[kMovingCarCount];
        for (int ci = 0; ci < kMovingCarCount; ++ci) {
            carBeachMotionSample(ci, carStates[ci].anchor, carStates[ci].forwardX, carStates[ci].forwardZ);
        }

        size_t li = static_cast<size_t>(kIdxFirstCarPointLight);
        for (int ci = 0; ci < kMovingCarCount; ++ci) {
            PointLight block[4];
            carWritePointLights(carStates[ci], block);
            for (int j = 0; j < 4; ++j) {
                gPointLights[li++] = block[j];
            }
        }
    }

    PointLight& beam = gPointLights[static_cast<size_t>(kIdxLighthousePointLight)];
    beam.position = lighthouseHeadCenterWorld();
    beam.color = kLighthouseBeamRgb;
    beam.range = kLighthouseBeamRange;
}
