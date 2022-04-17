#ifndef _GL_DEBUG_H_
#define _GL_DEBUG_H_

#ifdef ENABLE_GL_DEBUG
#define GL_DEBUG gl_debug(__FILE__, __LINE__);
#else
#define GL_DEBUG
#endif

void gl_debug(const char *file, int line);

#endif
