/* hosttest: vitaGL sobre OpenGL de Mesa (llvmpipe, perfil de compatibilidad =
 * pipeline fijo de GLES 1.1) en un pbuffer EGL sin pantalla ("surfaceless").
 * Las funciones solo de GLES 1.1 (float/punto fijo) se simulan al final. vglSwapBuffers lee el cuadro y lo manda
 * a la ventana SDL (window_sdl.c) y, cada HOSTTEST_FRAME_MS, a un PPM. */
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "vitaGL.h"
#include "window_sdl.h"
#include "macho.h"

/* CTouchPad::s_currentDeviceOrientation del juego (1 o 3), como uikit.c */
static int guest_orientation(void)
{
    static int *o;
    if (!o) o = (int *)macho_sym("__ZN9CTouchPad26s_currentDeviceOrientationE");
    return o ? *o : 3;
}

static EGLDisplay s_dpy;
static EGLSurface s_surf;
static int s_w, s_h;
static uint32_t *s_pixels, *s_flipped;

GLboolean vglInitExtended(int legacy_pool_size, int width, int height, int ram_threshold, int msaa)
{
    static const EGLint cfg_attr[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_NONE };
    /* GL de escritorio, perfil de compatibilidad: Debian compila Mesa sin GLES 1.1 */
    static const EGLint ctx_attr[] = { EGL_NONE };
    PFNEGLGETPLATFORMDISPLAYEXTPROC get_display =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    EGLConfig cfg;
    EGLint n, surf_attr[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };
    EGLContext ctx;
    (void)legacy_pool_size; (void)ram_threshold; (void)msaa;
    const char *step = "display";
    s_dpy = get_display ? get_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL) : EGL_NO_DISPLAY;
    if (s_dpy == EGL_NO_DISPLAY || (step = "initialize", !eglInitialize(s_dpy, NULL, NULL))
            || (step = "config", !eglChooseConfig(s_dpy, cfg_attr, &cfg, 1, &n) || !n)
            || (step = "bind", !eglBindAPI(EGL_OPENGL_API))
            || (step = "pbuffer", (s_surf = eglCreatePbufferSurface(s_dpy, cfg, surf_attr)) == EGL_NO_SURFACE)
            || (step = "context", (ctx = eglCreateContext(s_dpy, cfg, EGL_NO_CONTEXT, ctx_attr)) == EGL_NO_CONTEXT)
            || (step = "make current", !eglMakeCurrent(s_dpy, s_surf, s_surf, ctx))) {
        fprintf(stderr, "[HOSTTEST] EGL/GLES1 no disponible: %s (error 0x%X)\n", step, eglGetError());
        exit(1);
    }
    s_w = width; s_h = height;
    s_pixels = malloc((size_t)width * height * 4);
    s_flipped = malloc((size_t)width * height * 4);
    fprintf(stderr, "[HOSTTEST] GLES %s (%s) en pbuffer %dx%d\n", glGetString(GL_VERSION),
            glGetString(GL_RENDERER), width, height);
    return GL_TRUE;
}

static void dump_ppm(void)
{
    static int every = -1;
    static unsigned shot;
    static time_t next;
    char path[256];
    FILE *f;
    int i;
    if (every < 0) every = getenv("HOSTTEST_FRAME_S") ? atoi(getenv("HOSTTEST_FRAME_S")) : 5;
    if (!every || time(NULL) < next) return;
    next = time(NULL) + every;
    mkdir("/data/frames", 0777);
    snprintf(path, sizeof path, "/data/frames/%04u.ppm", shot++);
    if (!(f = fopen(path, "wb"))) return;
    fprintf(f, "P6\n%d %d\n255\n", s_w, s_h);
    for (i = 0; i < s_w * s_h; i++) fwrite(&s_flipped[i], 1, 3, f);   /* RGBA -> RGB */
    fclose(f);
}

void vglSwapBuffers(GLboolean has_commondialog)
{
    GLint fb;
    int y;
    (void)has_commondialog;
    /* se lee del framebuffer de la ventana, no del FBO que el juego tenga puesto */
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &fb);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
    glReadPixels(0, 0, s_w, s_h, GL_RGBA, GL_UNSIGNED_BYTE, s_pixels);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, (GLuint)fb);
    /* gles.c gira el FBO retrato 320x480 a paisaje (±90° según la orientación del
     * juego) con la convención de vitaGL, que guarda las texturas FBO con el eje Y
     * al revés que Mesa. Compuesto con el giro, en Mesa sale espejado en Y con la
     * orientación 3 y en X con la 1 (comprobado con la pantalla de título). */
    for (y = 0; y < s_h; y++) {
        int x;
        if (guest_orientation() == 1) {
            const uint32_t *src = &s_pixels[y * s_w];
            for (x = 0; x < s_w; x++) s_flipped[y * s_w + x] = src[s_w - 1 - x];
        } else {
            memcpy(&s_flipped[y * s_w], &s_pixels[(s_h - 1 - y) * s_w], (size_t)s_w * 4);
        }
    }
    hosttest_window_frame(s_flipped, s_w, s_h, s_w);
    dump_ppm();
    eglSwapBuffers(s_dpy, s_surf);
}

void vglUseTripleBuffering(GLboolean usage) { (void)usage; }
size_t vglMemFree(vglMemType type) { (void)type; return 64u << 20; }

/* GL_OES_framebuffer_object = FBO del núcleo de GL 3.0 (mismo nombre sin "OES"). */
#define OES(ret, name, params, args) \
    ret name params { \
        static ret (*fn) params; \
        char core[64]; \
        if (!fn) { snprintf(core, sizeof core, "%s", #name); core[strlen(core) - 3] = 0; \
                   fn = (ret (*) params)eglGetProcAddress(core); } \
        return fn args; \
    }
OES(void, glGenFramebuffersOES, (GLsizei n, GLuint *i), (n, i))
OES(void, glDeleteFramebuffersOES, (GLsizei n, const GLuint *i), (n, i))
OES(void, glBindFramebufferOES, (GLenum t, GLuint f), (t, f))
OES(void, glFramebufferTexture2DOES, (GLenum t, GLenum a, GLenum tt, GLuint x, GLint l), (t, a, tt, x, l))
OES(void, glFramebufferRenderbufferOES, (GLenum t, GLenum a, GLenum rt, GLuint r), (t, a, rt, r))
OES(GLenum, glCheckFramebufferStatusOES, (GLenum t), (t))
OES(void, glGetFramebufferAttachmentParameterivOES, (GLenum t, GLenum a, GLenum p, GLint *v), (t, a, p, v))
OES(void, glGenRenderbuffersOES, (GLsizei n, GLuint *i), (n, i))
OES(void, glDeleteRenderbuffersOES, (GLsizei n, const GLuint *i), (n, i))
OES(void, glBindRenderbufferOES, (GLenum t, GLuint r), (t, r))
OES(void, glRenderbufferStorageOES, (GLenum t, GLenum f, GLsizei w, GLsizei h), (t, f, w, h))
OES(void, glGetRenderbufferParameterivOES, (GLenum t, GLenum p, GLint *v), (t, p, v))

/* FBO con los nombres de GLES 2 que vitaGL acepta también en GLES 1 */
void glGenFramebuffers(GLsizei n, GLuint *ids) { glGenFramebuffersOES(n, ids); }
void glDeleteFramebuffers(GLsizei n, const GLuint *ids) { glDeleteFramebuffersOES(n, ids); }
void glBindFramebuffer(GLenum t, GLuint fb) { glBindFramebufferOES(t, fb); }
void glFramebufferTexture2D(GLenum t, GLenum a, GLenum tt, GLuint tex, GLint l) { glFramebufferTexture2DOES(t, a, tt, tex, l); }
void glFramebufferRenderbuffer(GLenum t, GLenum a, GLenum rt, GLuint rb) { glFramebufferRenderbufferOES(t, a, rt, rb); }
GLenum glCheckFramebufferStatus(GLenum t) { return glCheckFramebufferStatusOES(t); }
void glGetFramebufferAttachmentParameteriv(GLenum t, GLenum a, GLenum p, GLint *v) { glGetFramebufferAttachmentParameterivOES(t, a, p, v); }
void glGenRenderbuffers(GLsizei n, GLuint *ids) { glGenRenderbuffersOES(n, ids); }
void glDeleteRenderbuffers(GLsizei n, const GLuint *ids) { glDeleteRenderbuffersOES(n, ids); }
void glBindRenderbuffer(GLenum t, GLuint rb) { glBindRenderbufferOES(t, rb); }
void glRenderbufferStorage(GLenum t, GLenum f, GLsizei w, GLsizei h) { glRenderbufferStorageOES(t, f, w, h); }
void glGetRenderbufferParameteriv(GLenum t, GLenum p, GLint *v) { glGetRenderbufferParameterivOES(t, p, v); }

/* Solo GLES 1.1: versiones float y en punto fijo 16.16 sobre las de escritorio */
void glOrtho(double, double, double, double, double, double);
void glFrustum(double, double, double, double, double, double);
void glClipPlane(GLenum, const double *);
#define FX(x) ((GLfloat)(x) / 65536.0f)
void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f) { glOrtho(l, r, b, t, n, f); }
void glFrustumf(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f) { glFrustum(l, r, b, t, n, f); }
void glClipPlanef(GLenum p, const GLfloat *e)
{
    double d[4] = { e[0], e[1], e[2], e[3] };
    glClipPlane(p, d);
}
void glOrthox(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f)
{
    glOrtho(FX(l), FX(r), FX(b), FX(t), FX(n), FX(f));
}
void glColor4x(GLfixed r, GLfixed g, GLfixed b, GLfixed a) { glColor4f(FX(r), FX(g), FX(b), FX(a)); }
void glLoadMatrixx(const GLfixed *m)
{
    GLfloat f[16];
    int i;
    for (i = 0; i < 16; i++) f[i] = FX(m[i]);
    glLoadMatrixf(f);
}
void glRotatex(GLfixed a, GLfixed x, GLfixed y, GLfixed z) { glRotatef(FX(a), FX(x), FX(y), FX(z)); }
void glScalex(GLfixed x, GLfixed y, GLfixed z) { glScalef(FX(x), FX(y), FX(z)); }
void glTranslatex(GLfixed x, GLfixed y, GLfixed z) { glTranslatef(FX(x), FX(y), FX(z)); }
/* los parámetros de modo (GL_MODULATE...) van como enteros, no como 16.16 */
void glTexEnvx(GLenum t, GLenum p, GLfixed v) { glTexEnvi(t, p, v); }
void glTexEnvxv(GLenum t, GLenum p, const GLfixed *v)
{
    GLfloat f[4];
    int i;
    if (p != GL_TEXTURE_ENV_COLOR) { glTexEnvi(t, p, v[0]); return; }
    for (i = 0; i < 4; i++) f[i] = FX(v[i]);
    glTexEnvfv(t, p, f);
}

/* Diagnóstico: formatos comprimidos que pide el juego (PVRTC no existe en Mesa). */
void glCompressedTexImage2D(GLenum target, GLint level, GLenum fmt, GLsizei w, GLsizei h,
                            GLint border, GLsizei size, const void *data)
{
    static void (*real)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void *);
    static int logged;
    GLenum err;
    if (!real) real = (void (*)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void *))
                     eglGetProcAddress("glCompressedTexImage2D");
    real(target, level, fmt, w, h, border, size, data);
    if ((err = glGetError()) != GL_NO_ERROR && logged++ < 10)
        fprintf(stderr, "[HOSTTEST] glCompressedTexImage2D formato 0x%04X %dx%d: error GL 0x%X\n", fmt, w, h, err);
}
