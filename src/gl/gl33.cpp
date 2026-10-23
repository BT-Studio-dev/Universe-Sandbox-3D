// ============================================================================
//  Universe Sandbox 3D - OpenGL 3.3 loader (implementation)
// ============================================================================
#include "gl/gl33.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#  include <windows.h>
static HMODULE g_openglLib = nullptr;
#endif

namespace gl33 {

// ---------------------------------------------------------------------------
//  Function pointers (initialized to null)
// ---------------------------------------------------------------------------
#define US3D_GL_DECL(name, type) type name = nullptr;
US3D_GL_DECL(glClear, PFNGLCLEARPROC)
US3D_GL_DECL(glClearColor, PFNGLCLEARCOLORPROC)
US3D_GL_DECL(glClearDepth, PFNGLCLEARDEPTHPROC)
US3D_GL_DECL(glViewport, PFNGLVIEWPORTPROC)
US3D_GL_DECL(glEnable, PFNGLENABLEPROC)
US3D_GL_DECL(glDisable, PFNGLDISABLEPROC)
US3D_GL_DECL(glDepthRange, PFNGLDEPTHRANGEPROC)
US3D_GL_DECL(glDepthMask, PFNGLDEPTHMASKPROC)
US3D_GL_DECL(glBlendFunc, PFNGLBLENDFUNCPROC)
US3D_GL_DECL(glDepthFunc, PFNGLDEPTHFUNCPROC)
US3D_GL_DECL(glCullFace, PFNGLCULLFACEPROC)
US3D_GL_DECL(glFrontFace, PFNGLFRONTFACEPROC)
US3D_GL_DECL(glLineWidth, PFNGLLINEWIDTHPROC)
US3D_GL_DECL(glPointSize, PFNGLPOINTSIZEPROC)
US3D_GL_DECL(glFlush, PFNGLFLUSHPROC)
US3D_GL_DECL(glBlendEquation, PFNGLBLENDEQUATIONPROC)
US3D_GL_DECL(glCreateProgram, PFNGLCREATEPROGRAMPROC)
US3D_GL_DECL(glDeleteProgram, PFNGLDELETEPROGRAMPROC)
US3D_GL_DECL(glUseProgram, PFNGLUSEPROGRAMPROC)
US3D_GL_DECL(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC)
US3D_GL_DECL(glUniform1f, PFNGLUNIFORM1FPROC)
US3D_GL_DECL(glUniform1i, PFNGLUNIFORM1IPROC)
US3D_GL_DECL(glUniform2f, PFNGLUNIFORM2FPROC)
US3D_GL_DECL(glUniform3f, PFNGLUNIFORM3FPROC)
US3D_GL_DECL(glUniform4f, PFNGLUNIFORM4FPROC)
US3D_GL_DECL(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC)
US3D_GL_DECL(glUniform2i, PFNGLUNIFORM2IPROC)
US3D_GL_DECL(glUniform1iv, PFNGLUNIFORM1IVPROC)
US3D_GL_DECL(glUniform3iv, PFNGLUNIFORM3IVPROC)
US3D_GL_DECL(glCreateShader, PFNGLCREATESHADERPROC)
US3D_GL_DECL(glDeleteShader, PFNGLDELETESHADERPROC)
US3D_GL_DECL(glShaderSource, PFNGLSHADERSOURCEPROC)
US3D_GL_DECL(glCompileShader, PFNGLCOMPILESHADERPROC)
US3D_GL_DECL(glGetShaderiv, PFNGLGETSHADERIVPROC)
US3D_GL_DECL(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC)
US3D_GL_DECL(glAttachShader, PFNGLATTACHSHADERPROC)
US3D_GL_DECL(glLinkProgram, PFNGLLINKPROGRAMPROC)
US3D_GL_DECL(glGetProgramiv, PFNGLGETPROGRAMIVPROC)
US3D_GL_DECL(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC)
US3D_GL_DECL(glDetachShader, PFNGLDETACHSHADERPROC)
US3D_GL_DECL(glGenBuffers, PFNGLGENBUFFERSPROC)
US3D_GL_DECL(glDeleteBuffers, PFNGLDELETEBUFFERSPROC)
US3D_GL_DECL(glBindBuffer, PFNGLBINDBUFFERPROC)
US3D_GL_DECL(glBufferData, PFNGLBUFFERDATAPROC)
US3D_GL_DECL(glBufferSubData, PFNGLBUFFERSUBDATAPROC)
US3D_GL_DECL(glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC)
US3D_GL_DECL(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC)
US3D_GL_DECL(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC)
US3D_GL_DECL(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC)
US3D_GL_DECL(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC)
US3D_GL_DECL(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC)
US3D_GL_DECL(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC)
US3D_GL_DECL(glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC)
US3D_GL_DECL(glVertexAttrib2f, PFNGLVERTEXATTRIB2FPROC)
US3D_GL_DECL(glVertexAttrib3f, PFNGLVERTEXATTRIB3FPROC)
US3D_GL_DECL(glVertexAttrib4f, PFNGLVERTEXATTRIB4FPROC)
US3D_GL_DECL(glDrawArrays, PFNGLDRAWARRAYSPROC)
US3D_GL_DECL(glDrawElements, PFNGLDRAWELEMENTSPROC)
US3D_GL_DECL(glDrawArraysInstanced, PFNGLDRAWARRAYSINSTANCEDPROC)
US3D_GL_DECL(glDrawElementsInstanced, PFNGLDRAWELEMENTSINSTANCEDPROC)
US3D_GL_DECL(glVertexAttribDivisor, PFNGLVERTEXATTRIBDIVISORPROC)
US3D_GL_DECL(glGenTextures, PFNGLGENTEXTURESPROC)
US3D_GL_DECL(glDeleteTextures, PFNGLDELETETEXTURESPROC)
US3D_GL_DECL(glBindTexture, PFNGLBINDTEXTUREPROC)
US3D_GL_DECL(glActiveTexture, PFNGLACTIVETEXTUREPROC)
US3D_GL_DECL(glTexImage2D, PFNGLTEXIMAGE2DPROC)
US3D_GL_DECL(glTexSubImage2D, PFNGLTEXSUBIMAGE2DPROC)
US3D_GL_DECL(glTexParameteri, PFNGLTEXPARAMETERIPROC)
US3D_GL_DECL(glTexParameterf, PFNGLTEXPARAMETERFPROC)
US3D_GL_DECL(glTexStorage2D, PFNGLTEXSTORAGE2DPROC)
US3D_GL_DECL(glGenerateMipmap, PFNGLGENERATEMIPMAPPROC)
US3D_GL_DECL(glGetString, PFNGLGETSTRINGPROC2)
US3D_GL_DECL(glGetIntegerv, PFNGLGETINTEGERVPROC)
US3D_GL_DECL(glGetError, PFNGLGETERRORPROC)
US3D_GL_DECL(wglCreateContext, PFNWGLCREATECONTEXTPROC)
US3D_GL_DECL(wglCreateContextAttribsARB, PFNWGLCREATECONTEXTATTRIBSARBPROC)
US3D_GL_DECL(wglCreateContextAttribs, PFNWGLCREATECONTEXTATTRIBPROC)
US3D_GL_DECL(wglMakeCurrent, PFNWGLMAKECURRENTPROC)
US3D_GL_DECL(wglGetCurrentContext, PFNWGLGETCURRENTCONTEXTPROC)
US3D_GL_DECL(wglDeleteContext, PFNWGLDELETECONTEXTPROC)
#undef US3D_GL_DECL

// ---------------------------------------------------------------------------
namespace detail {

void* resolve(const char* name) {
#ifdef _WIN32
    if (g_openglLib == nullptr) {
        g_openglLib = LoadLibraryA("opengl32.dll");
        if (g_openglLib == nullptr) return nullptr;
    }
    void* p = reinterpret_cast<void*>(
        reinterpret_cast<FARPROC>(
            ::GetProcAddress(g_openglLib, name)));
    if (p) return p;
    // Some very old drivers only expose a handful of entry points through
    // GetProcAddress; everything else comes from wglGetProcAddress.
    PFNWGLGETPROCADDRESSPROC wgl = reinterpret_cast<PFNWGLGETPROCADDRESSPROC>(
        ::GetProcAddress(g_openglLib, "wglGetProcAddress"));
    if (wgl) {
        p = wgl(name);
        if (p) return p;
    }
    return nullptr;
#else
    (void)name;
    return nullptr;
#endif
}

} // namespace detail
// ---------------------------------------------------------------------------

// (name, pointer-slot) table; one pass fills every entry point.
struct LoaderTable {
    const char* name;
    void** slot;
};

static void loadTable(const LoaderTable* tbl, size_t n, bool& allOk) {
    for (size_t i = 0; i < n; ++i) {
        *tbl[i].slot = detail::resolve(tbl[i].name);
        if (!*tbl[i].slot) allOk = false;
    }
}

bool load() {
    bool allOk = true;
    const LoaderTable tbl[] = {
        { "glClear",                reinterpret_cast<void**>(&glClear) },
        { "glClearColor",           reinterpret_cast<void**>(&glClearColor) },
        { "glClearDepth",           reinterpret_cast<void**>(&glClearDepth) },
        { "glViewport",             reinterpret_cast<void**>(&glViewport) },
        { "glEnable",               reinterpret_cast<void**>(&glEnable) },
        { "glDisable",              reinterpret_cast<void**>(&glDisable) },
        { "glDepthRange",           reinterpret_cast<void**>(&glDepthRange) },
        { "glDepthMask",            reinterpret_cast<void**>(&glDepthMask) },
        { "glBlendFunc",            reinterpret_cast<void**>(&glBlendFunc) },
        { "glDepthFunc",            reinterpret_cast<void**>(&glDepthFunc) },
        { "glCullFace",             reinterpret_cast<void**>(&glCullFace) },
        { "glFrontFace",            reinterpret_cast<void**>(&glFrontFace) },
        { "glLineWidth",            reinterpret_cast<void**>(&glLineWidth) },
        { "glPointSize",            reinterpret_cast<void**>(&glPointSize) },
        { "glFlush",                reinterpret_cast<void**>(&glFlush) },
        { "glBlendEquation",        reinterpret_cast<void**>(&glBlendEquation) },
        { "glCreateProgram",        reinterpret_cast<void**>(&glCreateProgram) },
        { "glDeleteProgram",        reinterpret_cast<void**>(&glDeleteProgram) },
        { "glUseProgram",           reinterpret_cast<void**>(&glUseProgram) },
        { "glGetUniformLocation",   reinterpret_cast<void**>(&glGetUniformLocation) },
        { "glUniform1f",            reinterpret_cast<void**>(&glUniform1f) },
        { "glUniform1i",            reinterpret_cast<void**>(&glUniform1i) },
        { "glUniform2f",            reinterpret_cast<void**>(&glUniform2f) },
        { "glUniform3f",            reinterpret_cast<void**>(&glUniform3f) },
        { "glUniform4f",            reinterpret_cast<void**>(&glUniform4f) },
        { "glUniformMatrix4fv",     reinterpret_cast<void**>(&glUniformMatrix4fv) },
        { "glUniform2i",            reinterpret_cast<void**>(&glUniform2i) },
        { "glUniform1iv",           reinterpret_cast<void**>(&glUniform1iv) },
        { "glUniform3iv",           reinterpret_cast<void**>(&glUniform3iv) },
        { "glCreateShader",         reinterpret_cast<void**>(&glCreateShader) },
        { "glDeleteShader",         reinterpret_cast<void**>(&glDeleteShader) },
        { "glShaderSource",         reinterpret_cast<void**>(&glShaderSource) },
        { "glCompileShader",        reinterpret_cast<void**>(&glCompileShader) },
        { "glGetShaderiv",          reinterpret_cast<void**>(&glGetShaderiv) },
        { "glGetShaderInfoLog",     reinterpret_cast<void**>(&glGetShaderInfoLog) },
        { "glAttachShader",         reinterpret_cast<void**>(&glAttachShader) },
        { "glLinkProgram",          reinterpret_cast<void**>(&glLinkProgram) },
        { "glGetProgramiv",         reinterpret_cast<void**>(&glGetProgramiv) },
        { "glGetProgramInfoLog",    reinterpret_cast<void**>(&glGetProgramInfoLog) },
        { "glDetachShader",         reinterpret_cast<void**>(&glDetachShader) },
        { "glGenBuffers",           reinterpret_cast<void**>(&glGenBuffers) },
        { "glDeleteBuffers",        reinterpret_cast<void**>(&glDeleteBuffers) },
        { "glBindBuffer",           reinterpret_cast<void**>(&glBindBuffer) },
        { "glBufferData",           reinterpret_cast<void**>(&glBufferData) },
        { "glBufferSubData",        reinterpret_cast<void**>(&glBufferSubData) },
        { "glGetBufferParameteriv", reinterpret_cast<void**>(&glGetBufferParameteriv) },
        { "glGenVertexArrays",      reinterpret_cast<void**>(&glGenVertexArrays) },
        { "glDeleteVertexArrays",   reinterpret_cast<void**>(&glDeleteVertexArrays) },
        { "glBindVertexArray",      reinterpret_cast<void**>(&glBindVertexArray) },
        { "glEnableVertexAttribArray", reinterpret_cast<void**>(&glEnableVertexAttribArray) },
        { "glDisableVertexAttribArray", reinterpret_cast<void**>(&glDisableVertexAttribArray) },
        { "glVertexAttribPointer",  reinterpret_cast<void**>(&glVertexAttribPointer) },
        { "glVertexAttrib1f",       reinterpret_cast<void**>(&glVertexAttrib1f) },
        { "glVertexAttrib2f",       reinterpret_cast<void**>(&glVertexAttrib2f) },
        { "glVertexAttrib3f",       reinterpret_cast<void**>(&glVertexAttrib3f) },
        { "glVertexAttrib4f",       reinterpret_cast<void**>(&glVertexAttrib4f) },
        { "glDrawArrays",           reinterpret_cast<void**>(&glDrawArrays) },
        { "glDrawElements",         reinterpret_cast<void**>(&glDrawElements) },
        { "glDrawArraysInstanced",  reinterpret_cast<void**>(&glDrawArraysInstanced) },
        { "glDrawElementsInstanced", reinterpret_cast<void**>(&glDrawElementsInstanced) },
        { "glVertexAttribDivisor",  reinterpret_cast<void**>(&glVertexAttribDivisor) },
        { "glGenTextures",          reinterpret_cast<void**>(&glGenTextures) },
        { "glDeleteTextures",       reinterpret_cast<void**>(&glDeleteTextures) },
        { "glBindTexture",          reinterpret_cast<void**>(&glBindTexture) },
        { "glActiveTexture",        reinterpret_cast<void**>(&glActiveTexture) },
        { "glTexImage2D",           reinterpret_cast<void**>(&glTexImage2D) },
        { "glTexSubImage2D",        reinterpret_cast<void**>(&glTexSubImage2D) },
        { "glTexParameteri",        reinterpret_cast<void**>(&glTexParameteri) },
        { "glTexParameterf",        reinterpret_cast<void**>(&glTexParameterf) },
        { "glTexStorage2D",         reinterpret_cast<void**>(&glTexStorage2D) },
        { "glGenerateMipmap",       reinterpret_cast<void**>(&glGenerateMipmap) },
        { "glGetString",            reinterpret_cast<void**>(&glGetString) },
        { "glGetIntegerv",          reinterpret_cast<void**>(&glGetIntegerv) },
        { "glGetError",             reinterpret_cast<void**>(&glGetError) },
        { "wglCreateContext",       reinterpret_cast<void**>(&wglCreateContext) },
        { "wglCreateContextAttribsARB", reinterpret_cast<void**>(&wglCreateContextAttribsARB) },
        { "wglCreateContextAttribs", reinterpret_cast<void**>(&wglCreateContextAttribs) },
        { "wglMakeCurrent",         reinterpret_cast<void**>(&wglMakeCurrent) },
        { "wglGetCurrentContext",   reinterpret_cast<void**>(&wglGetCurrentContext) },
        { "wglDeleteContext",       reinterpret_cast<void**>(&wglDeleteContext) },
    };
    loadTable(tbl, sizeof(tbl) / sizeof(tbl[0]), allOk);
    return allOk;
}

std::string versionString() {
    if (!glGetString || !glGetString(GL_VERSION)) return "<no GL context>";
    return reinterpret_cast<const char*>(glGetString(GL_VERSION));
}

static std::string queryString(GLenum name) {
    if (!glGetString) return "<no GL context>";
    const char* s = reinterpret_cast<const char*>(glGetString(name));
    return s ? s : "<null>";
}

std::string vendorString()   { return queryString(GL_VENDOR); }
std::string rendererString() { return queryString(GL_RENDERER); }

bool isCompatible() {
    // Parse the major/minor from "x.y.z vendor...".
    const std::string v = versionString();
    int major = 0, minor = 0;
    const char* p = v.c_str();
    while (*p >= '0' && *p <= '9') { major = major * 10 + (*p - '0'); ++p; }
    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9') { minor = minor * 10 + (*p - '0'); ++p; }
    }
    return major > 3 || (major == 3 && minor >= 3);
}

} // namespace gl33
