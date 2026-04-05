#include "wormhole3d_init.h"
#include "wormhole3d_globals.h"

#include <GL/gl.h>
#include <GL/glu.h>

void initAppGl() {
    glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    glDisable(GL_COLOR_MATERIAL);

    sphereQuadric = gluNewQuadric();
    gluQuadricTexture(sphereQuadric, GL_FALSE);
    gluQuadricNormals(sphereQuadric, GLU_SMOOTH);
}
