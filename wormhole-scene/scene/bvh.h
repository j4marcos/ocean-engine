#pragma once

#include "wormhole3d_types.h"
#include <vector>

namespace scene {

struct BvhNode {
    Vec3 aabbMin;
    Vec3 aabbMax;
    int left;   // índice do filho esquerdo (-1 = folha)
    int right;  // índice do filho direito  (-1 = folha)
    int prim0;  // índice do primeiro primitivo na folha (-1 = nó interno)
    int prim1;  // índice do segundo primitivo (-1 = ausente ou nó interno)
};

struct Bvh {
    std::vector<BvhNode> nodes;
};

// Globais definidos em bvh.cpp
extern Bvh gSceneBvh;
extern float gSceneBoundingSphereRadius;
extern Vec3  gSceneBoundingSphereCenter;

/** Constrói o BVH a partir dos primitivos da cena. Chamar após sceneBuild() ou updateSceneBvh(). */
void buildBvh(Bvh& outBvh, const std::vector<Sphere>& spheres, const std::vector<Aabb>& boxes);

/** Calcula a bounding sphere mínima da cena e armazena em gSceneBoundingSphereCenter/Radius. */
void computeSceneBoundingSphere(const std::vector<Sphere>& spheres, const std::vector<Aabb>& boxes);

/** Testa se o raio (origin, dir) intersecta a bounding sphere da cena. Retorna t >= 0 ou -1 se não. */
float rayIntersectsSceneBoundingSphere(const Vec3& origin, const Vec3& dir);

} // namespace scene
