#include "bvh.h"
#include "wormhole3d_globals.h"  // gSpheres, gBoxes

#include <algorithm>
#include <cmath>
#include <limits>

namespace scene {

// ─── Definição das globais ────────────────────────────────────────────────────
Bvh   gSceneBvh;
float gSceneBoundingSphereRadius   = 1.0f;
Vec3  gSceneBoundingSphereCenter   = {0.0f, 0.0f, 0.0f};

// ─── buildBvh ─────────────────────────────────────────────────────────────────

struct PrimInfo {
    int  index;
    Vec3 min;
    Vec3 max;
};

static inline float primCentroid(const PrimInfo& p, int axis) {
    return (p.min[axis] + p.max[axis]) * 0.5f;
}

void buildBvh(Bvh& outBvh,
              const std::vector<Sphere>& spheres,
              const std::vector<Aabb>&   boxes)
{
    outBvh.nodes.clear();

    std::vector<PrimInfo> prims;
    prims.reserve(spheres.size() + boxes.size());

    for (size_t i = 0; i < spheres.size(); ++i) {
        const Sphere& s = spheres[i];
        prims.push_back({
            static_cast<int>(i),
            {s.center.x - s.radius, s.center.y - s.radius, s.center.z - s.radius},
            {s.center.x + s.radius, s.center.y + s.radius, s.center.z + s.radius}
        });
    }
    for (size_t i = 0; i < boxes.size(); ++i) {
        const Aabb& b = boxes[i];
        prims.push_back({
            static_cast<int>(spheres.size() + i),
            {b.center.x - b.halfSize.x, b.center.y - b.halfSize.y, b.center.z - b.halfSize.z},
            {b.center.x + b.halfSize.x, b.center.y + b.halfSize.y, b.center.z + b.halfSize.z}
        });
    }

    if (prims.empty()) {
        outBvh.nodes.push_back({{0,0,0},{0,0,0},-1,-1,-1,-1});
        return;
    }

    // Índices de trabalho ordenáveis
    std::vector<int> order(prims.size());
    for (int i = 0; i < static_cast<int>(prims.size()); ++i) order[i] = i;

    struct StackItem { int start, end, parent, isLeft; };
    std::vector<StackItem> stack;
    stack.push_back({0, static_cast<int>(prims.size()), -1, 0});

    while (!stack.empty()) {
        auto item = stack.back(); stack.pop_back();
        int start = item.start, end = item.end;
        int count = end - start;

        // Calcular AABB do intervalo
        Vec3 nodeMin = prims[order[start]].min;
        Vec3 nodeMax = prims[order[start]].max;
        for (int i = start + 1; i < end; ++i) {
            const PrimInfo& p = prims[order[i]];
            nodeMin = {std::min(nodeMin.x, p.min.x),
                       std::min(nodeMin.y, p.min.y),
                       std::min(nodeMin.z, p.min.z)};
            nodeMax = {std::max(nodeMax.x, p.max.x),
                       std::max(nodeMax.y, p.max.y),
                       std::max(nodeMax.z, p.max.z)};
        }

        BvhNode node;
        node.aabbMin = nodeMin;
        node.aabbMax = nodeMax;
        node.left  = -1;
        node.right = -1;
        node.prim0 = -1;
        node.prim1 = -1;

        int idx = static_cast<int>(outBvh.nodes.size());
        outBvh.nodes.push_back(node);

        // Conectar ao pai
        if (item.parent >= 0) {
            if (item.isLeft) outBvh.nodes[item.parent].left  = idx;
            else             outBvh.nodes[item.parent].right = idx;
        }

        if (count <= 2) {
            // Folha: armazenar primitivos diretamente no nó
            outBvh.nodes[idx].prim0 = prims[order[start]].index;
            if (count > 1)
                outBvh.nodes[idx].prim1 = prims[order[start + 1]].index;
            continue;
        }

        // Split pelo eixo de maior extensão (SAH simplificado)
        Vec3 ext = {nodeMax.x - nodeMin.x,
                    nodeMax.y - nodeMin.y,
                    nodeMax.z - nodeMin.z};
        int axis = (ext.x >= ext.y && ext.x >= ext.z) ? 0
                 : (ext.y >= ext.z)                   ? 1
                 :                                       2;

        int mid = start + count / 2;
        std::nth_element(
            order.begin() + start,
            order.begin() + mid,
            order.begin() + end,
            [&prims, axis](int a, int b) {
                return primCentroid(prims[a], axis) < primCentroid(prims[b], axis);
            });

        stack.push_back({mid,   end, idx, 0}); // filho direito
        stack.push_back({start, mid, idx, 1}); // filho esquerdo
    }
}

// ─── computeSceneBoundingSphere ───────────────────────────────────────────────

void computeSceneBoundingSphere(const std::vector<Sphere>& spheres,
                                const std::vector<Aabb>&   boxes)
{
    if (spheres.empty() && boxes.empty()) {
        gSceneBoundingSphereCenter = {0.0f, 0.0f, 0.0f};
        gSceneBoundingSphereRadius = 1.0f;
        return;
    }

    // AABB de tudo
    Vec3 mn = { std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max() };
    Vec3 mx = { std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest() };

    for (const Sphere& s : spheres) {
        mn = {std::min(mn.x, s.center.x - s.radius),
              std::min(mn.y, s.center.y - s.radius),
              std::min(mn.z, s.center.z - s.radius)};
        mx = {std::max(mx.x, s.center.x + s.radius),
              std::max(mx.y, s.center.y + s.radius),
              std::max(mx.z, s.center.z + s.radius)};
    }
    for (const Aabb& b : boxes) {
        mn = {std::min(mn.x, b.center.x - b.halfSize.x),
              std::min(mn.y, b.center.y - b.halfSize.y),
              std::min(mn.z, b.center.z - b.halfSize.z)};
        mx = {std::max(mx.x, b.center.x + b.halfSize.x),
              std::max(mx.y, b.center.y + b.halfSize.y),
              std::max(mx.z, b.center.z + b.halfSize.z)};
    }

    gSceneBoundingSphereCenter = {(mn.x + mx.x) * 0.5f,
                                  (mn.y + mx.y) * 0.5f,
                                  (mn.z + mx.z) * 0.5f};

    float radius = 0.0f;
    for (const Sphere& s : spheres) {
        float dx = s.center.x - gSceneBoundingSphereCenter.x;
        float dy = s.center.y - gSceneBoundingSphereCenter.y;
        float dz = s.center.z - gSceneBoundingSphereCenter.z;
        float d  = std::sqrt(dx*dx + dy*dy + dz*dz) + s.radius;
        radius = std::max(radius, d);
    }
    for (const Aabb& b : boxes) {
        float dx = std::abs(b.center.x - gSceneBoundingSphereCenter.x);
        float dy = std::abs(b.center.y - gSceneBoundingSphereCenter.y);
        float dz = std::abs(b.center.z - gSceneBoundingSphereCenter.z);
        float corner = std::sqrt(dx*dx + dy*dy + dz*dz)
                     + std::max({b.halfSize.x, b.halfSize.y, b.halfSize.z});
        radius = std::max(radius, corner);
    }

    gSceneBoundingSphereRadius = radius;
}

// ─── rayIntersectsSceneBoundingSphere ────────────────────────────────────────

/**
 * Testa interseção raio-esfera.
 * Retorna t >= 0.0 do ponto de entrada (ou saída se câmera dentro),
 * ou -1.0 se não há interseção.
 */
float rayIntersectsSceneBoundingSphere(const Vec3& origin, const Vec3& dir)
{
    if (gSceneBoundingSphereRadius < 1.0f) {
        // Esfera degenerada: assume que sempre pode haver conteúdo
        return 0.0f;
    }
    const Vec3& c  = gSceneBoundingSphereCenter;
    float r = gSceneBoundingSphereRadius;

    float lx = origin.x - c.x;
    float ly = origin.y - c.y;
    float lz = origin.z - c.z;

    float a = dir.x*dir.x + dir.y*dir.y + dir.z*dir.z;
    if (a < 1e-12f) return -1.0f;

    float halfB = dir.x*lx + dir.y*ly + dir.z*lz;
    float cq    = lx*lx + ly*ly + lz*lz - r*r;
    float disc  = halfB*halfB - a*cq;
    if (disc < 0.0f) return -1.0f;

    float s  = std::sqrt(disc);
    float tA = (-halfB - s) / a;
    float tB = (-halfB + s) / a;

    if (tA >= 0.0f) return tA;
    if (tB >= 0.0f) return tB; // câmera dentro da esfera
    return -1.0f;
}

} // namespace scene
