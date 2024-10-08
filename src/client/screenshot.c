#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <GL/glew.h>
#include <GL/gl.h>
#include <stb_image_write.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include "client/game.h"
#include "client/opengl.h"
#include "client/window.h"

char *screenshot_dir = ".";

char *screenshot()
{
	// renderbuffer for depth & stencil buffer
	GLuint rbo;
	glGenRenderbuffers(1, &rbo); GL_DEBUG
	glBindRenderbuffer(GL_RENDERBUFFER, rbo); GL_DEBUG
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 8, GL_DEPTH24_STENCIL8, window.width, window.height); GL_DEBUG

	// 2 textures, one with AA, one without

	GLuint txos[2];
	glGenTextures(2, txos); GL_DEBUG

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, txos[0]); GL_DEBUG
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 8, GL_RGB, window.width, window.height, GL_TRUE); GL_DEBUG

	glBindTexture(GL_TEXTURE_2D, txos[1]); GL_DEBUG
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window.width, window.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); GL_DEBUG

	// 2 framebuffers, one with AA, one without

	GLuint fbos[2];
	glGenFramebuffers(2, fbos); GL_DEBUG

	glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]); GL_DEBUG
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, txos[0], 0); GL_DEBUG
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); GL_DEBUG

	glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]); GL_DEBUG
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, txos[1], 0); GL_DEBUG

	// render scene
	glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]); GL_DEBUG
	game_render(0.0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0); GL_DEBUG

	// blit AA-buffer into no-AA buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[0]); GL_DEBUG
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[1]); GL_DEBUG
	glBlitFramebuffer(0, 0, window.width, window.height, 0, 0, window.width, window.height, GL_COLOR_BUFFER_BIT, GL_NEAREST); GL_DEBUG

	// read data
	GLubyte *data = malloc(window.width * window.height * 3);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]); GL_DEBUG
	glPixelStorei(GL_PACK_ALIGNMENT, 1); GL_DEBUG
	glReadPixels(0, 0, window.width, window.height, GL_RGB, GL_UNSIGNED_BYTE, data); GL_DEBUG

	// create filename
	char filename[BUFSIZ];
	time_t timep = time(0);
	strftime(filename, BUFSIZ, "screenshot-%Y-%m-%d-%H:%M:%S.png", localtime(&timep));

	char path[strlen(screenshot_dir) + 1 + strlen(filename) + 1];
	sprintf(path, "%s/%s", screenshot_dir, filename);

	// save screenshot
	stbi_flip_vertically_on_write(true);
	stbi_write_png(path, window.width, window.height, 3, data, window.width * 3);
	free(data);

	// delete buffers
	glDeleteRenderbuffers(1, &rbo); GL_DEBUG
	glDeleteTextures(2, txos); GL_DEBUG
	glDeleteFramebuffers(2, fbos); GL_DEBUG

#ifdef _WIN32
	char *display_path = path;
#else
	char display_path[PATH_MAX];
	realpath(path, display_path);

	char *home = getenv("HOME");
	size_t len_home = strlen(home);
	if (len_home > 0 && strncmp(display_path, home, len_home) == 0) {
		display_path[len_home-1] = '~';
		return strdup(display_path+len_home-1);
	}
#endif

	return strdup(display_path);
}
