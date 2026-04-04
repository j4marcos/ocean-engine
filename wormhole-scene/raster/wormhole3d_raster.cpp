#include "wormhole3d_raster.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"

#include <GL/glut.h>

void drawSkybox() {
    if (!gTexSky) {
        return;
    }

    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gTexSky);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glPushMatrix();
    glTranslatef(gCamera.position.x, gCamera.position.y, gCamera.position.z);
    gluSphere(sphereQuadric, 80.0, 64, 64);
    glPopMatrix();

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void rasterScene() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    const GLfloat lightPos[4] = {2.0f, 4.0f, 2.0f, 1.0f};
    const GLfloat lightDiffuse[4] = {0.95f, 0.95f, 0.92f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(gCamera.fovViewDegree, static_cast<double>(gWindowWidth) / static_cast<double>(gWindowHeight), 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const Vec3 f = rayForward();
    const Vec3 target = add3(gCamera.position, f);
    gluLookAt(gCamera.position.x, gCamera.position.y, gCamera.position.z, target.x, target.y, target.z, 0.0, 1.0, 0.0);

    drawSkybox();

    glDisable(GL_LIGHTING);
    glColor3f(0.13f, 0.16f, 0.19f);
    glBegin(GL_QUADS);
    glVertex3f(-30.0f, -1.15f, -30.0f);
    glVertex3f(30.0f, -1.15f, -30.0f);
    glVertex3f(30.0f, -1.15f, 30.0f);
    glVertex3f(-30.0f, -1.15f, 30.0f);
    glEnd();

    glEnable(GL_LIGHTING);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, myTexture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        glPushMatrix();
        glTranslatef(gSpheres[i].center.x, gSpheres[i].center.y, gSpheres[i].center.z);
        GLfloat kd[4] = {gSpheres[i].color.x, gSpheres[i].color.y, gSpheres[i].color.z, 1.0f};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, kd);
        gluSphere(sphereQuadric, gSpheres[i].radius, 32, 24);
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);

    for (size_t i = 0; i < gBoxes.size(); ++i) {
        glPushMatrix();
        glTranslatef(gBoxes[i].center.x, gBoxes[i].center.y, gBoxes[i].center.z);
        glScalef(gBoxes[i].halfSize.x * 2.0f, gBoxes[i].halfSize.y * 2.0f, gBoxes[i].halfSize.z * 2.0f);
        GLfloat kd[4] = {gBoxes[i].color.x, gBoxes[i].color.y, gBoxes[i].color.z, 1.0f};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, kd);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    glColor3f(0.08f, 0.60f, 0.95f);
    glPushMatrix();
    glTranslatef(gWormhole.holeA.center.x, gWormhole.holeA.center.y, gWormhole.holeA.center.z);
    glutWireSphere(gWormhole.holeA.radius, 24, 24);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gWormhole.holeB.center.x, gWormhole.holeB.center.y, gWormhole.holeB.center.z);
    glutWireSphere(gWormhole.holeB.radius, 24, 24);
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
}
