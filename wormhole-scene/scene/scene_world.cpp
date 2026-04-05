#include "scene_world.h"

#include "scene_prefabs.h"
#include "scene_moving.h"

#include "wormhole3d_globals.h"

#include <memory>

namespace {

// Centro aproximado da ilha 2; figuras em Bézier orbitam no plano XZ.
constexpr float kIsland2OrbitCz = -50.0f;
constexpr float kIsland2MovingY = 6.0f;

void sceneRegisterMovingBezierPaths() {
    const float y = kIsland2MovingY;
    const float z0 = kIsland2OrbitCz;

    gBezierMovingSpheres[0] = {
        {18.0f, y, z0},
        {18.0f, y + 2.5f, z0 - 16.0f},
        {-18.0f, y + 2.5f, z0 - 16.0f},
        {18.0f, y, z0},
    };
    gBezierMovingSpheres[1] = {
        {-18.0f, y, z0},
        {-18.0f, y + 2.8f, z0 + 16.0f},
        {18.0f, y + 2.8f, z0 + 16.0f},
        {-18.0f, y, z0},
    };
    gBezierMovingSpheres[2] = {
        {0.0f, y, z0 - 20.0f},
        {22.0f, y + 2.2f, z0 - 8.0f},
        {-22.0f, y + 2.2f, z0 - 8.0f},
        {0.0f, y, z0 - 20.0f},
    };
}

constexpr float kSlabH = 0.02f;
inline float slabCenterY() {
    return kSceneGroundY + kSlabH;
}

/** Árvores na areia (ilha 1): Z entre mar e rua (~−5…−2). Poucas instâncias para caber na textura GPU (`kSceneDataWidth`). */
constexpr float kBeachTreeXZ[][2] = {
    {-18.0f, -3.4f},
    {-8.0f, -4.1f},
    {3.0f, -3.2f},
    {14.0f, -4.2f},
    {20.0f, -3.5f},
};
/**
 * Ilha 2: agrupadas ao redor do centro XZ do buraco B (cx≈0, z≈kIsland2OrbitCz),
 * com raio pequeno para não parecerem “no mar aberto”.
 */
constexpr float kIsland2TreeXZ[][2] = {
    {-3.2f, -49.0f},
    {3.5f, -50.2f},
    {-1.8f, -51.0f},
    {2.2f, -48.5f},
    {0.4f, -50.8f},
};

} // namespace

void sceneBuild() {
    gSpheres.clear();
    gBoxes.clear();
    gPointLights.clear();

    sceneRegisterMovingBezierPaths();

    std::vector<std::unique_ptr<SceneEntity>> entities;

    const float cx = 0.0f;
    const float islandHalfX = 24.0f;
    const float streetHalfZ = 2.4f;

    // Eixo Z: frente = -Z (mar / buraco B ao longe); costas = +Z (rua A, prédios, montanhas).
    // Praia (areia) entre o mar e a rua; câmera inicial em gCamera fica na faixa de areia (~z -9).
    const float waterEdgeZ = -5.0f;
    const float beachInnerZ = -2.0f;
    const float sandHalfZ = (beachInnerZ - waterEdgeZ) * 0.5f;
    const float sandCenterZ = (waterEdgeZ + beachInnerZ) * 0.5f;

    const float streetCenterZ = beachInnerZ + streetHalfZ;
    gStreetCenterZ = streetCenterZ;

    {
        const float yCar = carAnchorYWorld();
        // Rua (asfalto): centro Z ≈ gStreetCenterZ; curva suave ao longo de X.
        gBezierCarBeach = {
            {-20.0f, yCar, kCarRoadPathCenterZ},
            {-6.5f, yCar, kCarRoadPathCenterZ - 0.12f},
            {6.5f, yCar, kCarRoadPathCenterZ + 0.12f},
            {20.0f, yCar, kCarRoadPathCenterZ},
        };
    }

    const RGBA sand = {0.88f, 0.80f, 0.62f, 1.0f};
    const RGBA asphalt = {0.20f, 0.20f, 0.22f, 1.0f};

    // Areia (praia): de frente para o mar / B.
    gBoxes.push_back({{cx, slabCenterY(), sandCenterZ}, {islandHalfX, kSlabH, sandHalfZ}, sand});

    // Rua com buraco A (atrás da praia, +Z em relação à areia).
    gBoxes.push_back({{cx, slabCenterY() + 0.002f, streetCenterZ}, {islandHalfX, kSlabH, streetHalfZ}, asphalt});

    // Montanhas atrás da fileira de prédios (+Z).
    const RGBA mtnRidge = {0.40f, 0.39f, 0.37f, 1.0f};
    const RGBA mtnPeak = {0.34f, 0.37f, 0.35f, 1.0f};
    gBoxes.push_back({{0.0f, kSceneGroundY + 3.0f, 21.5f}, {26.0f, 5.0f, 5.0f}, mtnRidge});
    gBoxes.push_back({{-17.0f, kSceneGroundY + 4.5f, 23.0f}, {11.0f, 6.5f, 5.5f}, mtnPeak});
    gBoxes.push_back({{17.0f, kSceneGroundY + 5.0f, 22.5f}, {10.0f, 4.0f, 5.0f}, mtnPeak});
    gBoxes.push_back({{0.0f, kSceneGroundY + 2.0f, 26.0f}, {14.0f, 7.0f, 3.5f}, mtnPeak});

    const RGBA facadeA = {0.52f, 0.48f, 0.44f, 1.0f};
    const RGBA facadeB = {0.45f, 0.5f, 0.55f, 1.0f};
    const RGBA facadeC = {0.55f, 0.42f, 0.38f, 1.0f};
    const RGBA facadeD = {0.48f, 0.52f, 0.46f, 1.0f};
    const RGBA* facades[] = {&facadeA, &facadeB, &facadeC, &facadeD};

    static constexpr int kNumBuildings = 11;
    static constexpr float kBX[kNumBuildings] = {
        -20.0f, -16.0f, -12.0f, -8.0f, -4.0f, 0.0f, 4.0f, 8.0f, 12.0f, 16.0f, 20.0f,
    };
    static constexpr float kBHalfW[kNumBuildings] = {
        1.35f, 1.5f, 1.4f, 1.65f, 1.45f, 1.7f, 1.4f, 1.5f, 1.45f, 1.55f, 1.35f,
    };
    static constexpr float kBH[kNumBuildings] = {
        4.0f, 6.5f, 5.0f, 9.0f, 5.5f, 8.0f, 4.5f, 7.0f, 5.8f, 6.8f, 4.2f,
    };

    const float buildingRowZ = 4.6f;
    const float postZ = 1.4f;

    const RGBA poleCol = {0.28f, 0.28f, 0.3f, 1.0f};
    const RGBA lampCol = {0.98f, 0.92f, 0.45f, 1.0f};
    const float ph = 1.0f;
    const float pt = 0.02f;

    for (int i = 0; i < kNumBuildings; ++i) {
        entities.push_back(std::make_unique<BuildingPrefab>(
            Vec3{kBX[i], 0.0f, buildingRowZ},
            kBHalfW[i],
            kBH[i],
            *facades[static_cast<size_t>(i) % 4]));
        entities.push_back(std::make_unique<PostPrefab>(Vec3{kBX[i], 0.0f, postZ}, ph, pt, poleCol, lampCol));
    }

    const RGBA treeTrunk = {0.32f, 0.2f, 0.12f, 1.0f};
    const RGBA treeLeaf = {0.18f, 0.42f, 0.16f, 1.0f};
    for (size_t ti = 0; ti < sizeof(kBeachTreeXZ) / sizeof(kBeachTreeXZ[0]); ++ti) {
        entities.push_back(std::make_unique<TreePrefab>(
            Vec3{kBeachTreeXZ[ti][0], 0.0f, kBeachTreeXZ[ti][1]}, treeTrunk, treeLeaf));
    }
    for (size_t ti = 0; ti < sizeof(kIsland2TreeXZ) / sizeof(kIsland2TreeXZ[0]); ++ti) {
        entities.push_back(std::make_unique<TreePrefab>(
            Vec3{kIsland2TreeXZ[ti][0], 0.0f, kIsland2TreeXZ[ti][1]}, treeTrunk, treeLeaf));
    }

    const RGBA boatHullCol = {0.44f, 0.45f, 0.47f, 1.0f};
    for (int b = 0; b < kNumBoats; ++b) {
        gBoatHullBoxIndex[static_cast<size_t>(b)] = static_cast<int>(gBoxes.size());
        const float hullCy = kBoatDeckTopY - kBoatHullHalfY;
        gBoxes.push_back(
            {{0.0f, hullCy, 0.0f}, {kBoatDeckHalfXZ, kBoatHullHalfY, kBoatDeckHalfXZ}, boatHullCol});
        entities.push_back(std::make_unique<DeckPostPrefab>(Vec3{0.0f, kBoatDeckTopY, 0.0f}, ph, pt, poleCol, lampCol));
    }

    gSpheres.push_back({{cx, -50.0f, -50.0f}, 50.0f, {0.85f, 0.4f, 0.2f, 1.0f}});

    for (const auto& e : entities) {
        e->emit(gSpheres, gBoxes);
    }

    {
        const int nbox = static_cast<int>(gBoxes.size());
        for (int b = 0; b < kNumBoats; ++b) {
            gBoatPoleBoxIndex[static_cast<size_t>(b)] = nbox - kNumBoats + b;
        }
    }
    {
        constexpr int kStreetLampSpheres = 11;
        for (int b = 0; b < kNumBoats; ++b) {
            gBoatBulbSphereIndex[static_cast<size_t>(b)] = 1 + kStreetLampSpheres + b;
        }
    }

    gWormhole.holeA.center = {cx, 0.3f, streetCenterZ};
    gWormhole.holeB.center = {cx, 15.0f, -50.0f};

    {
        const RGBA kLhTowerCol = {0.9f, 0.89f, 0.86f, 1.0f};
        for (int i = 0; i < kMovingBezierSphereCount; ++i) {
            gMovingBezierSphereIndex[static_cast<size_t>(i)] = static_cast<int>(gSpheres.size());
            gSpheres.push_back({{0.0f, 0.0f, 0.0f}, kMovingSphereRadius, kMovingSphereMaterial});
        }
        for (int i = 0; i < kMovingCarCount; ++i) {
            gCarRearBoxIndex[static_cast<size_t>(i)] = static_cast<int>(gBoxes.size());
            gBoxes.push_back({{0.0f, -500.0f, 0.0f}, {1e-3f, 1e-3f, 1e-3f}, kCarRearBoxColor});
            gCarFrontBoxIndex[static_cast<size_t>(i)] = static_cast<int>(gBoxes.size());
            gBoxes.push_back({{0.0f, -500.0f, 0.0f}, {1e-3f, 1e-3f, 1e-3f}, kCarFrontBoxColor});
        }
        gLighthouseTowerBoxIndex = static_cast<int>(gBoxes.size());
        gBoxes.push_back(
            {lighthouseTowerCenterWorld(),
             {kLighthouseTowerHalfXZ, kLighthouseTowerHalfY, kLighthouseTowerHalfXZ},
             kLhTowerCol});
        for (int p = 0; p < kLighthouseHeadPlateCount; ++p) {
            gLighthouseHeadPlateBoxIndex[static_cast<size_t>(p)] = static_cast<int>(gBoxes.size());
            gBoxes.push_back({{0.0f, 0.0f, 0.0f}, {1e-3f, 1e-3f, 1e-3f}, kLighthouseHeadPlateColor});
        }
    }

    const float lightY = postPointLightY(ph);
    const Vec3 lampRgb = {0.98f, 0.88f, 0.42f};
    const float lampRange = 11.0f;
    for (int i = 0; i < kNumBuildings; ++i) {
        gPointLights.push_back({{kBX[i], lightY, postZ}, lampRgb, lampRange});
    }
    for (int i = 0; i < kCarPointLightsTotal; ++i) {
        gPointLights.push_back({{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f});
    }
    for (int b = 0; b < kNumBoats; ++b) {
        const float boatLightY = postPointLightYFromBaseY(kBoatDeckTopY, ph);
        gPointLights.push_back({{0.0f, boatLightY, 0.0f}, kBoatMastLampRgb, kBoatMastLampRange});
    }
    // Farol: posição/cor em sceneUpdateDynamicElements (`kLighthouseBeam*` em scene_moving.h).
    gPointLights.push_back({{0.0f, 0.0f, 0.0f}, kLighthouseBeamRgb, kLighthouseBeamRange});

    boatsUpdateDynamicGeometry();
    syncDynamicPrimitivesToScene();
}
