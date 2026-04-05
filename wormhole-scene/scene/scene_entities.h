#pragma once

#include "wormhole3d_types.h"

#include <memory>
#include <vector>

// Nível do chão na SDF (SignedDistanceFloor).
inline constexpr float kSceneGroundY = -1.15f;

// Poste: esfera do lampião (SDF) e luz pontual — a luz fica acima do topo da esfera para não ficar dentro da malha.
inline constexpr float kPostBulbRadius = 0.09f;
inline constexpr float kPostBulbStemOffset = 0.07f;
inline constexpr float kPostLightClearanceAboveBulb = 0.03f;

inline float postBulbCenterY(float poleHalfHeight) {
    return kSceneGroundY + poleHalfHeight * 2.0f + kPostBulbStemOffset - kPostBulbRadius;
}

inline float postPointLightY(float poleHalfHeight) {
    return postBulbCenterY(poleHalfHeight) + kPostBulbRadius + kPostLightClearanceAboveBulb;
}

// Entidade abstrata: prefabs geram esferas e AABBs consumidos pelo raycast e raster.
class SceneEntity {
public:
    virtual ~SceneEntity() = default;
    virtual void emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const = 0;
};

// Prédio: mesmo footprint em XZ, altura variável (halfSize.y); cor = fachada aproximada.
class BuildingPrefab final : public SceneEntity {
public:
    BuildingPrefab(Vec3 centerXZ, float halfWidthXZ, float height, const RGBA& facade);

    void emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const override;

private:
    Vec3 center_;
    Vec3 halfSize_;
    RGBA color_;
};

// Poste: aste fina (AABB) + esfera de “luz” no topo (emissiva por cor).
class PostPrefab final : public SceneEntity {
public:
    PostPrefab(Vec3 baseXZ, float poleHalfHeight, float poleThickness, const RGBA& poleColor, const RGBA& lightColor);

    void emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const override;

private:
    Vec3 baseXZ_;
    float poleHalfH_;
    float thick_;
    RGBA poleColor_;
    RGBA lightColor_;
};

// Árvore: tronco (caixa) + três “discos” folha (AABBs muito finos nos planos XY, XZ, YZ).
class TreePrefab final : public SceneEntity {
public:
    TreePrefab(Vec3 baseXZ, const RGBA& trunkColor, const RGBA& leafColor);

    void emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const override;

private:
    Vec3 baseXZ_;
    RGBA trunkColor_;
    RGBA leafColor_;
};
