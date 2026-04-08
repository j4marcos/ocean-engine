#include "scene_gpu.h"

#include "wormhole3d_globals.h"

#include <GL/glext.h>

#include <cstring>

#ifndef GL_RGBA32F_ARB
#define GL_RGBA32F_ARB 0x8814
#endif

void packSceneObjectsForGpu(SceneGpuPacked& out) {
    const int ns = static_cast<int>(gSpheres.size());
    const int nb = static_cast<int>(gBoxes.size());

    const size_t floatsPerTexel = 4;
    const size_t totalFloats = static_cast<size_t>(kSceneDataWidth) * static_cast<size_t>(kSceneDataHeight) * floatsPerTexel;
    out.pixels.assign(totalFloats, 0.0f);

    auto setTexel = [&](int row, int col, float a, float b, float c, float d) {
        if (col < 0 || col >= kSceneDataWidth || row < 0 || row >= kSceneDataHeight) {
            return;
        }
        const size_t idx = (static_cast<size_t>(row) * static_cast<size_t>(kSceneDataWidth) + static_cast<size_t>(col)) * floatsPerTexel;
        out.pixels[idx + 0] = a;
        out.pixels[idx + 1] = b;
        out.pixels[idx + 2] = c;
        out.pixels[idx + 3] = d;
    };

    int col = 0;
    for (int i = 0; i < ns && col < kSceneDataWidth; ++i, ++col) {
        const Sphere& s = gSpheres[static_cast<size_t>(i)];
        setTexel(0, col, 0.0f, s.center.x, s.center.y, s.center.z);
        setTexel(1, col, s.radius, s.color.r, s.color.g, s.color.b);
        setTexel(2, col, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    for (int i = 0; i < nb && col < kSceneDataWidth; ++i, ++col) {
        const Aabb& b = gBoxes[static_cast<size_t>(i)];
        setTexel(0, col, 1.0f, b.center.x, b.center.y, b.center.z);
        setTexel(1, col, b.halfSize.x, b.halfSize.y, b.halfSize.z, b.color.r);
        setTexel(2, col, b.color.g, b.color.b, 0.0f, 0.0f);
    }
    out.objectCount = col;
}

void uploadSceneDataTexture(GLuint tex, const float* pixels) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(GL_RGBA32F_ARB),
        kSceneDataWidth,
        kSceneDataHeight,
        0,
        GL_RGBA,
        GL_FLOAT,
        static_cast<const void*>(pixels));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}
