#include "wormhole3d_raster.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"
#include "scene_entities.h"
#include "scene_moving.h"

#include <cmath>

#include <GL/glut.h>

namespace {

void drawSceneFloor() {
    glDisable(GL_LIGHTING);
    glColor3f(kSceneFloorMaterial.r, kSceneFloorMaterial.g, kSceneFloorMaterial.b);
    const float y = kSceneGroundY;
    const float e = 80.0f;
    glBegin(GL_QUADS);
    glVertex3f(-e, y, -e);
    glVertex3f(e, y, -e);
    glVertex3f(e, y, e);
    glVertex3f(-e, y, e);
    glEnd();
}

void drawMovingBezierSpheres() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    Vec3 sph[3];
    movingBezierSpheresCompute(sph);
    glColor3f(kMovingSphereMaterial.r, kMovingSphereMaterial.g, kMovingSphereMaterial.b);
    for (int b = 0; b < 3; ++b) {
        const Vec3& p = sph[b];
        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glutSolidSphere(static_cast<double>(kMovingSphereRadius), 10, 10);
        glPopMatrix();
    }
}

void drawMovingCars() {
    if (!gSceneVehiclesEnabled) {
        return;
    }
    glDisable(GL_LIGHTING);
    Vec3 anchors[3];
    movingCarsCompute(anchors);
    for (int i = 0; i < 3; ++i) {
        CarRigidState st;
        carRigidState(i, anchors[i], st);
        const Vec3 rr = carRearBoxCenterWorld(st);
        glColor3f(0.9f, 0.13f, 0.11f);
        glPushMatrix();
        glTranslatef(rr.x, rr.y, rr.z);
        glScalef(kCarRearHalfX * 2.0f, kCarRearHalfY * 2.0f, kCarRearHalfZ * 2.0f);
        glutSolidCube(1.0);
        glPopMatrix();
        const Vec3 ff = carFrontBoxCenterWorld(st);
        glColor3f(kMovingCarMaterial.r, kMovingCarMaterial.g, kMovingCarMaterial.b);
        glPushMatrix();
        glTranslatef(ff.x, ff.y, ff.z);
        glScalef(kCarFrontHalfX * 2.0f, kCarFrontHalfY * 2.0f, kCarFrontHalfZ * 2.0f);
        glutSolidCube(1.0);
        glPopMatrix();
    }
}

/** Mesma geometria que o SDF do raycast: pilar + caixa aberta (5 faces) a girar; não é gBoxes. */
void drawLighthouseRaster() {
    glDisable(GL_LIGHTING);
    const Vec3 tc = lighthouseTowerCenterWorld();
    const Vec3 hc = lighthouseHeadCenterWorld();
    const float yawDeg = lighthouseYawRad() * (180.0f / static_cast<float>(M_PI));
    const float h = kLighthouseHeadHalf;
    const float t = kLighthouseFaceThickness;

    glColor3f(0.9f, 0.89f, 0.86f);
    glPushMatrix();
    glTranslatef(tc.x, tc.y, tc.z);
    glScalef(kLighthouseTowerHalfXZ * 2.0f, kLighthouseTowerHalfY * 2.0f, kLighthouseTowerHalfXZ * 2.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    glColor3f(0.72f, 0.78f, 0.86f);
    glPushMatrix();
    glTranslatef(hc.x, hc.y, hc.z);
    glRotatef(static_cast<double>(yawDeg), 0.0, 1.0, 0.0);
    auto plate = [](float cx, float cy, float cz, float hx, float hy, float hz) {
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        glScalef(hx * 2.0f, hy * 2.0f, hz * 2.0f);
        glutSolidCube(1.0);
        glPopMatrix();
    };
    plate(-h, 0.0f, 0.0f, t, h, h);
    plate(0.0f, 0.0f, h, h, h, t);
    plate(0.0f, 0.0f, -h, h, h, t);
    plate(0.0f, h, 0.0f, h, t, h);
    plate(0.0f, -h, 0.0f, h, t, h);
    glPopMatrix();

    glColor3f(1.0f, 0.98f, 0.92f);
    glPushMatrix();
    glTranslatef(hc.x, hc.y, hc.z);
    glutSolidSphere(0.12, 10, 8);
    glPopMatrix();
}

} // namespace

void rasterScene() {
    sceneUpdateDynamicElements();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);

    DayNightLighting dn;
    computeDayNightLighting(dn);
    const float day = dn.skyDayFactor;

    glClearColor(0.02f + 0.14f * day, 0.03f + 0.18f * day, 0.06f + 0.24f * day, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(gCamera.fovViewDegree, static_cast<double>(gWindowWidth) / static_cast<double>(gWindowHeight), 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const Vec3 f = rayForward();
    const Vec3 target = add3(gCamera.position, f);
    gluLookAt(gCamera.position.x, gCamera.position.y, gCamera.position.z, target.x, target.y, target.z, 0.0, 1.0, 0.0);

    glShadeModel(GL_SMOOTH);

    drawSceneFloor();

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const RGBA& c = gSpheres[i].color;
        glColor3f(c.r, c.g, c.b);
        glPushMatrix();
        glTranslatef(gSpheres[i].center.x, gSpheres[i].center.y, gSpheres[i].center.z);
        gluSphere(sphereQuadric, gSpheres[i].radius, 32, 24);
        glPopMatrix();
    }

    for (size_t i = 0; i < gBoxes.size(); ++i) {
        const RGBA& c = gBoxes[i].color;
        const bool thinPole = gBoxes[i].halfSize.x < 0.12f && gBoxes[i].halfSize.z < 0.12f;
        if (thinPole) {
            glColor3f(c.r * 0.85f + 0.15f, c.g * 0.85f + 0.15f, c.b * 0.85f + 0.15f);
        } else {
            glColor3f(c.r, c.g, c.b);
        }
        glPushMatrix();
        glTranslatef(gBoxes[i].center.x, gBoxes[i].center.y, gBoxes[i].center.z);
        glScalef(gBoxes[i].halfSize.x * 2.0f, gBoxes[i].halfSize.y * 2.0f, gBoxes[i].halfSize.z * 2.0f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    glColor3f(0.08f, 0.60f, 0.95f);
    glPushMatrix();
    glTranslatef(gWormhole.holeA.center.x, gWormhole.holeA.center.y, gWormhole.holeA.center.z);
    glutWireSphere(gWormhole.holeA.warpRadius, 24, 24);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gWormhole.holeB.center.x, gWormhole.holeB.center.y, gWormhole.holeB.center.z);
    glutWireSphere(gWormhole.holeB.warpRadius, 24, 24);
    glPopMatrix();

    glColor3f(0.22f, 0.86f, 1.0f);
    glPushMatrix();
    glTranslatef(gWormhole.holeA.center.x, gWormhole.holeA.center.y, gWormhole.holeA.center.z);
    glutSolidSphere(gWormhole.holeA.coreRadius, 20, 16);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gWormhole.holeB.center.x, gWormhole.holeB.center.y, gWormhole.holeB.center.z);
    glutSolidSphere(gWormhole.holeB.coreRadius, 20, 16);
    glPopMatrix();

    drawMovingBezierSpheres();
    drawMovingCars();
    drawLighthouseRaster();
}
