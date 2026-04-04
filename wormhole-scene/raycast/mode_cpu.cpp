#include "wormhole3d_raycast_cpu.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"

#include <cmath>

#include <GL/gl.h>
#include <GL/glu.h>

void raycastSceneCpu() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    const Vec3 f = rayForward();
    const Vec3 r = rayRight();
    const Vec3 up = rayUp();

    const float aspect = static_cast<float>(kRaycastWidth) / static_cast<float>(kRaycastHeight);
    const float tanHalfFov = std::tan((gCamera.fovViewDegree * 3.14159265359f / 180.0f) * 0.5f);

    for (int rowFromBottom = 0; rowFromBottom < kRaycastHeight; ++rowFromBottom) {
        for (int x = 0; x < kRaycastWidth; ++x) {
            const float px = -((2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(kRaycastWidth) - 1.0f) * aspect * tanHalfFov);
            const float py = -((-1.0f + 2.0f * (static_cast<float>(rowFromBottom) + 0.5f) / static_cast<float>(kRaycastHeight)) * tanHalfFov);
            const Vec3 dir = normalize3(add3(f, add3(scale3(r, px), scale3(up, py))));

            const Vec3 c = traceRay(gCamera.position, dir);
            const int idx = (rowFromBottom * kRaycastWidth + x) * 3;
            gRaycastPixels[idx + 0] = static_cast<unsigned char>(clampf(c.x, 0.0f, 1.0f) * 255.0f);
            gRaycastPixels[idx + 1] = static_cast<unsigned char>(clampf(c.y, 0.0f, 1.0f) * 255.0f);
            gRaycastPixels[idx + 2] = static_cast<unsigned char>(clampf(c.z, 0.0f, 1.0f) * 255.0f);
        }
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(gWindowWidth), 0.0, static_cast<double>(gWindowHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRasterPos2i(0, 0);
    const float zoomX = static_cast<float>(gWindowWidth) / static_cast<float>(kRaycastWidth);
    const float zoomY = static_cast<float>(gWindowHeight) / static_cast<float>(kRaycastHeight);
    glPixelZoom(zoomX, zoomY);
    glDrawPixels(kRaycastWidth, kRaycastHeight, GL_RGB, GL_UNSIGNED_BYTE, &gRaycastPixels[0]);
    glPixelZoom(1.0f, 1.0f);
}
