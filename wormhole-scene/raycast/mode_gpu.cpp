#include "wormhole3d_raycast_gpu.h"
#include "wormhole3d_globals.h"
#include "wormhole3d_simulation.h"
#include "wormhole3d_gpu_shaders.h"
#include "scene_gpu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include <GL/gl.h>
#include <GL/glext.h>

#if defined(__linux__)
#include <GL/glx.h>
#include <dlfcn.h>

namespace {

struct GpuGlProcs {
    PFNGLCREATESHADERPROC CreateShader = nullptr;
    PFNGLSHADERSOURCEPROC ShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC CompileShader = nullptr;
    PFNGLGETSHADERIVPROC GetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = nullptr;
    PFNGLDELETESHADERPROC DeleteShader = nullptr;
    PFNGLCREATEPROGRAMPROC CreateProgram = nullptr;
    PFNGLATTACHSHADERPROC AttachShader = nullptr;
    PFNGLLINKPROGRAMPROC LinkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC GetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = nullptr;
    PFNGLDELETEPROGRAMPROC DeleteProgram = nullptr;
    PFNGLUSEPROGRAMPROC UseProgram = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = nullptr;
    PFNGLGETATTRIBLOCATIONPROC GetAttribLocation = nullptr;
    PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation = nullptr;
    PFNGLUNIFORM1FPROC Uniform1f = nullptr;
    PFNGLUNIFORM1FVPROC Uniform1fv = nullptr;
    PFNGLUNIFORM1IPROC Uniform1i = nullptr;
    PFNGLUNIFORM2FPROC Uniform2f = nullptr;
    PFNGLUNIFORM3FPROC Uniform3f = nullptr;
    PFNGLUNIFORM3FVPROC Uniform3fv = nullptr;
    PFNGLACTIVETEXTUREPROC ActiveTexture = nullptr;
    PFNGLGENBUFFERSPROC GenBuffers = nullptr;
    PFNGLBINDBUFFERPROC BindBuffer = nullptr;
    PFNGLBUFFERDATAPROC BufferData = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray = nullptr;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray = nullptr;
    PFNGLGENFRAMEBUFFERSPROC GenFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC BindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers = nullptr;
    PFNGLBLITFRAMEBUFFERPROC BlitFramebuffer = nullptr;
};

GpuGlProcs gGl;

GLuint gRayFbo = 0;
GLuint gRayColorTex = 0;
GLuint gRayProgram = 0;
GLuint gRayVbo = 0;
GLuint gSceneDataTex = 0;
SceneGpuPacked gScenePack;
GLint gLoc_aPos = -1;
GLint gLoc_uCamPos = -1;
GLint gLoc_uRayForward = -1;
GLint gLoc_uRayRight = -1;
GLint gLoc_uRayUp = -1;
GLint gLoc_uResolution = -1;
GLint gLoc_uAspect = -1;
GLint gLoc_uTanHalfFov = -1;
GLint gLoc_uHoleA_center = -1;
GLint gLoc_uHoleA_radius = -1;
GLint gLoc_uHoleA_coreRadius = -1;
GLint gLoc_uHoleA_strength = -1;
GLint gLoc_uHoleB_center = -1;
GLint gLoc_uHoleB_radius = -1;
GLint gLoc_uHoleB_coreRadius = -1;
GLint gLoc_uHoleB_strength = -1;
GLint gLoc_uSceneData = -1;
GLint gLoc_uSceneInvW = -1;
GLint gLoc_uObjectCount = -1;
GLint gLoc_uPointCount = -1;
GLint gLoc_uPointRange0 = -1;
GLint gLoc_uPointPos0 = -1;
GLint gLoc_uPointCol0 = -1;
GLint gLoc_uSunDir = -1;
GLint gLoc_uSunDiffuse = -1;
GLint gLoc_uAmbient = -1;
GLint gLoc_uPointLightScale = -1;
GLint gLoc_uSkyDayFactor = -1;
GLint gLoc_uSceneTimeSec = -1;

void* glResolve(const char* name) {
    void* p = reinterpret_cast<void*>(glXGetProcAddress(reinterpret_cast<const GLubyte*>(name)));
    if (!p) {
        p = dlsym(RTLD_DEFAULT, name);
    }
    return p;
}

bool loadGpuGlProcs() {
    gGl.CreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(glResolve("glCreateShader"));
    gGl.ShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(glResolve("glShaderSource"));
    gGl.CompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(glResolve("glCompileShader"));
    gGl.GetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(glResolve("glGetShaderiv"));
    gGl.GetShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(glResolve("glGetShaderInfoLog"));
    gGl.DeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(glResolve("glDeleteShader"));
    gGl.CreateProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(glResolve("glCreateProgram"));
    gGl.AttachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(glResolve("glAttachShader"));
    gGl.LinkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(glResolve("glLinkProgram"));
    gGl.GetProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(glResolve("glGetProgramiv"));
    gGl.GetProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(glResolve("glGetProgramInfoLog"));
    gGl.DeleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(glResolve("glDeleteProgram"));
    gGl.UseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(glResolve("glUseProgram"));
    gGl.GetUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(glResolve("glGetUniformLocation"));
    gGl.GetAttribLocation = reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(glResolve("glGetAttribLocation"));
    gGl.BindAttribLocation = reinterpret_cast<PFNGLBINDATTRIBLOCATIONPROC>(glResolve("glBindAttribLocation"));
    gGl.Uniform1f = reinterpret_cast<PFNGLUNIFORM1FPROC>(glResolve("glUniform1f"));
    gGl.Uniform1fv = reinterpret_cast<PFNGLUNIFORM1FVPROC>(glResolve("glUniform1fv"));
    gGl.Uniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(glResolve("glUniform1i"));
    gGl.Uniform2f = reinterpret_cast<PFNGLUNIFORM2FPROC>(glResolve("glUniform2f"));
    gGl.Uniform3f = reinterpret_cast<PFNGLUNIFORM3FPROC>(glResolve("glUniform3f"));
    gGl.Uniform3fv = reinterpret_cast<PFNGLUNIFORM3FVPROC>(glResolve("glUniform3fv"));
    gGl.ActiveTexture = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(glResolve("glActiveTexture"));
    gGl.GenBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(glResolve("glGenBuffers"));
    gGl.BindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(glResolve("glBindBuffer"));
    gGl.BufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(glResolve("glBufferData"));
    gGl.VertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(glResolve("glVertexAttribPointer"));
    gGl.EnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(glResolve("glEnableVertexAttribArray"));
    gGl.DisableVertexAttribArray = reinterpret_cast<PFNGLDISABLEVERTEXATTRIBARRAYPROC>(glResolve("glDisableVertexAttribArray"));

    gGl.GenFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(glResolve("glGenFramebuffers"));
    gGl.BindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(glResolve("glBindFramebuffer"));
    gGl.FramebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(glResolve("glFramebufferTexture2D"));
    gGl.CheckFramebufferStatus = reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(glResolve("glCheckFramebufferStatus"));
    gGl.DeleteFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(glResolve("glDeleteFramebuffers"));
    if (!gGl.GenFramebuffers || !gGl.BindFramebuffer || !gGl.FramebufferTexture2D || !gGl.CheckFramebufferStatus ||
        !gGl.DeleteFramebuffers) {
        gGl.GenFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(glResolve("glGenFramebuffersEXT"));
        gGl.BindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(glResolve("glBindFramebufferEXT"));
        gGl.FramebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(glResolve("glFramebufferTexture2DEXT"));
        gGl.CheckFramebufferStatus = reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(glResolve("glCheckFramebufferStatusEXT"));
        gGl.DeleteFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(glResolve("glDeleteFramebuffersEXT"));
    }

    gGl.BlitFramebuffer = reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(glResolve("glBlitFramebuffer"));
    if (!gGl.BlitFramebuffer) {
        gGl.BlitFramebuffer = reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(glResolve("glBlitFramebufferEXT"));
    }

    return gGl.CreateShader && gGl.ShaderSource && gGl.CompileShader && gGl.GetShaderiv && gGl.GetShaderInfoLog &&
           gGl.DeleteShader && gGl.CreateProgram && gGl.AttachShader && gGl.LinkProgram && gGl.GetProgramiv &&
           gGl.GetProgramInfoLog && gGl.DeleteProgram && gGl.UseProgram && gGl.GetUniformLocation &&
           gGl.GetAttribLocation && gGl.BindAttribLocation && gGl.Uniform1f && gGl.Uniform1fv && gGl.Uniform1i &&
           gGl.Uniform2f &&
           gGl.Uniform3f && gGl.Uniform3fv && gGl.ActiveTexture &&
           gGl.GenBuffers && gGl.BindBuffer && gGl.BufferData && gGl.VertexAttribPointer && gGl.EnableVertexAttribArray &&
           gGl.DisableVertexAttribArray && gGl.GenFramebuffers && gGl.BindFramebuffer && gGl.FramebufferTexture2D &&
           gGl.CheckFramebufferStatus && gGl.DeleteFramebuffers && gGl.BlitFramebuffer;
}

GLuint compileGlShader(const GLenum type, const char* src) {
    const GLuint s = gGl.CreateShader(type);
    const GLchar* glsrc = static_cast<const GLchar*>(src);
    gGl.ShaderSource(s, 1, &glsrc, nullptr);
    gGl.CompileShader(s);
    GLint ok = 0;
    gGl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        gGl.GetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "compileGlShader failed:\n" << log << std::endl;
        gGl.DeleteShader(s);
        return 0;
    }
    return s;
}

GLuint linkRaycastGlProgram(const GLuint vs, const GLuint fs) {
    const GLuint p = gGl.CreateProgram();
    gGl.AttachShader(p, vs);
    gGl.AttachShader(p, fs);
    gGl.BindAttribLocation(p, 0, "aPos");
    gGl.LinkProgram(p);
    gGl.DeleteShader(vs);
    gGl.DeleteShader(fs);
    GLint ok = 0;
    gGl.GetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        gGl.GetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::cerr << "linkRaycastGlProgram failed:\n" << log << std::endl;
        gGl.DeleteProgram(p);
        return 0;
    }
    return p;
}

} // namespace

bool initGpuRaycast() {
    if (!loadGpuGlProcs()) {
        std::cerr << "OpenGL 2.1 GPU raycast: entry points not available (loader failed).\n";
        return false;
    }

    const char* ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!ext || std::strstr(ext, "GL_ARB_texture_float") == nullptr) {
        std::cerr << "GPU raycast: GL_ARB_texture_float not found (needed for scene data texture).\n";
        return false;
    }

    const std::string fragCombined =
        std::string("#version 120\n") + kGlslSdfPrimitives + kGlslRaycastFragment;

    const GLuint vs = compileGlShader(GL_VERTEX_SHADER, kVertRaycast);
    const GLuint fs = compileGlShader(GL_FRAGMENT_SHADER, fragCombined.c_str());
    if (!vs || !fs) {
        if (vs) {
            gGl.DeleteShader(vs);
        }
        if (fs) {
            gGl.DeleteShader(fs);
        }
        return false;
    }
    gRayProgram = linkRaycastGlProgram(vs, fs);
    if (!gRayProgram) {
        return false;
    }

    gLoc_aPos = gGl.GetAttribLocation(gRayProgram, "aPos");
    gLoc_uCamPos = gGl.GetUniformLocation(gRayProgram, "uCamPos");
    gLoc_uRayForward = gGl.GetUniformLocation(gRayProgram, "uRayForward");
    gLoc_uRayRight = gGl.GetUniformLocation(gRayProgram, "uRayRight");
    gLoc_uRayUp = gGl.GetUniformLocation(gRayProgram, "uRayUp");
    gLoc_uResolution = gGl.GetUniformLocation(gRayProgram, "uResolution");
    gLoc_uAspect = gGl.GetUniformLocation(gRayProgram, "uAspect");
    gLoc_uTanHalfFov = gGl.GetUniformLocation(gRayProgram, "uTanHalfFov");
    gLoc_uHoleA_center = gGl.GetUniformLocation(gRayProgram, "uHoleA_center");
    gLoc_uHoleA_radius = gGl.GetUniformLocation(gRayProgram, "uHoleA_radius");
    gLoc_uHoleA_coreRadius = gGl.GetUniformLocation(gRayProgram, "uHoleA_coreRadius");
    gLoc_uHoleA_strength = gGl.GetUniformLocation(gRayProgram, "uHoleA_strength");
    gLoc_uHoleB_center = gGl.GetUniformLocation(gRayProgram, "uHoleB_center");
    gLoc_uHoleB_radius = gGl.GetUniformLocation(gRayProgram, "uHoleB_radius");
    gLoc_uHoleB_coreRadius = gGl.GetUniformLocation(gRayProgram, "uHoleB_coreRadius");
    gLoc_uHoleB_strength = gGl.GetUniformLocation(gRayProgram, "uHoleB_strength");
    gLoc_uSceneData = gGl.GetUniformLocation(gRayProgram, "uSceneData");
    gLoc_uSceneInvW = gGl.GetUniformLocation(gRayProgram, "uSceneInvW");
    gLoc_uObjectCount = gGl.GetUniformLocation(gRayProgram, "uObjectCount");
    gLoc_uPointCount = gGl.GetUniformLocation(gRayProgram, "uPointCount");
    gLoc_uPointRange0 = gGl.GetUniformLocation(gRayProgram, "uPointRange[0]");
    gLoc_uPointPos0 = gGl.GetUniformLocation(gRayProgram, "uPointPos[0]");
    gLoc_uPointCol0 = gGl.GetUniformLocation(gRayProgram, "uPointCol[0]");
    gLoc_uSunDir = gGl.GetUniformLocation(gRayProgram, "uSunDir");
    gLoc_uSunDiffuse = gGl.GetUniformLocation(gRayProgram, "uSunDiffuse");
    gLoc_uAmbient = gGl.GetUniformLocation(gRayProgram, "uAmbient");
    gLoc_uPointLightScale = gGl.GetUniformLocation(gRayProgram, "uPointLightScale");
    gLoc_uSkyDayFactor = gGl.GetUniformLocation(gRayProgram, "uSkyDayFactor");
    gLoc_uSceneTimeSec = gGl.GetUniformLocation(gRayProgram, "uSceneTimeSec");

    if (gLoc_aPos < 0 || gLoc_uCamPos < 0 || gLoc_uRayForward < 0 || gLoc_uRayRight < 0 || gLoc_uRayUp < 0 ||
        gLoc_uResolution < 0 || gLoc_uAspect < 0 || gLoc_uTanHalfFov < 0 ||
        gLoc_uHoleA_center < 0 || gLoc_uHoleA_radius < 0 || gLoc_uHoleA_coreRadius < 0 || gLoc_uHoleA_strength < 0 ||
        gLoc_uHoleB_center < 0 || gLoc_uHoleB_radius < 0 || gLoc_uHoleB_coreRadius < 0 || gLoc_uHoleB_strength < 0 ||
        gLoc_uSceneData < 0 || gLoc_uSceneInvW < 0 || gLoc_uObjectCount < 0 || gLoc_uPointCount < 0 ||
        gLoc_uPointRange0 < 0 || gLoc_uPointPos0 < 0 || gLoc_uPointCol0 < 0 || gLoc_uSunDir < 0 ||
        gLoc_uSunDiffuse < 0 || gLoc_uAmbient < 0 || gLoc_uPointLightScale < 0 || gLoc_uSkyDayFactor < 0 ||
        gLoc_uSceneTimeSec < 0) {
        std::cerr << "initGpuRaycast: missing attrib or uniform location\n";
        gGl.DeleteProgram(gRayProgram);
        gRayProgram = 0;
        return false;
    }

    glGenTextures(1, &gSceneDataTex);
    packSceneObjectsForGpu(gScenePack);
    uploadSceneDataTexture(gSceneDataTex, gScenePack.pixels.data());

    static const float kFullScreenTri[6] = {-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};
    gGl.GenBuffers(1, &gRayVbo);
    gGl.BindBuffer(GL_ARRAY_BUFFER, gRayVbo);
    gGl.BufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(kFullScreenTri)), kFullScreenTri, GL_STATIC_DRAW);
    gGl.BindBuffer(GL_ARRAY_BUFFER, 0);

    gGl.GenFramebuffers(1, &gRayFbo);
    glGenTextures(1, &gRayColorTex);
    glBindTexture(GL_TEXTURE_2D, gRayColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, kRaycastWidth, kRaycastHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    gGl.BindFramebuffer(GL_FRAMEBUFFER, gRayFbo);
    gGl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gRayColorTex, 0);
    const GLenum status = gGl.CheckFramebufferStatus(GL_FRAMEBUFFER);
    gGl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "initGpuRaycast: FBO incomplete, status=" << status << std::endl;
        glDeleteTextures(1, &gRayColorTex);
        glDeleteTextures(1, &gSceneDataTex);
        gGl.DeleteFramebuffers(1, &gRayFbo);
        gGl.DeleteProgram(gRayProgram);
        gRayColorTex = gRayFbo = gRayProgram = gSceneDataTex = 0;
        return false;
    }

    return true;
}

void raycastSceneGpu() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    const Vec3 f = rayForward();
    const Vec3 r = rayRight();
    const Vec3 u = rayUp();
    const float aspect = static_cast<float>(kRaycastWidth) / static_cast<float>(kRaycastHeight);
    const float tanHalfFov = std::tan((gCamera.fovViewDegree * 3.14159265359f / 180.0f) * 0.5f);

    gGl.BindFramebuffer(GL_FRAMEBUFFER, gRayFbo);
    glViewport(0, 0, kRaycastWidth, kRaycastHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    gGl.UseProgram(gRayProgram);

    gGl.Uniform3f(gLoc_uCamPos, gCamera.position.x, gCamera.position.y, gCamera.position.z);
    gGl.Uniform3f(gLoc_uRayForward, f.x, f.y, f.z);
    gGl.Uniform3f(gLoc_uRayRight, r.x, r.y, r.z);
    gGl.Uniform3f(gLoc_uRayUp, u.x, u.y, u.z);
    gGl.Uniform2f(gLoc_uResolution, static_cast<float>(kRaycastWidth), static_cast<float>(kRaycastHeight));
    gGl.Uniform1f(gLoc_uAspect, aspect);
    gGl.Uniform1f(gLoc_uTanHalfFov, tanHalfFov);
    gGl.Uniform1f(gLoc_uSceneTimeSec, gSceneTimeSec);

    gGl.Uniform3f(gLoc_uHoleA_center, gWormhole.holeA.center.x, gWormhole.holeA.center.y, gWormhole.holeA.center.z);
    gGl.Uniform1f(gLoc_uHoleA_radius, gWormhole.holeA.warpRadius);
    gGl.Uniform1f(gLoc_uHoleA_coreRadius, gWormhole.holeA.coreRadius);
    gGl.Uniform1f(gLoc_uHoleA_strength, gWormhole.holeA.strength);

    gGl.Uniform3f(gLoc_uHoleB_center, gWormhole.holeB.center.x, gWormhole.holeB.center.y, gWormhole.holeB.center.z);
    gGl.Uniform1f(gLoc_uHoleB_radius, gWormhole.holeB.warpRadius);
    gGl.Uniform1f(gLoc_uHoleB_coreRadius, gWormhole.holeB.coreRadius);
    gGl.Uniform1f(gLoc_uHoleB_strength, gWormhole.holeB.strength);

    {
        DayNightLighting dn;
        computeDayNightLighting(dn);
        gGl.Uniform3f(gLoc_uSunDir, dn.sunDir.x, dn.sunDir.y, dn.sunDir.z);
        gGl.Uniform1f(gLoc_uSunDiffuse, dn.sunDiffuse);
        gGl.Uniform1f(gLoc_uAmbient, dn.ambient);
        gGl.Uniform1f(gLoc_uPointLightScale, dn.pointLightScale);
        gGl.Uniform1f(gLoc_uSkyDayFactor, dn.skyDayFactor);
    }

    packSceneObjectsForGpu(gScenePack);
    uploadSceneDataTexture(gSceneDataTex, gScenePack.pixels.data());
    gGl.ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gSceneDataTex);
    gGl.Uniform1i(gLoc_uSceneData, 0);
    gGl.Uniform1f(gLoc_uSceneInvW, 1.0f / static_cast<float>(kSceneDataWidth));
    gGl.Uniform1i(gLoc_uObjectCount, gScenePack.objectCount);

    {
        float pos[kMaxPointLights * 3] = {};
        float col[kMaxPointLights * 3] = {};
        float rng[kMaxPointLights] = {};
        const int n = std::min(static_cast<int>(gPointLights.size()), kMaxPointLights);
        for (int i = 0; i < n; ++i) {
            const PointLight& L = gPointLights[static_cast<size_t>(i)];
            pos[i * 3 + 0] = L.position.x;
            pos[i * 3 + 1] = L.position.y;
            pos[i * 3 + 2] = L.position.z;
            col[i * 3 + 0] = L.color.x;
            col[i * 3 + 1] = L.color.y;
            col[i * 3 + 2] = L.color.z;
            rng[i] = L.range;
        }
        gGl.Uniform1i(gLoc_uPointCount, n);
        gGl.Uniform1fv(gLoc_uPointRange0, kMaxPointLights, rng);
        gGl.Uniform3fv(gLoc_uPointPos0, kMaxPointLights, pos);
        gGl.Uniform3fv(gLoc_uPointCol0, kMaxPointLights, col);
    }

    gGl.BindBuffer(GL_ARRAY_BUFFER, gRayVbo);
    gGl.VertexAttribPointer(static_cast<GLuint>(gLoc_aPos), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    gGl.EnableVertexAttribArray(static_cast<GLuint>(gLoc_aPos));
    glDrawArrays(GL_TRIANGLES, 0, 3);
    gGl.DisableVertexAttribArray(static_cast<GLuint>(gLoc_aPos));
    gGl.BindBuffer(GL_ARRAY_BUFFER, 0);

    gGl.UseProgram(0);

    gGl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    gGl.BindFramebuffer(GL_READ_FRAMEBUFFER, gRayFbo);
    gGl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    gGl.BlitFramebuffer(
        0, 0, kRaycastWidth, kRaycastHeight,
        0, 0, gWindowWidth, gWindowHeight,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
    gGl.BindFramebuffer(GL_FRAMEBUFFER, 0);
}

#else

bool initGpuRaycast() {
    return false;
}

void raycastSceneGpu() {}

#endif
