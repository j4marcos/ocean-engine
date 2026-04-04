#include "wormhole3d_globals.h"
#include "wormhole3d_init.h"
#include "wormhole3d_raster.h"
#include "wormhole3d_raycast.h"
#include "wormhole3d_raycast_gpu.h"
#include "wormhole3d_simulation.h"
#include "wormhole3d_ui.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include <GL/glut.h>
#include <GL/freeglut_ext.h>

static void display() {
    const int nowMs = glutGet(GLUT_ELAPSED_TIME);
    if (gFpsLastMs == 0) {
        gFpsLastMs = nowMs;
    }
    gFpsFrameAccum++;
    const int elapsed = nowMs - gFpsLastMs;
    if (elapsed >= 500) {
        gFpsDisplay = static_cast<float>(gFpsFrameAccum) * 1000.0f / static_cast<float>(elapsed);
        gFpsFrameAccum = 0;
        gFpsLastMs = nowMs;
    }

    if (gAnimatingCamera) {
        gCameraT += 0.003f;

        if (gCameraT > 1.0f) {
            gCameraT = 0.0f;
            gAnimatingCamera = false;
        }

        gCamera.position = calculateBezierPoint(gCameraT, P0, P1, P2, P3);
        glutPostRedisplay();
    }

    if (gUseRaycast) {
        raycastScene();
    } else {
        rasterScene();
    }

    drawOverlay();
    glutSwapBuffers();
}

static void reshape(const int w, const int h) {
    gWindowWidth = std::max(1, w);
    gWindowHeight = std::max(1, h);
    glViewport(0, 0, static_cast<GLsizei>(gWindowWidth), static_cast<GLsizei>(gWindowHeight));
    glutPostRedisplay();
}

static void keyboard(const unsigned char key, const int x, const int y) {
    (void)x;
    (void)y;

    const Vec3 forward = rayForward();
    const Vec3 right = rayRight();
    const float moveStep = 0.24f;

    switch (key) {
        case 27:
        case 'q':
        case 'Q':
            std::exit(0);
            break;
        case 'w':
        case 'W':
            gCamera.position = add3(gCamera.position, scale3(forward, moveStep));
            break;
        case 's':
        case 'S':
            gCamera.position = sub3(gCamera.position, scale3(forward, moveStep));
            break;
        case 'd':
        case 'D':
            gCamera.position = sub3(gCamera.position, scale3(right, moveStep));
            break;
        case 'a':
        case 'A':
            gCamera.position = add3(gCamera.position, scale3(right, moveStep));
            break;
        case 'r':
        case 'R':
            gWormhole.holeA.strength = clampf(gWormhole.holeA.strength + 0.02f, 0.02f, 1.2f);
            gWormhole.holeB.strength = gWormhole.holeA.strength;
            break;
        case 'f':
        case 'F':
            gWormhole.holeA.strength = clampf(gWormhole.holeA.strength - 0.02f, 0.02f, 1.2f);
            gWormhole.holeB.strength = gWormhole.holeA.strength;
            break;
        case 't':
        case 'T':
            gUseRaycast = !gUseRaycast;
            break;
        case 'c':
        case 'C':
            gAnimatingCamera = !gAnimatingCamera;
            if (gAnimatingCamera && gCameraT > 1.0f) {
                gCameraT = 0.0f;
            }
            break;
    }

    glutPostRedisplay();
}

static void specialKeys(const int key, const int x, const int y) {
    (void)x;
    (void)y;

    const float angStep = 0.05f;
    switch (key) {
        case GLUT_KEY_LEFT:
            gCamera.yawHorizontalDegree -= angStep;
            break;
        case GLUT_KEY_RIGHT:
            gCamera.yawHorizontalDegree += angStep;
            break;
        case GLUT_KEY_UP:
            gCamera.pitchVerticalDegree = clampf(gCamera.pitchVerticalDegree + angStep, -1.25f, 1.25f);
            break;
        case GLUT_KEY_DOWN:
            gCamera.pitchVerticalDegree = clampf(gCamera.pitchVerticalDegree - angStep, -1.25f, 1.25f);
            break;
    }
    glutPostRedisplay();
}

static void mouse(const int button, const int state, const int x, const int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) {
        return;
    }

    const int yBottom = gWindowHeight - y;
    const bool insideMode =
        x >= gButtonX && x <= gButtonX + gButtonW &&
        yBottom >= gButtonY && yBottom <= gButtonY + gButtonH;

    if (insideMode) {
        gUseRaycast = !gUseRaycast;
        glutPostRedisplay();
        return;
    }

    const int gpuBtnY = gButtonY + gButtonH + kGpuCpuButtonGap;
    const bool insideGpuCpu =
        gRaycastGpuReady && gUseRaycast &&
        x >= gButtonX && x <= gButtonX + gButtonW &&
        yBottom >= gpuBtnY && yBottom <= gpuBtnY + gButtonH;

    if (insideGpuCpu) {
        gUseGpuRaycast = !gUseGpuRaycast;
        glutPostRedisplay();
    }
}

static void idle() {
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitContextVersion(2, 1);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutInitWindowPosition(100, 60);
    glutCreateWindow("Raycast Wormhole Simulation 3D");

    glutSwapInterval(0);

    if (initGpuRaycast()) {
        gRaycastGpuReady = true;
    } else {
        std::cerr << "GPU raycast init failed; using CPU fallback for raycast mode.\n";
    }

    initAppGl();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
