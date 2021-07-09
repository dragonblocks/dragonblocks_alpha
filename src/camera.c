#include "client.h"
#include "camera.h"

static mat4x4 view, projection;
static GLFWwindow *window;
static ShaderProgram *prog;

void set_camera_position(v3f pos)
{
	mat4x4_translate(view, -pos.x, -pos.y, -pos.z);
	glUniformMatrix4fv(prog->loc_view, 1, GL_FALSE, view[0]);
}

void set_window_size(int width, int height)
{
	mat4x4_perspective(projection, 86.1f / 180.0f * M_PI, (float) width / (float) height, 0.01f, 1000.0f);
	glUniformMatrix4fv(prog->loc_projection, 1, GL_FALSE, projection[0]);
}

static void framebuffer_size_callback(__attribute__((unused)) GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	set_window_size(width, height);
}

void init_camera(GLFWwindow *w, ShaderProgram *p)
{
	window = w;
	prog = p;

	glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback);
}
