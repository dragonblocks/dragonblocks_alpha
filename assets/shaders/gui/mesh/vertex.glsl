layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec3 vertColor;

out vec3 fragColor;

uniform mat4 model;
uniform mat4 projection;

void main()
{
	gl_Position = projection * model * vec4(vertPos, 1.0);
	fragColor = vertColor;
}
