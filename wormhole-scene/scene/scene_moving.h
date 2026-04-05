#pragma once

#include "scene_prefabs.h"
#include "wormhole3d_types.h"

#include <cmath>

/** Esferas em curvas de Bézier (definidas em scene_world). */
inline constexpr int kMovingBezierSphereCount = 3;
/** Carros: cubos que se movem na rua. */
inline constexpr int kMovingCarCount = 3;

inline constexpr float kMovingSphereRadius = 0.09f;
inline constexpr RGBA kMovingSphereMaterial = {1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr RGBA kMovingCarMaterial = {0.92f, 0.12f, 0.1f, 1.0f};
/** Corpo traseiro (mais estreito — leitura em planta). */
inline constexpr float kCarRearHalfX = 0.14f;
inline constexpr float kCarRearHalfY = 0.18f;
inline constexpr float kCarRearHalfZ = 0.22f;
/** Frente alongada (mais fina em Z que antes). */
inline constexpr float kCarFrontHalfX = 0.075f;
inline constexpr float kCarFrontHalfY = 0.15f;
inline constexpr float kCarFrontHalfZ = 0.26f;
/** Distância do `anchor` ao centro de cada caixa ao longo do movimento (±X). */
inline constexpr float kCarHalfToRearCenter = 0.14f;
inline constexpr float kCarHalfToFrontCenter = 0.12f;
inline constexpr float kCarHeadlightOut = 0.07f;
inline constexpr float kCarTaillightOut = 0.055f;

/** Faróis e lanternas — mesmo bloco lógico que o prefab do carro. */
inline constexpr Vec3 kCarHeadlightRgb = {2.4f, 2.4f, 2.2f};
inline constexpr float kCarHeadlightRange = 3.0f;
inline constexpr Vec3 kCarTaillightRgb = {2.0f, 0.1f, 0.08f};
inline constexpr float kCarTaillightRange = 2.5f;

/** Luz do farol (torre na ilha distante): declarada aqui com a geometria do farol. */
inline constexpr Vec3 kLighthouseBeamRgb = {2.0f, 2.0f, 1.0f};
/**
 * Alcance usado na atenuação (`edge = 1 - d/(range*2)` → zero se d ≥ 2×range).
 * Valores ~2 cortam a luz para tudo que esteja a mais de ~4 unidades — o farol fica longe (buraco B),
 * por isso precisa de alcance grande para a ilha ainda receber luz.
 */
inline constexpr float kLighthouseBeamRange = 100.0f;

/** Barcos no canal entre a ilha da cidade e a ilha 2 (uma luz por barco, mais forte que os postes da rua). */
inline constexpr int kNumBoats = 3;
inline constexpr Vec3 kBoatMastLampRgb = {1.55f, 1.38f, 0.62f};
inline constexpr float kBoatMastLampRange = 26.0f;
/** Dimensões do casco (partilhado com `scene_world` / atualização dinâmica). */
inline constexpr float kBoatDeckHalfXZ = 1.28f;
inline constexpr float kBoatHullHalfY = 0.14f;
inline constexpr float kBoatDeckTopY = kSceneGroundY + 0.34f;
/** Órbita em torno do farol (XZ do buraco B) + deriva e bob. */
inline constexpr float kBoatOrbitRadius = 29.0f;
inline constexpr float kBoatOrbitSpeed = 0.062f;
inline constexpr float kBoatDriftAmp = 1.15f;
/** rad/s efetivos na deriva (oscilações X/Z sobre a órbita). */
inline constexpr float kBoatDriftFreq = 2.1f;
inline constexpr float kBoatBobAmp = 0.0f;
inline constexpr float kBoatBobFreq = 0.48f;
inline constexpr float kBoatYawTiltAmp = 0.38f;
inline constexpr float kBoatYawFreq = 0.55f;

/** Praia: período de uma ida (ponta→ponta); easing deixa o movimento mais lento no meio do percurso. */
inline constexpr float kCarBeachLapSec = 22.0f;
inline constexpr float kCarBeachEaseMiddle = 0.38f;
/**
 * Z da linha média da curva na rua (asfalto); alinhar com `gStreetCenterZ` (~beachInnerZ + streetHalfZ).
 * Faixas paralelas dentro da largura da rua.
 */
inline constexpr float kCarRoadPathCenterZ = 0.4f;
inline constexpr float kCarLaneZ[kMovingCarCount] = {0.05f, 0.4f, 0.75f};

/** Estado rígido do carro: tangente no plano XZ; traseira/frente alinhadas ao movimento. */
struct CarRigidState {
    Vec3 anchor;
    float forwardX;
    float forwardZ;
};

/** Altura Y do ponto de referência do carro (solo + meia-altura máxima). */
float carAnchorYWorld();

/** `movingCarsCompute`: centro de referência no solo (x, z da rua). */
void carRigidState(int carIndex, const Vec3& anchor, CarRigidState& out);
Vec3 carRearBoxCenterWorld(const CarRigidState& s);
Vec3 carFrontBoxCenterWorld(const CarRigidState& s);
/** Compat: recalcula o estado internamente (evite em laços apertados). */
Vec3 carRearBoxCenterWorld(int carIndex, const Vec3& anchor);
Vec3 carFrontBoxCenterWorld(int carIndex, const Vec3& anchor);

float carForwardXWorld(int carIndex);

/** Preenche as 4 luzes a partir de um estado já calculado (evita segunda amostragem da curva). */
void carWritePointLights(const CarRigidState& st, PointLight out[4]);
/** Compat: recalcula o estado com `carRigidState` (preferir a sobrecarga com `CarRigidState` em laços). */
void carWritePointLights(int carIndex, const Vec3& anchor, PointLight out[4]);

/** Postes na rua (`scene_world`); tem de coincidir com `kNumBuildings`. */
inline constexpr int kNumStreetLamps = 11;
inline constexpr int kCarLightsPerVehicle = 4;
inline constexpr int kCarPointLightsTotal = kMovingCarCount * kCarLightsPerVehicle;
inline constexpr int kBoatPointLightsTotal = kNumBoats;
inline constexpr int kIdxFirstCarPointLight = kNumStreetLamps;
inline constexpr int kIdxFirstBoatPointLight = kNumStreetLamps + kCarPointLightsTotal;
inline constexpr int kIdxLighthousePointLight = kIdxFirstBoatPointLight + kBoatPointLightsTotal;

// Farol: centro (XZ) = gWormhole.holeB.center (vertical sob o buraco B).
inline constexpr float kLighthouseTowerHalfY = 3.2f;
inline constexpr float kLighthouseHeadHalf = 1.15f;
/** Base da torre: mesma largura/profundidade que a cabeça (cubo). */
inline constexpr float kLighthouseTowerHalfXZ = kLighthouseHeadHalf;
inline constexpr float kLighthouseFaceThickness = 0.09f;
inline constexpr float kLighthouseYawSpeed = 0.42f;
/** Faces da cabeça (cubo oco; +X aberto para a luz). */
inline constexpr int kLighthouseHeadPlateCount = 5;
inline constexpr RGBA kLighthouseHeadPlateColor = {0.72f, 0.78f, 0.86f, 1.0f};
inline constexpr RGBA kCarRearBoxColor = {0.9f, 0.13f, 0.11f, 1.0f};
inline constexpr RGBA kCarFrontBoxColor = {0.92f, 0.12f, 0.1f, 1.0f};

/** Preenchido em sceneBuild — centro Z da rua (carros). */
extern float gStreetCenterZ;

void sceneUpdateDynamicElements();
void boatsUpdateDynamicGeometry();
/** Atualiza centros das esferas Bézier, AABBs dos carros e do farol (torre + placas em mundo). */
void syncDynamicPrimitivesToScene();

Vec3 lighthouseTowerCenterWorld();
Vec3 lighthouseHeadCenterWorld();
float lighthouseYawRad();
