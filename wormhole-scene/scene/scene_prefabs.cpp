#include "scene_prefabs.h"

#include <algorithm>

BuildingPrefab::BuildingPrefab(const Vec3 centerXZ, const float halfWidthXZ, const float height, const RGBA& facade)
    : center_{centerXZ.x, kSceneGroundY + height * 0.5f, centerXZ.z},
      halfSize_{halfWidthXZ, height * 0.5f, halfWidthXZ},
      color_{facade} {}

void BuildingPrefab::emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const {
    (void)spheres;
    boxes.push_back({center_, halfSize_, color_});
}

// poste de madeira na praia
PostPrefab::PostPrefab(
    const Vec3 baseXZ,
    const float poleHalfHeight,
    const float poleThickness,
    const RGBA& poleColor,
    const RGBA& lightColor
)
    : baseXZ_{baseXZ},
      poleHalfH_{poleHalfHeight},
      thick_{poleThickness},
      poleColor_{poleColor},
      lightColor_{lightColor} {}

void PostPrefab::emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const {
    const Vec3 poleCenter = {baseXZ_.x, kSceneGroundY + poleHalfH_, baseXZ_.z};
    boxes.push_back({poleCenter, {thick_, poleHalfH_, thick_}, poleColor_});

    const float bulbCy = postBulbCenterY(poleHalfH_);
    spheres.push_back({{baseXZ_.x, bulbCy, baseXZ_.z}, kPostBulbRadius, lightColor_});
}

// poste de madeira no convés de um barco
DeckPostPrefab::DeckPostPrefab(
    const Vec3 baseXZY,
    const float poleHalfHeight,
    const float poleThickness,
    const RGBA& poleColor,
    const RGBA& lightColor
)
    : base_{baseXZY},
      poleHalfH_{poleHalfHeight},
      thick_{poleThickness},
      poleColor_{poleColor},
      lightColor_{lightColor} {}

void DeckPostPrefab::emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const {
    const Vec3 poleCenter = {base_.x, base_.y + poleHalfH_, base_.z};
    boxes.push_back({poleCenter, {thick_, poleHalfH_, thick_}, poleColor_});

    const float bulbCy = postBulbCenterYFromBaseY(base_.y, poleHalfH_);
    spheres.push_back({{base_.x, bulbCy, base_.z}, kPostBulbRadius, lightColor_});
}

TreePrefab::TreePrefab(const Vec3 baseXZ, const RGBA& trunkColor, const RGBA& leafColor)
    : baseXZ_{baseXZ}, trunkColor_{trunkColor}, leafColor_{leafColor} {}

void TreePrefab::emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const {
    (void)spheres;
    const float trunkHalfH = 0.38f;
    const Vec3 trunkCenter = {baseXZ_.x, kSceneGroundY + trunkHalfH, baseXZ_.z};
    boxes.push_back({trunkCenter, {0.11f, trunkHalfH, 0.11f}, trunkColor_});

    const float topY = kSceneGroundY + trunkHalfH * 2.0f + 0.22f;
    const Vec3 foliage = {baseXZ_.x, topY, baseXZ_.z};
    const float r = 0.32f;
    const float t = 0.03f;
    boxes.push_back({foliage, {r, t, r}, leafColor_});
    boxes.push_back({foliage, {t, r, r}, leafColor_});
    boxes.push_back({foliage, {r, r, t}, leafColor_});
}
