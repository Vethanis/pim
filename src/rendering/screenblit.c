#include "screenblit.h"
#include "rendering/window.h"
#include "io/fd.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <glad/glad.h>

// ----------------------------------------------------------------------------

typedef GLuint glhandle;

typedef struct glmesh_s
{
    glhandle vao;
    glhandle vbo;
    i32 vertCount;
} glmesh_t;

typedef struct vert_attrib_s
{
    i32 dimension;
    i32 offset;
} vert_attrib_t;

// ----------------------------------------------------------------------------

static const float kScreenMesh[] =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f,  1.0f,
    -1.0f,  1.0f,
    -1.0f, -1.0f
};

static const char* const kBlitVertShader[] =
{
    "#version 330 core\n",
    "layout(location = 0) in vec2 mesh;\n",
    "out vec2 uv;\n",
    "void main()\n",
    "{\n",
    "   gl_Position = vec4(mesh.xy, 0.0, 1.0);\n",
    "   uv = mesh * 0.5 + 0.5;\n",
    "}\n",
};

static const char* const kBlitFragShader[] =
{
    "#version 330 core\n",
    "in vec2 uv;\n",
    "out vec4 outColor;\n",
    "uniform sampler2D inColor;\n",
    "void main()\n",
    "{\n",
    "   outColor = texture(inColor, uv);\n",
    "}\n",
};

// ----------------------------------------------------------------------------

static glhandle CreateGlProgram(
    i32 vsLines, const char* const * const vs,
    i32 fsLines, const char* const * const fs);
static void DestroyGlProgram(glhandle* pProg);
static void SetupTextureUnit(glhandle prog, const char* texName, i32 unit);

static glhandle CreateGlTexture(i32 width, i32 height);
static void UpdateGlTexture(
    glhandle hdl, i32 width, i32 height, const void* data);
static void DestroyGlTexture(glhandle* pHdl);
static void BindGlTexture(glhandle hdl, i32 unit);

static glmesh_t CreateGlMesh(
    i32 bytes, const void* data,
    i32 stride,
    i32 attribCount, const vert_attrib_t* attribs);
static void DrawGlMesh(glmesh_t mesh);
static void DestroyGlMesh(glmesh_t* pMesh);

// ----------------------------------------------------------------------------

static glmesh_t ms_blitMesh;
static glhandle ms_blitProgram;
static glhandle ms_image;
static i32 ms_width;
static i32 ms_height;

// ----------------------------------------------------------------------------

void screenblit_init(i32 width, i32 height)
{
    ms_width = width;
    ms_height = height;
    ms_image = CreateGlTexture(width, height);
    ASSERT(ms_image);
    const vert_attrib_t attribs[] =
    {
        {.dimension = 2,.offset = 0 },
    };
    ms_blitMesh = CreateGlMesh(
        sizeof(kScreenMesh), kScreenMesh,
        sizeof(float) * 2,
        NELEM(attribs), attribs);
    ms_blitProgram = CreateGlProgram(
        NELEM(kBlitVertShader), kBlitVertShader,
        NELEM(kBlitFragShader), kBlitFragShader);
    ASSERT(ms_blitProgram);
    SetupTextureUnit(ms_blitProgram, "inColor", 0);
}

void screenblit_shutdown(void)
{
    DestroyGlTexture(&ms_image);
    DestroyGlMesh(&ms_blitMesh);
    DestroyGlProgram(&ms_blitProgram);
    ms_width = 0;
    ms_height = 0;
}

ProfileMark(pm_blit, screenblit_blit)
void screenblit_blit(const u32* texels, i32 width, i32 height)
{
    ProfileBegin(pm_blit);

    ASSERT(width == ms_width);
    ASSERT(height == ms_height);
    UpdateGlTexture(ms_image, width, height, texels);

    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, window_width(), window_height());
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(ms_blitProgram);
    BindGlTexture(ms_image, 0);
    DrawGlMesh(ms_blitMesh);

    ProfileEnd(pm_blit);
}

// ----------------------------------------------------------------------------

static glhandle CreateGlTexture(i32 width, i32 height)
{
    glhandle hdl = 0;
    glGenTextures(1, &hdl);
    ASSERT(hdl);
    ASSERT(!glGetError());
    glBindTexture(GL_TEXTURE_2D, hdl);
    ASSERT(!glGetError());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    ASSERT(!glGetError());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT(!glGetError());
    glTexImage2D(
        GL_TEXTURE_2D,              // target
        0,                          // level
        GL_RGBA8,                   // internalformat
        width,                      // width
        height,                     // height
        GL_FALSE,                   // border
        GL_RGBA,                    // format
        GL_UNSIGNED_BYTE,           // type
        NULL);                      // data
    ASSERT(!glGetError());
    return hdl;
}

static void UpdateGlTexture(
    glhandle hdl,
    i32 width,
    i32 height,
    const void* data)
{
    ASSERT(hdl);
    ASSERT(data);
    glBindTexture(GL_TEXTURE_2D, hdl);
    ASSERT(!glGetError());
    glTexSubImage2D(
        GL_TEXTURE_2D,              // target
        0,                          // mip level
        0,                          // xoffset
        0,                          // yoffset
        width,                      // width
        height,                     // height
        GL_RGBA,                    // format
        GL_UNSIGNED_BYTE,           // type
        data);                      // data
    ASSERT(!glGetError());
}

static void DestroyGlTexture(glhandle* pHdl)
{
    ASSERT(pHdl);
    glhandle hdl = *pHdl;
    *pHdl = 0;
    if (hdl)
    {
        glDeleteTextures(1, &hdl);
        ASSERT(!glGetError());
    }
}

static void BindGlTexture(glhandle hdl, i32 index)
{
    ASSERT(hdl);
    ASSERT(index >= 0 && index < 8);
    glActiveTexture(GL_TEXTURE0 + index);
    ASSERT(!glGetError());
    glBindTexture(GL_TEXTURE_2D, hdl);
    ASSERT(!glGetError());
}

static glmesh_t CreateGlMesh(
    i32 bytes, const void* data,
    i32 stride,
    i32 attribCount, const vert_attrib_t* attribs)
{
    ASSERT(bytes > 0);
    ASSERT(data);
    ASSERT(attribCount > 0);
    ASSERT(attribs);

    i32 vao = 0;
    i32 vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    ASSERT(vao);
    ASSERT(vbo);
    ASSERT(!glGetError());

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    ASSERT(!glGetError());
    for (i32 i = 0; i < attribCount; ++i)
    {
        glEnableVertexAttribArray(i);
        ASSERT(!glGetError());
        glVertexAttribPointer(
            i,                              // index
            attribs[i].dimension,           // dimension
            GL_FLOAT,                       // type
            GL_FALSE,                       // normalized
            stride,                         // bytes between vertices
            (void*)(isize)(attribs[i].offset));    // offset of attribute
        ASSERT(!glGetError());
    }
    glBufferData(GL_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
    ASSERT(!glGetError());

    glmesh_t mesh;
    mesh.vao = vao;
    mesh.vbo = vbo;
    mesh.vertCount = bytes / stride;

    return mesh;
}

static void DrawGlMesh(glmesh_t mesh)
{
    ASSERT(mesh.vao);
    ASSERT(mesh.vbo);
    ASSERT(mesh.vertCount > 0);
    glBindVertexArray(mesh.vao);
    ASSERT(!glGetError());
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertCount);
    ASSERT(!glGetError());
}

static void DestroyGlMesh(glmesh_t* pMesh)
{
    ASSERT(pMesh);
    glhandle vao = pMesh->vao;
    glhandle vbo = pMesh->vbo;
    pMesh->vao = 0;
    pMesh->vbo = 0;
    pMesh->vertCount = 0;
    if (vbo)
    {
        glDeleteBuffers(1, &vbo);
        ASSERT(!glGetError());
    }
    if (vao)
    {
        glDeleteVertexArrays(1, &vao);
        ASSERT(!glGetError());
    }
}

static char* GetShaderLog(glhandle shader)
{
    ASSERT(shader);
    i32 status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    ASSERT(!glGetError());
    if (!status)
    {
        i32 loglen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
        ASSERT(!glGetError());
        char* infolog = pim_calloc(EAlloc_Temp, loglen + 1);
        ASSERT(infolog);
        glGetShaderInfoLog(shader, loglen, NULL, infolog);
        ASSERT(!glGetError());
        infolog[loglen] = 0;
        return infolog;
    }
    return NULL;
}

static char* GetProgramLog(glhandle program)
{
    ASSERT(program);
    i32 status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    ASSERT(!glGetError());
    if (!status)
    {
        i32 loglen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);
        ASSERT(!glGetError());
        char* infolog = pim_calloc(EAlloc_Temp, loglen + 1);
        glGetProgramInfoLog(program, loglen, NULL, infolog);
        ASSERT(!glGetError());
        infolog[loglen] = 0;
        return infolog;
    }
    return NULL;
}

static glhandle CreateGlProgram(
    i32 vsLines, const char* const * const vs,
    i32 fsLines, const char* const * const fs)
{
    ASSERT(vsLines > 0);
    ASSERT(fsLines > 0);
    ASSERT(vs);
    ASSERT(fs);

    glhandle vso = glCreateShader(GL_VERTEX_SHADER);
    ASSERT(vso);
    ASSERT(!glGetError());
    glShaderSource(vso, vsLines, vs, NULL);
    ASSERT(!glGetError());
    glCompileShader(vso);
    ASSERT(!glGetError());
    char* vsErrors = GetShaderLog(vso);
    if (vsErrors)
    {
        fd_puts(fd_stderr, vsErrors);
        pim_free(vsErrors);
        glDeleteShader(vso);
        ASSERT(!glGetError());
        return 0;
    }

    glhandle fso = glCreateShader(GL_FRAGMENT_SHADER);
    ASSERT(fso);
    ASSERT(!glGetError());
    glShaderSource(fso, fsLines, fs, NULL);
    ASSERT(!glGetError());
    glCompileShader(fso);
    ASSERT(!glGetError());
    char* fsErrors = GetShaderLog(vso);
    if (fsErrors)
    {
        fd_puts(fd_stderr, fsErrors);
        pim_free(fsErrors);
        glDeleteShader(vso);
        ASSERT(!glGetError());
        glDeleteShader(fso);
        ASSERT(!glGetError());
        return 0;
    }

    glhandle prog = glCreateProgram();
    ASSERT(prog);
    ASSERT(!glGetError());
    glAttachShader(prog, vso);
    ASSERT(!glGetError());
    glAttachShader(prog, fso);
    ASSERT(!glGetError());
    glLinkProgram(prog);
    ASSERT(!glGetError());

    char* progErrors = GetProgramLog(prog);
    if (progErrors)
    {
        fd_puts(fd_stderr, progErrors);
        pim_free(progErrors);
        glDeleteProgram(prog);
        ASSERT(!glGetError());
        prog = 0;
    }

    glDeleteShader(vso);
    ASSERT(!glGetError());
    glDeleteShader(fso);
    ASSERT(!glGetError());

    return prog;
}

static void DestroyGlProgram(glhandle* pProg)
{
    ASSERT(pProg);
    glhandle prog = *pProg;
    *pProg = 0;
    if (prog)
    {
        glUseProgram(0);
        ASSERT(!glGetError());
        glDeleteProgram(prog);
        ASSERT(!glGetError());
    }
}

static void SetupTextureUnit(glhandle prog, const char* texName, i32 unit)
{
    ASSERT(prog);
    ASSERT(texName);
    i32 location = glGetUniformLocation(prog, texName);
    ASSERT(!glGetError());
    ASSERT(location != -1);
    glUseProgram(prog);
    ASSERT(!glGetError());
    glUniform1i(location, unit);
    ASSERT(!glGetError());
}
