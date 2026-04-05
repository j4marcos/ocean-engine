#include "scene_moving.h"

#include "scene_entities.h"
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
