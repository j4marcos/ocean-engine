#include "wormhole3d_audio.h"
#include "wormhole3d_globals.h"
#include "scene_world.h"
#include "wormhole3d_init.h"
#include "wormhole3d_raster.h"
#include "wormhole3d_raycast.h"
#include "wormhole3d_raycast_gpu.h"
#include "wormhole3d_simulation.h"
#include "wormhole3d_ui.h"
#include "scene_textures.h"
#include "cpu_texture.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include <GL/glut.h>
#include <GL/freeglut_ext.h> // freeglut_ext não disponível em alguns ambientes

#include "scene_moving.h"

static void display() {

    // sincronizando tempo da app com o tempo do sistema para cliclo dia/noite e fps
    const int nowMs = glutGet(GLUT_ELAPSED_TIME);
    gSceneTimeMs = nowMs;
    gSceneTimeSec = static_cast<float>(nowMs) * 0.001f;
    // Offset de meio ciclo: t=0 da app corresponde a fase "meio-dia" (céu claro), não meia-noite.
    if (gDayNightAuto && gDayNightCycleMs > 0) {
        gDayNightEffectiveMs = nowMs + gDayNightCycleMs / 2;
    } else if (gDayNightAuto) {
        gDayNightEffectiveMs = nowMs;
    }
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

    // animando câmera - cinematica
    if (gAnimatingCamera) {
        gCameraT += 0.003f;

        if (gCameraT > 1.0f) {
            gCameraT = 0.0f;
            gAnimatingCamera = false;
        }

        const Vec3 prevCam = gCamera.position;
        gCamera.position = calculateBezierPoint(gCameraT, P0, P1, P2, P3);
        cameraApplyWormholeTeleportIfNeeded(prevCam, gCamera.position);
        glutPostRedisplay();
    }

    // movendo os elementos na cena - dinamicos
    sceneUpdateDynamicElements();

    // renderizando cena
    if (gUseRaycast) {
        raycastScene();
    } else {
        rasterScene();
    }

    drawOverlay();
    glutSwapBuffers();
}

// window proporção e viewport
static void reshape(const int w, const int h) {
    gWindowWidth = std::max(1, w);
    gWindowHeight = std::max(1, h);
    glViewport(0, 0, static_cast<GLsizei>(gWindowWidth), static_cast<GLsizei>(gWindowHeight));
    glutPostRedisplay();
}

static void applyNumericMode(const int digit) {
    switch (digit) {
        case 1:
            gUseRaycast = false;
            break;
        case 2:
            gUseRaycast = true;
            gUseGpuRaycast = false;
            break;
        case 3:
            gUseRaycast = true;
            if (gRaycastGpuReady) {
                gUseGpuRaycast = true;
            } else {
                gUseGpuRaycast = false;
            }
            break;
        case 4:
            gAnimatingCamera = !gAnimatingCamera;
            if (gAnimatingCamera && gCameraT > 1.0f) {
                gCameraT = 0.0f;
            }
            break;
        default:
            break;
    }
}

static void keyboard(const unsigned char key, const int x, const int y) {
    (void)x;
    (void)y;

    if (key >= '1' && key <= '9') {
        applyNumericMode(static_cast<int>(key - '0'));
        glutPostRedisplay();
        return;
    }

    const Vec3 forward = rayForward();
    const Vec3 right = rayRight();
    const float moveStep = 0.24f;

    switch (key) {
        case 'w':
        case 'W': {
            const Vec3 prev = gCamera.position;
            gCamera.position = add3(gCamera.position, scale3(forward, moveStep));
            cameraApplyWormholeTeleportIfNeeded(prev, gCamera.position);
            break;
        }
        case 's':
        case 'S': {
            const Vec3 prev = gCamera.position;
            gCamera.position = sub3(gCamera.position, scale3(forward, moveStep));
            cameraApplyWormholeTeleportIfNeeded(prev, gCamera.position);
            break;
        }
        case 'd':
        case 'D': {
            const Vec3 prev = gCamera.position;
            gCamera.position = sub3(gCamera.position, scale3(right, moveStep));
            cameraApplyWormholeTeleportIfNeeded(prev, gCamera.position);
            break;
        }
        case 'a':
        case 'A': {
            const Vec3 prev = gCamera.position;
            gCamera.position = add3(gCamera.position, scale3(right, moveStep));
            cameraApplyWormholeTeleportIfNeeded(prev, gCamera.position);
            break;
        }
        case 'z':
        case 'Z': {
            const Vec3 prev = gCamera.position;
            gCamera.position.y -= moveStep;
            cameraApplyWormholeTeleportIfNeeded(prev, gCamera.position);
            break;
        }
        case 'x':
        case 'X': {
            const Vec3 prev = gCamera.position;
            gCamera.position.y += moveStep;
            cameraApplyWormholeTeleportIfNeeded(prev, gCamera.position);
            break;
        }
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
        case '[':
            if (gDayNightAuto) {
                gDayNightEffectiveMs =
                    gDayNightCycleMs > 0 ? gSceneTimeMs + gDayNightCycleMs / 2 : gSceneTimeMs;
                gDayNightAuto = false;
            }
            gDayNightEffectiveMs -= gDayNightStepMs;
            if (gDayNightCycleMs > 0) {
                gDayNightEffectiveMs %= gDayNightCycleMs;
                if (gDayNightEffectiveMs < 0) {
                    gDayNightEffectiveMs += gDayNightCycleMs;
                }
            }
            break;
        case ']':
            if (gDayNightAuto) {
                gDayNightEffectiveMs =
                    gDayNightCycleMs > 0 ? gSceneTimeMs + gDayNightCycleMs / 2 : gSceneTimeMs;
                gDayNightAuto = false;
            }
            gDayNightEffectiveMs += gDayNightStepMs;
            if (gDayNightCycleMs > 0) {
                gDayNightEffectiveMs %= gDayNightCycleMs;
            }
            break;
        case 't':
        case 'T':
            if (gDayNightAuto) {
                gDayNightAuto = false;
                gDayNightEffectiveMs =
                    gDayNightCycleMs > 0 ? gSceneTimeMs + gDayNightCycleMs / 2 : gSceneTimeMs;
            } else {
                gDayNightAuto = true;
            }
            break;
        case 'v':
        case 'V':
            gSceneVehiclesEnabled = !gSceneVehiclesEnabled;
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

// forçar redraw da cena a cada frame
static void idle() {
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    // construindo a cena
    sceneBuild();

    // inicializando a janela da app
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
   glutInitContextVersion(2, 1); // não disponível em alguns ambientes
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutInitWindowPosition(100, 60);
    glutCreateWindow("Raycast Wormhole Simulation 3D");

    // 0 - fps ilimitado, 
    // 1 - fps limitado ao buffer de swap (monitor refresh rate)
    glutSwapInterval(0); // não disponível em alguns ambientes

    // carregando o buffer de raycast em gpu
    if (initGpuRaycast()) {
        gRaycastGpuReady = true;
    } else {
        std::cerr << "GPU raycast init failed; using CPU fallback for raycast mode.\n";
    }

    // configurando a janela da app
    glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    glDisable(GL_COLOR_MATERIAL);

    // configurando a esfera quadrica
    sphereQuadric = gluNewQuadric();
    gluQuadricTexture(sphereQuadric, GL_FALSE);
    gluQuadricNormals(sphereQuadric, GLU_SMOOTH);

    // carregando texturas da cena (deve ser feito APÓS criar o contexto GL)
    loadSceneTextures();
    // texturas CPU-side para o ray tracer (modo 2)
    loadCpuTextures();

    // init music background
    startBackgroundMusic(argv[0]);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);

    glutMainLoop();
    freeCpuTextures();
    freeSceneTextures();
    return 0;
}
