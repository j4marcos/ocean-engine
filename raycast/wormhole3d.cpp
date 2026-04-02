#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint myTexture;
GLUquadric* sphereQuadric;

struct Vec3 {
    float x;
    float y;
    float z;
};

struct WarpHole3D {
    Vec3 center;
    float radius;
    float coreRadius;
    float strength;
};

struct Wormhole3D {
    WarpHole3D holeA;
    WarpHole3D holeB;
};

struct Sphere {
    Vec3 center;
    float radius;
    Vec3 color;
};

struct Aabb {
    Vec3 center;
    Vec3 halfSize;
    Vec3 color;
};

struct Camera {
    Vec3 position;
    float yawHorizontalDegree;
    float pitchVerticalDegree;
    float fovViewDegree;
};

static int gWindowWidth = 1280;
static int gWindowHeight = 720;
static bool gUseRaycast = false;

static Wormhole3D gWormhole = {
    {{-1.6f, 0.45f, -4.2f}, 1.45f, 0.46f, 0.19f},
    {{1.9f, 0.15f, -9.2f}, 1.45f, 0.46f, 0.19f}
};

static Camera gCamera = {{0.0f, 0.9f, 2.2f}, 0.0f, -0.15f, 58.0f};

static std::vector<Sphere> gSpheres = {
    {{-2.2f, -0.15f, -6.2f}, 0.85f, {0.85f, 0.40f, 0.20f}},
    {{0.1f, -0.25f, -5.6f}, 0.75f, {0.25f, 0.72f, 0.92f}},
    {{2.4f, 0.00f, -7.5f}, 0.95f, {0.95f, 0.84f, 0.28f}}
};

static std::vector<Aabb> gBoxes = {
    {{-0.9f, -0.30f, -3.9f}, {0.55f, 0.55f, 0.55f}, {0.90f, 0.30f, 0.28f}},
    {{1.1f, -0.50f, -6.4f}, {0.80f, 0.35f, 0.70f}, {0.38f, 0.88f, 0.40f}},
    {{3.1f, -0.60f, -10.0f}, {0.50f, 0.25f, 0.50f}, {0.74f, 0.74f, 0.80f}}
};

static const int kRaycastWidth = 360;
static const int kRaycastHeight = 220;
static std::vector<unsigned char> gRaycastPixels(kRaycastWidth * kRaycastHeight * 3, 0);

static int gButtonX = 18;
static int gButtonY = 16;
static int gButtonW = 340;
static int gButtonH = 34;

// --- VARIÁVEIS PARA A CURVA DE BÉZIER (PRÁTICA 06) ---
static bool gAnimatingCamera = false;
static float gCameraT = 0.0f; // Varia de 0.0 (início) a 1.0 (fim)

// 4 Pontos de controlo da curva: Início -> Entrada do Wormhole A -> Saída do Wormhole B -> Fim
static Vec3 P0 = {0.0f, 0.9f, 2.2f};     // Posição inicial onde a câmara começa
static Vec3 P1 = {-1.6f, 0.45f, -1.0f};  // Puxa a câmara na direção do Wormhole A
static Vec3 P2 = {-1.6f, 0.45f, -4.2f};  // Exatamente no centro do Wormhole A
static Vec3 P3 = {1.9f, 0.15f, -9.2f};   // Sai voando pelo centro do Wormhole B
// ------------------------------------------------------

void loadTexture(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    
    if (data) {
        glGenTextures(1, &myTexture);
        glBindTexture(GL_TEXTURE_2D, myTexture);
        
        // Parâmetros de textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Carrega a textura dependendo dos canais (RGB ou RGBA)
        if (nrChannels == 3) {
            gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
        } else if (nrChannels == 4) {
            gluBuild2DMipmaps(GL_TEXTURE_2D, 4, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        stbi_image_free(data);
    } else {
        std::cerr << "Falha ao carregar a textura: " << filename << std::endl;
    }
}

static float clampf(const float v, const float lo, const float hi) {
    return std::max(lo, std::min(v, hi));
}

static float length3(const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static Vec3 add3(const Vec3& a, const Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

static Vec3 sub3(const Vec3& a, const Vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static Vec3 scale3(const Vec3& v, const float s) {
    return {v.x * s, v.y * s, v.z * s};
}

static float dot3(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 normalize3(const Vec3& v) {
    const float len = length3(v);
    if (len <= 1e-6f) {
        return {0.0f, 0.0f, 0.0f};
    }
    return {v.x / len, v.y / len, v.z / len};
}

static Vec3 abs3(const Vec3& v) {
    return {std::fabs(v.x), std::fabs(v.y), std::fabs(v.z)};
}

static Vec3 max3(const Vec3& v, const float m) {
    return {std::max(v.x, m), std::max(v.y, m), std::max(v.z, m)};
}

// Calcula um ponto na Curva de Bézier Cúbica
static Vec3 calculateBezierPoint(float t, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    Vec3 p;
    p.x = uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x;
    p.y = uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y;
    p.z = uuu * p0.z + 3 * uu * t * p1.z + 3 * u * tt * p2.z + ttt * p3.z;
    return p;
}

static Vec3 rayForward() {
    const float cp = std::cos(gCamera.pitchVerticalDegree);
    return normalize3({
        std::sin(gCamera.yawHorizontalDegree) * cp,
        std::sin(gCamera.pitchVerticalDegree),
        -std::cos(gCamera.yawHorizontalDegree) * cp
    });
}

static Vec3 rayRight() {
    const Vec3 f = rayForward();
    return normalize3({f.z, 0.0f, -f.x});
}

static Vec3 rayUp() {
    const Vec3 r = rayRight();
    const Vec3 f = rayForward();
    return normalize3({
        r.y * f.z - r.z * f.y,
        r.z * f.x - r.x * f.z,
        r.x * f.y - r.y * f.x
    });
}

static Vec3 warpFieldFromHole(const Vec3& p, const WarpHole3D& hole) {
    const Vec3 toCenter = sub3(hole.center, p);
    const float d = length3(toCenter);
    const float softened = d + 0.04f;
    const float influence = std::exp(-(d * d) / (hole.radius * hole.radius));
    const float magnitude = (hole.strength * influence) / (softened * softened);
    return scale3(normalize3(toCenter), magnitude);
}

static Vec3 warpField(const Vec3& p) {
    return add3(warpFieldFromHole(p, gWormhole.holeA), warpFieldFromHole(p, gWormhole.holeB));
}

// sdf = Signed Distance Function
static float SignedDistanceSphere(const Vec3& p, const Sphere& s) {
    return length3(sub3(p, s.center)) - s.radius;
}

static float SignedDistanceAabb(const Vec3& p, const Aabb& b) {
    const Vec3 q = sub3(abs3(sub3(p, b.center)), b.halfSize);
    const Vec3 qmax = max3(q, 0.0f);
    const float outside = length3(qmax);
    const float inside = std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
    return outside + inside;
}

static float SignedDistanceFloor(const Vec3& p) {
    return p.y + 1.15f;
}

static float SignedDistanceScene(const Vec3& p) {
    float d = SignedDistanceFloor(p);
    for (size_t i = 0; i < gSpheres.size(); ++i) {
        d = std::min(d, SignedDistanceSphere(p, gSpheres[i]));
    }
    for (size_t i = 0; i < gBoxes.size(); ++i) {
        d = std::min(d, SignedDistanceAabb(p, gBoxes[i]));
    }
    return d;
}

static Vec3 sceneColorAt(const Vec3& p) {
    float bestD = SignedDistanceFloor(p);
    Vec3 color = {0.35f, 0.37f, 0.41f};

    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const float d = SignedDistanceSphere(p, gSpheres[i]);
        if (d < bestD) {
            bestD = d;
            color = gSpheres[i].color;
        }
    }

    for (size_t i = 0; i < gBoxes.size(); ++i) {
        const float d = SignedDistanceAabb(p, gBoxes[i]);
        if (d < bestD) {
            bestD = d;
            color = gBoxes[i].color;
        }
    }

    return color;
}

static Vec3 estimateNormal(const Vec3& p) {
    const float e = 0.01f;
    const float dx = SignedDistanceScene({p.x + e, p.y, p.z}) - SignedDistanceScene({p.x - e, p.y, p.z});
    const float dy = SignedDistanceScene({p.x, p.y + e, p.z}) - SignedDistanceScene({p.x, p.y - e, p.z});
    const float dz = SignedDistanceScene({p.x, p.y, p.z + e}) - SignedDistanceScene({p.x, p.y, p.z - e});
    return normalize3({dx, dy, dz});
}

static Vec3 teleportToOppositeSide(
    const Vec3& entryPoint,
    const WarpHole3D& source,
    const WarpHole3D& destination,
    const float margin
) {
    Vec3 normal = normalize3(sub3(entryPoint, source.center));
    if (length3(normal) <= 1e-6f) {
        normal = {1.0f, 0.0f, 0.0f};
    }
    return add3(destination.center, scale3(normal, -(destination.coreRadius + margin)));
}

static void rasterScene() {
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

static Vec3 skyColor(const Vec3& dir) {
    const float t = 0.5f * (dir.y + 1.0f);
    return {
        0.03f + 0.20f * t,
        0.04f + 0.22f * t,
        0.07f + 0.34f * t
    };
}

static Vec3 traceRay(const Vec3& origin, Vec3 dir) {
    Vec3 position = origin;
    const float stepLength = 0.15f;
    const float maxDist = 75.0f;
    const int maxSteps = 420;
    const float hitEps = 0.08f;
    const float exitMargin = 0.04f;

    for (int i = 0; i < maxSteps; ++i) {
        const Vec3 accel = warpField(position);
        dir = normalize3(add3(dir, scale3(accel, stepLength * 0.85f)));

        Vec3 nextPos = add3(position, scale3(dir, stepLength));

        const float prevDistA = length3(sub3(position, gWormhole.holeA.center));
        const float nextDistA = length3(sub3(nextPos, gWormhole.holeA.center));
        const float prevDistB = length3(sub3(position, gWormhole.holeB.center));
        const float nextDistB = length3(sub3(nextPos, gWormhole.holeB.center));

        if (prevDistA >= gWormhole.holeA.coreRadius && nextDistA < gWormhole.holeA.coreRadius) {
            nextPos = teleportToOppositeSide(nextPos, gWormhole.holeA, gWormhole.holeB, exitMargin);
        } else if (prevDistB >= gWormhole.holeB.coreRadius && nextDistB < gWormhole.holeB.coreRadius) {
            nextPos = teleportToOppositeSide(nextPos, gWormhole.holeB, gWormhole.holeA, exitMargin);
        }

        position = nextPos;

        const float d = SignedDistanceScene(position);
        if (d < hitEps) {
            const Vec3 n = estimateNormal(position);
            const Vec3 lightDir = normalize3({0.62f, 0.74f, 0.23f});
            const float lambert = clampf(dot3(n, lightDir), 0.0f, 1.0f);
            const Vec3 base = sceneColorAt(position);
            const float amb = 0.22f;
            const float shade = amb + lambert * 0.78f;
            return {base.x * shade, base.y * shade, base.z * shade};
        }

        if (length3(sub3(position, origin)) > maxDist) {
            break;
        }
    }

    return skyColor(dir);
}

static void raycastScene() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    const Vec3 f = rayForward();
    const Vec3 r = rayRight();
    const Vec3 u = rayUp();

    const float aspect = static_cast<float>(kRaycastWidth) / static_cast<float>(kRaycastHeight);
    const float tanHalfFov = std::tan((gCamera.fovViewDegree * 3.14159265359f / 180.0f) * 0.5f);

    for (int y = 0; y < kRaycastHeight; ++y) {
        for (int x = 0; x < kRaycastWidth; ++x) {
            const float px = (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(kRaycastWidth) - 1.0f) * aspect * tanHalfFov;
            const float py = (1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(kRaycastHeight)) * tanHalfFov;
            const Vec3 dir = normalize3(add3(f, add3(scale3(r, px), scale3(u, py))));

            const Vec3 c = traceRay(gCamera.position, dir);
            const int idx = (y * kRaycastWidth + x) * 3;
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

static void drawText(const int x, const int y, const char* text) {
    glRasterPos2i(x, y);
    for (const char* p = text; *p != '\0'; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
    }
}

static void drawOverlay() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(gWindowWidth), 0.0, static_cast<double>(gWindowHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(0.18f, 0.18f, 0.20f);
    glBegin(GL_QUADS);
    glVertex2i(gButtonX, gButtonY);
    glVertex2i(gButtonX + gButtonW, gButtonY);
    glVertex2i(gButtonX + gButtonW, gButtonY + gButtonH);
    glVertex2i(gButtonX, gButtonY + gButtonH);
    glEnd();

    glColor3f(0.70f, 0.70f, 0.74f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(gButtonX, gButtonY);
    glVertex2i(gButtonX + gButtonW, gButtonY);
    glVertex2i(gButtonX + gButtonW, gButtonY + gButtonH);
    glVertex2i(gButtonX, gButtonY + gButtonH);
    glEnd();

    glColor3f(0.95f, 0.95f, 0.95f);
    if (gUseRaycast) {
        drawText(gButtonX + 12, gButtonY + 12, "Modo: Raycast 3D (clique para Rasterizacao)");
    } else {
        drawText(gButtonX + 12, gButtonY + 12, "Modo: Rasterizacao Leve (clique para Raycast)");
    }

    glColor3f(0.82f, 0.84f, 0.88f);
    drawText(18, 62, "WASD: mover | Setas: olhar | C: Viagem Wormhole (Bezier) | T: Raycast");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void display() {
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
        case 'a':
        case 'A':
            gCamera.position = sub3(gCamera.position, scale3(right, moveStep));
            break;
        case 'd':
        case 'D':
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
    const bool inside =
        x >= gButtonX && x <= gButtonX + gButtonW &&
        yBottom >= gButtonY && yBottom <= gButtonY + gButtonH;

    if (inside) {
        gUseRaycast = !gUseRaycast;
        glutPostRedisplay();
    }
}

static void init() {
    glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    loadTexture("raycast/textura.jpg");
    
    sphereQuadric = gluNewQuadric();
    gluQuadricTexture(sphereQuadric, GL_TRUE);
    gluQuadricNormals(sphereQuadric, GLU_SMOOTH);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutInitWindowPosition(100, 60);
    glutCreateWindow("Wormhole Simulation 3D - Rasterizacao / Raycast (OpenGL 2.1)");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}
