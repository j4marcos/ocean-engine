#include "scene_world.h"

#include "scene_entities.h"

#include "wormhole3d_globals.h"

#include <memory>

void sceneBuildCrossingQuarter() {
    gSpheres.clear();
    gBoxes.clear();
    gPointLights.clear();

    std::vector<std::unique_ptr<SceneEntity>> entities;

    const float halfW = 0.42f;

    const RGBA facadeA = {0.52f, 0.48f, 0.44f, 1.0f};
    const RGBA facadeB = {0.45f, 0.5f, 0.55f, 1.0f};
    const RGBA facadeC = {0.55f, 0.42f, 0.38f, 1.0f};
    const RGBA facadeD = {0.48f, 0.52f, 0.46f, 1.0f};

    // Centro do cruzamento (xz): wormhole A fica aqui em altura.
    const float cx = 0.0f;
    const float cz = -5.0f;

    // Oito prédios (dois por “lado” do cruz), alturas diferentes.
    entities.push_back(std::make_unique<BuildingPrefab>(Vec3{cx - 5.5f, 0.0f, cz - 0.5f}, halfW, 1.15f, facadeA));
    entities.push_back(std::make_unique<BuildingPrefab>(Vec3{cx - 5.5f, 0.0f, cz - 3.2f}, halfW, 0.85f, facadeB));
    entities.push_back(std::make_unique<BuildingPrefab>(Vec3{cx + 5.5f, 0.0f, cz - 0.5f}, halfW, 0.95f, facadeC));
    entities.push_back(std::make_unique<BuildingPrefab>(Vec3{cx + 5.5f, 0.0f, cz - 3.2f}, halfW, 1.25f, facadeD));

    entities.push_back(std::make_unique<BuildingPrefab>(Vec3{cx - 0.5f, 0.0f, cz - 8.2f}, halfW, 1.05f, facadeB));
    entities.push_back(std::make_unique<BuildingPrefab>(Vec3{cx + 0.5f, 0.0f, cz - 8.2f}, halfW, 0.75f, facadeA));
    entities.push_back(std::make_unique<BuildingPrefab>(Vec3{cx - 0.5f, 0.0f, cz + 1.2f}, halfW, 0.9f, facadeC));
    entities.push_back(std::make_unique<BuildingPrefab>(Vec3{cx + 0.5f, 0.0f, cz + 1.2f}, halfW, 1.1f, facadeD));

    const RGBA wood = {0.32f, 0.22f, 0.14f, 1.0f};
    const RGBA leaf = {0.18f, 0.48f, 0.22f, 1.0f};
    entities.push_back(std::make_unique<TreePrefab>(Vec3{cx - 7.0f, 0.0f, cz - 6.5f}, wood, leaf));
    entities.push_back(std::make_unique<TreePrefab>(Vec3{cx + 7.0f, 0.0f, cz - 6.5f}, wood, leaf));
    entities.push_back(std::make_unique<TreePrefab>(Vec3{cx - 7.0f, 0.0f, cz + 2.5f}, wood, leaf));
    entities.push_back(std::make_unique<TreePrefab>(Vec3{cx + 7.0f, 0.0f, cz + 2.5f}, wood, leaf));

    const RGBA poleCol = {0.28f, 0.28f, 0.3f, 1.0f};
    const RGBA lampCol = {0.98f, 0.92f, 0.45f, 1.0f};
    const float ph = 0.48f;
    const float pt = 0.05f;
    entities.push_back(std::make_unique<PostPrefab>(Vec3{cx - 2.4f, 0.0f, cz - 2.4f}, ph, pt, poleCol, lampCol));
    entities.push_back(std::make_unique<PostPrefab>(Vec3{cx + 2.4f, 0.0f, cz - 2.4f}, ph, pt, poleCol, lampCol));
    entities.push_back(std::make_unique<PostPrefab>(Vec3{cx - 2.4f, 0.0f, cz + 2.4f}, ph, pt, poleCol, lampCol));
    entities.push_back(std::make_unique<PostPrefab>(Vec3{cx + 2.4f, 0.0f, cz + 2.4f}, ph, pt, poleCol, lampCol));

    // Esferas decorativas (opcional, cores distintas).
    gSpheres.push_back({{-2.8f, -0.1f, cz - 4.5f}, 0.55f, {0.85f, 0.4f, 0.2f, 1.0f}});
    gSpheres.push_back({{2.5f, -0.15f, cz - 3.8f}, 0.45f, {0.25f, 0.72f, 0.92f, 1.0f}});

    for (const auto& e : entities) {
        e->emit(gSpheres, gBoxes);
    }

    gWormhole.holeA.center = {cx, 0.52f, cz};
    gWormhole.holeB.center = {2.2f, 11.0f, cz - 2.5f};

    const float lightY = postPointLightY(ph);
    const Vec3 lampRgb = {0.98f, 0.88f, 0.42f};
    const float lampRange = 9.0f;
    gPointLights.push_back({{cx - 2.4f, lightY, cz - 2.4f}, lampRgb, lampRange});
    gPointLights.push_back({{cx + 2.4f, lightY, cz - 2.4f}, lampRgb, lampRange});
    gPointLights.push_back({{cx - 2.4f, lightY, cz + 2.4f}, lampRgb, lampRange});
    gPointLights.push_back({{cx + 2.4f, lightY, cz + 2.4f}, {10.0f, 10.0f, 10.0f}, 10.0f});
}
