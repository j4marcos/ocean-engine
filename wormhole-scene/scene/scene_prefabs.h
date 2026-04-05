#pragma once

#include "wormhole3d_types.h"

#include <memory>
#include <vector>

// Nível do chão na SDF (SignedDistanceFloor) e no raster (plano).
inline constexpr float kSceneGroundY = -1.15f;

inline constexpr RGBA kSceneFloorMaterial = {0.35f, 0.37f, 0.61f, 1.0f};

/** Plano infinito (oceano): raio reflete e a cor do 2.º hit é tingida de azul — só no raycast (CPU/GPU). */
inline constexpr Vec3 kOceanReflectionTint = {0.78f, 0.92f, 1.05f};
inline constexpr float kOceanFresnelBase = 0.02f;
inline constexpr float kOceanFresnelMix = 0.68f;
inline constexpr float kOceanFresnelPower = 4.0f;
/** Offset ao longo da normal para o raio refletido não re-acertar o plano. */
inline constexpr float kOceanReflectBias = 0.14f;
/** Só aplica reflexo de “água” quando o SDF do plano coincide com o da cena (evita lajes finas tipo areia). */
inline constexpr float kOceanFloorMatchEps = 0.001f;
inline constexpr float kSceneBirdRadius = 0.09f;
inline constexpr RGBA kSceneBirdMaterial = {1.0f, 1.0f, 1.0f, 1.0f};

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

/** Base do poste no solo (kSceneGroundY); bulbo e luz — mesmo critério que `PostPrefab`. */
inline float postBulbCenterYFromBaseY(float baseY, float poleHalfHeight) {
    return baseY + poleHalfHeight * 2.0f + kPostBulbStemOffset - kPostBulbRadius;
}

inline float postPointLightYFromBaseY(float baseY, float poleHalfHeight) {
    return postBulbCenterYFromBaseY(baseY, poleHalfHeight) + kPostBulbRadius + kPostLightClearanceAboveBulb;
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

/** Poste com base no plano Y dado (ex.: convés): `baseXZY` é o pé do poste no convés (x, y_topo, z). */
class DeckPostPrefab final : public SceneEntity {
public:
    DeckPostPrefab(
        Vec3 baseXZY,
        float poleHalfHeight,
        float poleThickness,
        const RGBA& poleColor,
        const RGBA& lightColor);

    void emit(std::vector<Sphere>& spheres, std::vector<Aabb>& boxes) const override;

private:
    Vec3 base_;
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
