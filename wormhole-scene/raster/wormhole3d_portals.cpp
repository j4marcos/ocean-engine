#include "wormhole3d_portals.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"
#include "scene_prefabs.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <iostream>

// ---------------------------------------------------------------------------
//  FBO function pointers (loaded via dlsym on Linux)
// ---------------------------------------------------------------------------
static PFNGLGENFRAMEBUFFERSPROC ptrGenFramebuffers = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC ptrDeleteFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC ptrBindFramebuffer = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC ptrFramebufferTexture2D = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC ptrCheckFramebufferStatus = nullptr;
static PFNGLGENRENDERBUFFERSPROC ptrGenRenderbuffers = nullptr;
static PFNGLDELETERENDERBUFFERSPROC ptrDeleteRenderbuffers = nullptr;
static PFNGLBINDRENDERBUFFERPROC ptrBindRenderbuffer = nullptr;
static PFNGLRENDERBUFFERSTORAGEPROC ptrRenderbufferStorage = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC ptrFramebufferRenderbuffer = nullptr;

static void* loadGlProc(const char* name) {
    // Tenta GLX (libGL), depois libglut, depois fallback no próprio executável
    static bool tried = false;
    static void* hGL = nullptr;
    static void* hGlut = nullptr;
    if (!tried) {
        tried = true;
        hGL = dlopen("libGL.so", RTLD_LAZY);
        // libGL.so já está carregada, dlopen retorna a mesma referência
        hGlut = dlopen("libglut.so", RTLD_LAZY);
    }
    void* sym = nullptr;
    if (hGL) sym = dlsym(hGL, name);
    if (!sym && hGlut) sym = dlsym(hGlut, name);
    if (!sym) sym = dlsym(RTLD_NEXT, name);
    return sym;
}

static bool resolveFBOFunctions() {
    auto resolve = [](const char* core, const char* ext1, const char* ext2) -> void* {
        if (auto fn = loadGlProc(core)) return fn;
        if (auto fn = loadGlProc(ext1)) return fn;
        if (auto fn = loadGlProc(ext2)) return fn;
        return nullptr;
    };

    ptrGenFramebuffers         = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(resolve("glGenFramebuffers","glGenFramebuffersEXT","glGenFramebuffersARB"));
    ptrDeleteFramebuffers      = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(resolve("glDeleteFramebuffers","glDeleteFramebuffersEXT","glDeleteFramebuffersARB"));
    ptrBindFramebuffer         = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(resolve("glBindFramebuffer","glBindFramebufferEXT","glBindFramebufferARB"));
    ptrFramebufferTexture2D    = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(resolve("glFramebufferTexture2D","glFramebufferTexture2DEXT","glFramebufferTexture2DARB"));
    ptrCheckFramebufferStatus  = reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(resolve("glCheckFramebufferStatus","glCheckFramebufferStatusEXT","glCheckFramebufferStatusARB"));
    ptrGenRenderbuffers        = reinterpret_cast<PFNGLGENRENDERBUFFERSPROC>(resolve("glGenRenderbuffers","glGenRenderbuffersEXT","glGenRenderbuffersARB"));
    ptrDeleteRenderbuffers     = reinterpret_cast<PFNGLDELETERENDERBUFFERSPROC>(resolve("glDeleteRenderbuffers","glDeleteRenderbuffersEXT","glDeleteRenderbuffersARB"));
    ptrBindRenderbuffer        = reinterpret_cast<PFNGLBINDRENDERBUFFERPROC>(resolve("glBindRenderbuffer","glBindRenderbufferEXT","glBindRenderbufferARB"));
    ptrRenderbufferStorage     = reinterpret_cast<PFNGLRENDERBUFFERSTORAGEPROC>(resolve("glRenderbufferStorage","glRenderbufferStorageEXT","glRenderbufferStorageARB"));
    ptrFramebufferRenderbuffer = reinterpret_cast<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(resolve("glFramebufferRenderbuffer","glFramebufferRenderbufferEXT","glFramebufferRenderbufferARB"));

    if (!ptrGenFramebuffers || !ptrBindFramebuffer || !ptrFramebufferTexture2D ||
        !ptrCheckFramebufferStatus || !ptrGenRenderbuffers || !ptrBindRenderbuffer ||
        !ptrRenderbufferStorage || !ptrFramebufferRenderbuffer) {
        std::cerr << "Could not resolve all FBO extension functions.\n";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------
static Vec3 v3_add(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 v3_sub(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 v3_scale(const Vec3& v, float s) { return {v.x * s, v.y * s, v.z * s}; }
static float v3_len(const Vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
static Vec3 v3_normalize(const Vec3& v) {
    float l = v3_len(v);
    if (l < 1e-6f) return {0.0f, 0.0f, 1.0f};
    return {v.x / l, v.y / l, v.z / l};
}
static Vec3 v3_cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static Vec3 holeCenter(int idx) {
    return idx == 0 ? gWormhole.holeA.center : gWormhole.holeB.center;
}
static float holeRadius(int idx) {
    return idx == 0 ? gWormhole.holeA.warpRadius : gWormhole.holeB.warpRadius;
}
// FBO target — preferimos EXT, fallback para core
static GLenum sFbTarget = 0;
static GLenum fbTarget() {
    if (sFbTarget == 0) {
        // Verifica se a ext está disponível
        const char* exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        if (exts && std::strstr(exts, "GL_EXT_framebuffer_object")) {
            sFbTarget = GL_FRAMEBUFFER_EXT;
        } else {
            sFbTarget = GL_FRAMEBUFFER;
        }
    }
    return sFbTarget;
}

static GLenum depthAttach() {
    const char* exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (exts && std::strstr(exts, "GL_EXT_framebuffer_object")) {
        return GL_DEPTH_ATTACHMENT_EXT;
    }
    return GL_DEPTH_ATTACHMENT;
}
static GLenum renderbufferTarget() {
    const char* exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (exts && std::strstr(exts, "GL_EXT_framebuffer_object")) {
        return GL_RENDERBUFFER_EXT;
    }
    return GL_RENDERBUFFER;
}
static GLenum fbCompleteValue() {
    const char* exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (exts && std::strstr(exts, "GL_EXT_framebuffer_object")) {
        return GL_FRAMEBUFFER_COMPLETE_EXT;
    }
    return GL_FRAMEBUFFER_COMPLETE;
}

// ---------------------------------------------------------------------------
//  Init / Destroy
// ---------------------------------------------------------------------------
bool initPortalFBOs(PortalFBO& a, PortalFBO& b) {
    if (!resolveFBOFunctions()) {
        return false;
    }

    const GLenum fb = fbTarget();
    const GLenum rb = renderbufferTarget();
    const GLenum da = depthAttach();
    const GLenum ok = fbCompleteValue();

    auto makeFBO = [fb, rb, da, ok]() -> std::pair<GLuint, GLuint> {
        GLuint tex = 0, fbo = 0, rbo = 0;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, kPortalTexSize, kPortalTexSize, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        ptrGenRenderbuffers(1, &rbo);
        ptrBindRenderbuffer(rb, rbo);
        ptrRenderbufferStorage(rb, GL_DEPTH_COMPONENT, kPortalTexSize, kPortalTexSize);

        ptrGenFramebuffers(1, &fbo);
        ptrBindFramebuffer(fb, fbo);
        ptrFramebufferTexture2D(fb, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        ptrFramebufferRenderbuffer(fb, da, rb, rbo);

        GLint status = ptrCheckFramebufferStatus(fb);
        if (static_cast<GLenum>(status) != ok) {
            std::cerr << "Portal FBO incomplete (status=" << status << ")\n";
            if (fbo) ptrDeleteFramebuffers(1, &fbo);
            if (tex) glDeleteTextures(1, &tex);
            if (rbo) ptrDeleteRenderbuffers(1, &rbo);
            return {0, 0};
        }

        ptrBindFramebuffer(fb, 0);
        return {fbo, tex};
    };

    auto fbRb = makeFBO();
    if (fbRb.first == 0) return false;
    auto fbRb2 = makeFBO();
    if (fbRb2.first == 0) {
        ptrDeleteFramebuffers(1, &fbRb.first);
        glDeleteTextures(1, &fbRb.second);
        return false;
    }

    a = {fbRb.first, fbRb.second, 0};
    b = {fbRb2.first, fbRb2.second, 0};
    return true;
}

void destroyPortalFBO(const PortalFBO& p) {
    if (p.fbo) ptrDeleteFramebuffers(1, &p.fbo);
    if (p.tex) glDeleteTextures(1, &p.tex);
}

// ---------------------------------------------------------------------------
//  Render portal view
// ---------------------------------------------------------------------------
void renderPortalView(int idx, const PortalFBO& fbo) {
    const int oppositeIdx = 1 - idx;
    const Vec3 oppositeCenter = holeCenter(oppositeIdx);
    const Vec3 myCenter = holeCenter(idx);

    // Normal do portal (apontando para a câmera real, lado de fora do disco)
    Vec3 portalNormal = v3_normalize(v3_sub(gCamera.position, myCenter));

    // Offset da câmera real em relação ao centro do portal
    const Vec3 camOffset = v3_sub(gCamera.position, myCenter);

    // Decompor offset em componentes paralela e perpendicular ao portal
    const float dotN = camOffset.x * portalNormal.x + camOffset.y * portalNormal.y + camOffset.z * portalNormal.z;
    const Vec3 parallelComp = v3_scale(portalNormal, dotN);
    const Vec3 lateralComp = v3_sub(camOffset, parallelComp);

    // Câmera virtual fica do lado oposto, na mesma distância perpendicular,
    // e com o MESMO deslocamento lateral (preserva o ângulo relativo ao portal).
    // A normal aponta para FORA do portal oposto, então a câmera fica em:
    //   oppositeCenter - parallelComp * (para frente do portal) + lateralComp
    const Vec3 virtualCam = v3_add(oppositeCenter, v3_sub(lateralComp, v3_scale(portalNormal, dotN * 0.15f)));

    // Direção do olhar da câmera real em coordenadas do mundo
    const Vec3 eyeDir = v3_normalize(rayForward());

    // Projeção do eyeDir no portal: flip na componente paralela ao normal do portal
    // Isso faz com que a câmera virtual olhe na direção espelhada pelo portal
    const float eyeDotN = eyeDir.x * portalNormal.x + eyeDir.y * portalNormal.y + eyeDir.z * portalNormal.z;
    const Vec3 virtualEyeDir = v3_sub(eyeDir, v3_scale(portalNormal, 2.0f * eyeDotN));

    const Vec3 virtualTarget = v3_add(virtualCam, virtualEyeDir);

    // Up vector: projetar o world-up no plano do portal
    const Vec3 worldUp = {0.0f, 1.0f, 0.0f};
    const float upDotN = portalNormal.y;
    Vec3 projUp = v3_sub(worldUp, v3_scale(portalNormal, upDotN));
    if (v3_len(projUp) < 0.01f) projUp = {0.0f, 0.0f, 1.0f};
    else projUp = v3_normalize(projUp);

    // Salva viewport
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    const GLenum fb = fbTarget();

    ptrBindFramebuffer(fb, fbo.fbo);
    glViewport(0, 0, kPortalTexSize, kPortalTexSize);

    DayNightLighting dn;
    computeDayNightLighting(dn);
    const float day = dn.skyDayFactor;
    glClearColor(0.02f + 0.14f * day, 0.03f + 0.18f * day, 0.06f + 0.24f * day, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(gCamera.fovViewDegree, 1.0, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(
        virtualCam.x, virtualCam.y, virtualCam.z,
        virtualTarget.x, virtualTarget.y, virtualTarget.z,
        projUp.x, projUp.y, projUp.z);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    // Chão
    glColor3f(kSceneFloorMaterial.r, kSceneFloorMaterial.g, kSceneFloorMaterial.b);
    const float y = kSceneGroundY;
    const float e = 80.0f;
    glBegin(GL_QUADS);
    glVertex3f(-e, y, -e);
    glVertex3f(e, y, -e);
    glVertex3f(e, y, e);
    glVertex3f(-e, y, e);
    glEnd();

    // Esferas
    for (size_t i = 0; i < gSpheres.size(); ++i) {
        const RGBA& c = gSpheres[i].color;
        glColor3f(c.r, c.g, c.b);
        glPushMatrix();
        glTranslatef(gSpheres[i].center.x, gSpheres[i].center.y, gSpheres[i].center.z);
        gluSphere(sphereQuadric, gSpheres[i].radius, 32, 24);
        glPopMatrix();
    }

    // Caixas
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

    glPopMatrix(); // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix(); // projection
    glMatrixMode(GL_MODELVIEW);

    ptrBindFramebuffer(fb, 0);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
}

// ---------------------------------------------------------------------------
//  Billboard disco texturizado
// ---------------------------------------------------------------------------
static GLuint buildPortalDiscList(int idx) {
    const float r = holeRadius(idx);
    const int segments = 48;

    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);

    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.5f, 0.5f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= segments; ++i) {
        const float angle = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(segments);
        const float cx = std::cos(angle) * r;
        const float sy = std::sin(angle) * r;
        glTexCoord2f(0.5f + cx / (2.0f * r), 0.5f + sy / (2.0f * r));
        glVertex3f(cx, sy, 0.0f);
    }
    glEnd();

    glEndList();
    return list;
}

void drawPortalBillboard(int idx, GLuint portalTex) {
    const Vec3 center = holeCenter(idx);

    // Vetor portal → câmera (sempre de frente)
    Vec3 forward = v3_normalize(v3_sub(gCamera.position, center));

    // Right: perpendicular a forward e worldUp
    const Vec3 worldUp = {0.0f, 1.0f, 0.0f};
    Vec3 right;
    if (std::fabs(forward.y) > 0.95f) {
        right = {1.0f, 0.0f, 0.0f};
    } else {
        right = v3_normalize(v3_cross(worldUp, forward));
    }
    Vec3 up = v3_cross(forward, right);

    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);

    // Matriz de modelo: colunas = right, up, forward (base ortonormal orientada para a câmera)
    const GLdouble mat[16] = {
        static_cast<GLdouble>(right.x), static_cast<GLdouble>(up.x), static_cast<GLdouble>(forward.x), 0.0,
        static_cast<GLdouble>(right.y), static_cast<GLdouble>(up.y), static_cast<GLdouble>(forward.y), 0.0,
        static_cast<GLdouble>(right.z), static_cast<GLdouble>(up.z), static_cast<GLdouble>(forward.z), 0.0,
        0.0, 0.0, 0.0, 1.0
    };
    glMultMatrixd(mat);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, portalTex);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    static GLuint discListA = 0;
    static GLuint discListB = 0;
    GLuint& discList = (idx == 0) ? discListA : discListB;
    if (discList == 0) {
        discList = buildPortalDiscList(idx);
    }
    glCallList(discList);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// ---------------------------------------------------------------------------
//  Render ambos os portais
// ---------------------------------------------------------------------------
void renderPortals() {
    static PortalFBO fboA = {};
    static PortalFBO fboB = {};
    static bool initialized = false;

    if (!initialized) {
        if (!initPortalFBOs(fboA, fboB)) {
            std::cerr << "Portal FBO init failed; continuing without portals.\n";
            initialized = true;
            return;
        }
        initialized = true;
    }

    const GLenum fb = fbTarget();

    renderPortalView(0, fboA);
    renderPortalView(1, fboB);

    ptrBindFramebuffer(fb, 0);

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    drawPortalBillboard(0, fboA.tex);
    drawPortalBillboard(1, fboB.tex);
}
