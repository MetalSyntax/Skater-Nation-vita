/* hosttest: lo de vitaGL.h que usa el loader, sobre el GLES 1.1 real de Linux
 * (Mesa). Las funciones vgl* y los nombres sin sufijo OES de FBO, que vitaGL
 * expone y GLES 1.1 no, están en vitagl_fake.c. */
#ifndef HOSTTEST_VITAGL_H
#define HOSTTEST_VITAGL_H
#include <GLES/gl.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES/glext.h>
#include <stddef.h>

#ifndef SCE_GXM_MULTISAMPLE_NONE
#define SCE_GXM_MULTISAMPLE_NONE 0
#endif
#define GL_FRAMEBUFFER          GL_FRAMEBUFFER_OES
#define GL_RENDERBUFFER         GL_RENDERBUFFER_OES
#define GL_COLOR_ATTACHMENT0    GL_COLOR_ATTACHMENT0_OES
#define GL_DEPTH_ATTACHMENT     GL_DEPTH_ATTACHMENT_OES
#define GL_STENCIL_ATTACHMENT   GL_STENCIL_ATTACHMENT_OES
#define GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_OES
#define GL_DEPTH_COMPONENT16    GL_DEPTH_COMPONENT16_OES
#define GL_RGBA8                GL_RGBA8_OES
#define GL_FRAMEBUFFER_BINDING  GL_FRAMEBUFFER_BINDING_OES

typedef enum { VGL_MEM_VRAM, VGL_MEM_RAM, VGL_MEM_PHYCONT, VGL_MEM_SLOW, VGL_MEM_BUDGET, VGL_MEM_EXTERNAL, VGL_MEM_ALL } vglMemType;

GLboolean vglInitExtended(int legacy_pool_size, int width, int height, int ram_threshold, int msaa);
void vglSwapBuffers(GLboolean has_commondialog);
void vglUseTripleBuffering(GLboolean usage);
size_t vglMemFree(vglMemType type);

void glGenFramebuffers(GLsizei n, GLuint *ids);
void glDeleteFramebuffers(GLsizei n, const GLuint *ids);
void glBindFramebuffer(GLenum target, GLuint fb);
void glFramebufferTexture2D(GLenum target, GLenum att, GLenum textarget, GLuint tex, GLint level);
void glFramebufferRenderbuffer(GLenum target, GLenum att, GLenum rbtarget, GLuint rb);
GLenum glCheckFramebufferStatus(GLenum target);
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum att, GLenum pname, GLint *params);
void glGenRenderbuffers(GLsizei n, GLuint *ids);
void glDeleteRenderbuffers(GLsizei n, const GLuint *ids);
void glBindRenderbuffer(GLenum target, GLuint rb);
void glRenderbufferStorage(GLenum target, GLenum fmt, GLsizei w, GLsizei h);
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params);

#endif
