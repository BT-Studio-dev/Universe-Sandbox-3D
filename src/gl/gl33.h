// ============================================================================
//  Universe Sandbox 3D - self-contained OpenGL 3.3 core-profile loader
//
//  Instead of GLEW / system GL headers we declare the small subset of the
//  GL 3.3 API the project uses and resolve every entry point at runtime via
//  wglGetProcAddress (with a GetProcAddress fallback). This keeps the build
//  dependency-free and stable across MSVC / MinGW and Windows 7 .. 11.
//
//  Call gl33::load() AFTER wglMakeCurrent() on a fresh context.
// ============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif



// ---------------------------------------------------------------------------
//  Core types
// ---------------------------------------------------------------------------
typedef unsigned int    GLuint;
typedef int             GLint;
typedef unsigned int    GLenum;
typedef int             GLsizei;
typedef int             GLboolean;
typedef float           GLfloat;
typedef float           GLclampf;
typedef void            GLvoid;
typedef ptrdiff_t       GLsizeiptr;
typedef ptrdiff_t       GLintptr;
typedef uint16_t        GLushort;
typedef char            GLchar;
typedef unsigned char   GLubyte;
typedef double          GLdouble;

typedef void* HGLRC;   // opaque GL context handle (Windows: HGLRC)
typedef void* HDC2;    // opaque device context handle (Windows: HDC)

// ---------------------------------------------------------------------------
//  Constants (the subset this project uses)
// ---------------------------------------------------------------------------
enum {
    GL_FALSE = 0,
    GL_TRUE  = 1,
};

// buffers / clear
enum {
    GL_DEPTH_BUFFER_BIT   = 0x0100,
    GL_STENCIL_BUFFER_BIT = 0x0400,
    GL_COLOR_BUFFER_BIT   = 0x4000,

    // enables
    GL_DEPTH_TEST     = 0x0B71,
    GL_BLEND          = 0x0BE2,
    GL_CULL_FACE      = 0x0B44,
    GL_LINE_SMOOTH    = 0x0B20,
    GL_PROGRAM_POINT_SIZE = 0x8642,

    // depth / comparison
    GL_NEVER          = 0x0200,
    GL_LESS           = 0x0201,
    GL_EQUAL          = 0x0202,
    GL_LEQUAL         = 0x0203,
    GL_GREATER        = 0x0204,
    GL_GEQUAL         = 0x0206,
    GL_ALWAYS         = 0x0207,

    // blend
    GL_ZERO           = 0,
    GL_ONE            = 1,
    GL_SRC_COLOR      = 0x0300,
    GL_ONE_MINUS_SRC_COLOR = 0x0301,
    GL_SRC_ALPHA      = 0x0302,
    GL_ONE_MINUS_SRC_ALPHA = 0x0303,
    GL_DST_ALPHA      = 0x0304,
    GL_ONE_MINUS_DST_ALPHA = 0x0305,

    // primitives
    GL_POINTS         = 0x0000,
    GL_LINES          = 0x0001,
    GL_LINE_STRIP     = 0x0003,
    GL_LINE_LOOP      = 0x0002,
    GL_TRIANGLES      = 0x0004,
    GL_TRIANGLE_STRIP = 0x0005,
    GL_TRIANGLE_FAN   = 0x0006,

    // buffers
    GL_ARRAY_BUFFER             = 0x8892,
    GL_ELEMENT_ARRAY_BUFFER     = 0x8893,
    GL_UNIFORM_BUFFER           = 0x8A11,
    GL_STATIC_DRAW              = 0x88E4,
    GL_DYNAMIC_DRAW             = 0x88E8,
    GL_STREAM_DRAW              = 0x88E0,
    GL_BUFFER_SIZE              = 0x8764,

    // vertex data
    GL_BYTE           = 0x1400,
    GL_UNSIGNED_BYTE  = 0x1401,
    GL_SHORT          = 0x1402,
    GL_UNSIGNED_SHORT = 0x1403,
    GL_INT            = 0x1404,
    GL_UNSIGNED_INT   = 0x1405,
    GL_FLOAT          = 0x1406,
    GL_INT_2_10_10_10_REV = 0x8D9F,

    // textures
    GL_TEXTURE_1D         = 0x0DE0,
    GL_TEXTURE_2D         = 0x0DE1,
    GL_TEXTURE_3D         = 0x806F,
    GL_TEXTURE_2D_ARRAY   = 0x8C1A,
    GL_TEXTURE_2D_MULTISAMPLE = 0x9100,
    GL_RGBA               = 0x1908,
    GL_BGRA               = 0x80E1,
    GL_RGB                  = 0x1907,
    GL_RGBA8                = 0x8058,
    GL_R8                   = 0x8229,
    GL_RG8                  = 0x822B,
    GL_TEXTURE_MIN_FILTER   = 0x2801,
    GL_TEXTURE_MAG_FILTER   = 0x2800,
    GL_TEXTURE_WRAP_S       = 0x2802,
    GL_TEXTURE_WRAP_T       = 0x2803,
    GL_NEAREST              = 0x2600,
    GL_LINEAR               = 0x2601,
    GL_NEAREST_MIPMAP_LINEAR = 0x2687,
    GL_LINEAR_MIPMAP_LINEAR = 0x2691,
    GL_CLAMP_TO_EDGE        = 0x812F,
    GL_REPEAT               = 0x2901,
    GL_TEXTURE0             = 0x84C0,

    // shaders / programs
    GL_FRAGMENT_SHADER          = 0x8B30,
    GL_VERTEX_SHADER            = 0x8B31,
    GL_COMPILE_STATUS           = 0x8B81,
    GL_INFO_LOG_LENGTH          = 0x8B84,
    GL_LINK_STATUS              = 0x8B82,
    GL_ATTACHED_SHADERS         = 0x8B85,
    GL_ACTIVE_UNIFORM           = 0x8B86,
    GL_ACTIVE_UNIFORMS          = 0x8B86,

    // queries
    GL_VENDOR         = 0x1F00,
    GL_VERSION        = 0x1F01,
    GL_RENDERER       = 0x1F02,
    GL_SHADING_LANGUAGE_VERSION = 0x8B8C,
    GL_NUM_EXTENSIONS = 0x1F03,
    GL_MAX_TEXTURE_SIZE = 0x0D33,
    GL_MAX_VIEWPORT_DIMS = 0x0D3A,

    // face culling
    GL_FRONT          = 0x0404,
    GL_BACK           = 0x0405,
    GL_FRONT_AND_BACK = 0x0408,
    GL_CW             = 0x0900,
    GL_CCW            = 0x0901,

    // errors
    GL_NO_ERROR       = 0,
};

// WGL (Windows GL) extension enums for context creation.
enum {
    WGL_CONTEXT_MAJOR_VERSION_ARB        = 0x2091,
    WGL_CONTEXT_MINOR_VERSION_ARB        = 0x2092,
    WGL_CONTEXT_ES2_PROFILE_BIT_ARB      = 0x0004,
    WGL_CONTEXT_CORE_PROFILE_BIT_ARB     = 0x0001,
    WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x0002,
    WGL_CONTEXT_PROFILE_MASK_ARB         = 0x9125,
    WGL_CONTEXT_FORWARD_COMPAT_BIT_ARB   = 0x0002,
    WGL_CONTEXT_DEBUG_BIT_ARB            = 0x0001,
    WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB    = 0x0004,
    WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB = 0x8256,
    WGL_NO_RESET_NOTIFICATION_ARB        = 0x8257,
};

// ---------------------------------------------------------------------------
//  Function pointer typedefs
// ---------------------------------------------------------------------------
typedef void    (*PFNGLCLEARPROC)(GLenum);
typedef void    (*PFNGLCLEARCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void    (*PFNGLCLEARDEPTHPROC)(GLdouble);
typedef void    (*PFNGLVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void    (*PFNGLENABLEPROC)(GLenum);
typedef void    (*PFNGLDISABLEPROC)(GLenum);
typedef void    (*PFNGLDEPTHRANGEPROC)(GLdouble, GLdouble);
typedef void    (*PFNGLDEPTHMASKPROC)(GLboolean);
typedef void    (*PFNGLBLENDFUNCPROC)(GLenum, GLenum);
typedef void    (*PFNGLDEPTHFUNCPROC)(GLenum);
typedef void    (*PFNGLCULLFACEPROC)(GLenum);
typedef void    (*PFNGLFRONTFACEPROC)(GLenum);
typedef void    (*PFNGLLINEWIDTHPROC)(GLfloat);
typedef void    (*PFNGLPOINTSIZEPROC)(GLfloat);
typedef void    (*PFNGLFLUSHPROC)(void);
typedef void    (*PFNGLBLENDEQUATIONPROC)(GLenum);

typedef GLuint  (*PFNGLCREATEPROGRAMPROC)(void);
typedef void    (*PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void    (*PFNGLUSEPROGRAMPROC)(GLuint);
typedef GLint   (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void    (*PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void    (*PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void    (*PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void    (*PFNGLUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
typedef void    (*PFNGLUNIFORM4FPROC)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void    (*PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void    (*PFNGLUNIFORM2IPROC)(GLint, GLint, GLint);
typedef void    (*PFNGLUNIFORM1IVPROC)(GLint, GLsizei, const GLint*);
typedef void    (*PFNGLUNIFORM3IVPROC)(GLint, GLsizei, const GLint*);

typedef GLuint  (*PFNGLCREATESHADERPROC)(GLenum);
typedef void    (*PFNGLDELETESHADERPROC)(GLuint);
typedef void    (*PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void    (*PFNGLCOMPILESHADERPROC)(GLuint);
typedef void    (*PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void    (*PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void    (*PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void    (*PFNGLLINKPROGRAMPROC)(GLuint);
typedef void    (*PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void    (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void    (*PFNGLDETACHSHADERPROC)(GLuint, GLuint);
typedef void    (*PFNGLVALIDATEPROGRAMPROC)(GLuint);

typedef void    (*PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void    (*PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void    (*PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void    (*PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void    (*PFNGLBUFFERSUBDATAPROC)(GLenum, GLintptr, GLsizeiptr, const void*);
typedef void    (*PFNGLGETBUFFERPARAMETERIVPROC)(GLenum, GLenum, GLint*);

typedef void    (*PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void    (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void    (*PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void    (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void    (*PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void    (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void    (*PFNGLVERTEXATTRIB1FPROC)(GLuint, GLfloat);
typedef void    (*PFNGLVERTEXATTRIB2FPROC)(GLuint, GLfloat, GLfloat);
typedef void    (*PFNGLVERTEXATTRIB3FPROC)(GLuint, GLfloat, GLfloat, GLfloat);
typedef void    (*PFNGLVERTEXATTRIB4FPROC)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void    (*PFNGLVERTEXATTRIBI4IPROC)(GLuint, GLint, GLint, GLint, GLint);

typedef void    (*PFNGLDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
typedef void    (*PFNGLDRAWELEMENTSPROC)(GLenum, GLsizei, GLenum, const void*);
typedef void    (*PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum, GLint, GLsizei, GLsizei);
typedef void    (*PFNGLDRAWELEMENTSINSTANCEDPROC)(GLenum, GLsizei, GLenum, const void*, GLsizei);
typedef void    (*PFNGLVERTEXATTRIBDIVISORPROC)(GLuint, GLuint);

typedef void    (*PFNGLGENTEXTURESPROC)(GLsizei, GLuint*);
typedef void    (*PFNGLDELETETEXTURESPROC)(GLsizei, const GLuint*);
typedef void    (*PFNGLBINDTEXTUREPROC)(GLenum, GLuint);
typedef void    (*PFNGLACTIVETEXTUREPROC)(GLenum);
typedef void    (*PFNGLTEXIMAGE2DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void    (*PFNGLTEXSUBIMAGE2DPROC)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*);
typedef void    (*PFNGLTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef void    (*PFNGLTEXPARAMETERFPROC)(GLenum, GLenum, GLfloat);
typedef void    (*PFNGLTEXSTORAGE2DPROC)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
typedef void    (*PFNGLGENERATEMIPMAPPROC)(GLenum);
typedef void    (*PFNGLTEXTURESTORAGE2DPROC)(GLuint, GLsizei, GLenum, GLsizei, GLsizei);

typedef const GLubyte* (*PFNGLGETSTRINGPROC2)(GLenum);
typedef void    (*PFNGLGETINTEGERVPROC)(GLenum, GLint*);
typedef GLenum  (*PFNGLGETERRORPROC)(void);

// WGL
typedef HGLRC   (*PFNWGLCREATECONTEXTPROC)(void* /*HDC*/);
typedef HGLRC   (*PFNWGLCREATECONTEXTATTRIBSARBPROC)(void*, HGLRC, const int*);
typedef int     (*PFNWGLMAKECURRENTPROC)(void* /*HDC*/, HGLRC);
typedef HGLRC   (*PFNWGLGETCURRENTCONTEXTPROC)(void);
typedef int     (*PFNWGLDELETECONTEXTPROC)(HGLRC);
typedef void*   (*PFNWGLGETPROCADDRESSPROC)(const char*);


// ---------------------------------------------------------------------------
//  Function pointers (resolved at runtime by gl33::load)
//  Namespaced so they never clash with opengl32.dll's exported symbols.
//  Call sites use gl33::glClear(...), gl33::glDrawArrays(...), etc.
// ---------------------------------------------------------------------------
namespace gl33 {

extern PFNGLCLEARPROC                 glClear;
extern PFNGLCLEARCOLORPROC            glClearColor;
extern PFNGLCLEARDEPTHPROC            glClearDepth;
extern PFNGLVIEWPORTPROC              glViewport;
extern PFNGLENABLEPROC                glEnable;
extern PFNGLDISABLEPROC               glDisable;
extern PFNGLDEPTHRANGEPROC            glDepthRange;
extern PFNGLDEPTHMASKPROC             glDepthMask;
extern PFNGLBLENDFUNCPROC             glBlendFunc;
extern PFNGLDEPTHFUNCPROC             glDepthFunc;
extern PFNGLCULLFACEPROC              glCullFace;
extern PFNGLFRONTFACEPROC             glFrontFace;
extern PFNGLLINEWIDTHPROC             glLineWidth;
extern PFNGLPOINTSIZEPROC             glPointSize;
extern PFNGLFLUSHPROC                 glFlush;
extern PFNGLBLENDEQUATIONPROC         glBlendEquation;

extern PFNGLCREATEPROGRAMPROC         glCreateProgram;
extern PFNGLDELETEPROGRAMPROC         glDeleteProgram;
extern PFNGLUSEPROGRAMPROC            glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC    glGetUniformLocation;
extern PFNGLUNIFORM1FPROC             glUniform1f;
extern PFNGLUNIFORM1IPROC             glUniform1i;
extern PFNGLUNIFORM2FPROC             glUniform2f;
extern PFNGLUNIFORM3FPROC             glUniform3f;
extern PFNGLUNIFORM4FPROC             glUniform4f;
extern PFNGLUNIFORMMATRIX4FVPROC     glUniformMatrix4fv;
extern PFNGLUNIFORM2IPROC             glUniform2i;
extern PFNGLUNIFORM1IVPROC            glUniform1iv;
extern PFNGLUNIFORM3IVPROC            glUniform3iv;

extern PFNGLCREATESHADERPROC          glCreateShader;
extern PFNGLDELETESHADERPROC          glDeleteShader;
extern PFNGLSHADERSOURCEPROC          glShaderSource;
extern PFNGLCOMPILESHADERPROC         glCompileShader;
extern PFNGLGETSHADERIVPROC           glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC      glGetShaderInfoLog;
extern PFNGLATTACHSHADERPROC          glAttachShader;
extern PFNGLLINKPROGRAMPROC           glLinkProgram;
extern PFNGLGETPROGRAMIVPROC          glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC     glGetProgramInfoLog;
extern PFNGLDETACHSHADERPROC          glDetachShader;

extern PFNGLGENBUFFERSPROC            glGenBuffers;
extern PFNGLDELETEBUFFERSPROC         glDeleteBuffers;
extern PFNGLBINDBUFFERPROC            glBindBuffer;
extern PFNGLBUFFERDATAPROC            glBufferData;
extern PFNGLBUFFERSUBDATAPROC         glBufferSubData;
extern PFNGLGETBUFFERPARAMETERIVPROC  glGetBufferParameteriv;

extern PFNGLGENVERTEXARRAYSPROC       glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC    glDeleteVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC       glBindVertexArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC   glVertexAttribPointer;
extern PFNGLVERTEXATTRIB1FPROC        glVertexAttrib1f;
extern PFNGLVERTEXATTRIB2FPROC        glVertexAttrib2f;
extern PFNGLVERTEXATTRIB3FPROC        glVertexAttrib3f;
extern PFNGLVERTEXATTRIB4FPROC        glVertexAttrib4f;

extern PFNGLDRAWARRAYSPROC            glDrawArrays;
extern PFNGLDRAWELEMENTSPROC          glDrawElements;
extern PFNGLDRAWARRAYSINSTANCEDPROC   glDrawArraysInstanced;
extern PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
extern PFNGLVERTEXATTRIBDIVISORPROC   glVertexAttribDivisor;

extern PFNGLGENTEXTURESPROC           glGenTextures;
extern PFNGLDELETETEXTURESPROC        glDeleteTextures;
extern PFNGLBINDTEXTUREPROC           glBindTexture;
extern PFNGLACTIVETEXTUREPROC         glActiveTexture;
extern PFNGLTEXIMAGE2DPROC            glTexImage2D;
extern PFNGLTEXSUBIMAGE2DPROC         glTexSubImage2D;
extern PFNGLTEXPARAMETERIPROC         glTexParameteri;
extern PFNGLTEXPARAMETERFPROC         glTexParameterf;
extern PFNGLTEXSTORAGE2DPROC          glTexStorage2D;
extern PFNGLGENERATEMIPMAPPROC        glGenerateMipmap;

extern PFNGLGETSTRINGPROC2            glGetString;
extern PFNGLGETINTEGERVPROC           glGetIntegerv;
extern PFNGLGETERRORPROC              glGetError;

extern PFNWGLCREATECONTEXTPROC        wglCreateContext;
extern PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
extern PFNWGLMAKECURRENTPROC          wglMakeCurrent;
extern PFNWGLGETCURRENTCONTEXTPROC    wglGetCurrentContext;
extern PFNWGLDELETECONTEXTPROC        wglDeleteContext;

// ---------------------------------------------------------------------------
//  Loader
// ---------------------------------------------------------------------------
namespace detail {
// On Windows we resolve from the already-mapped opengl32.dll.
void* resolve(const char* name);
}

// Loads every entry point. Returns true when the whole table is ready.
// `haveCurrentContext` must be true (a context is current).
bool load();

// Human readable GL version / renderer string ("3.3.0 NVIDIA ...").
std::string versionString();

// True if the running context is core-profile 3.3 or newer.
bool isCompatible();

} // namespace gl33
