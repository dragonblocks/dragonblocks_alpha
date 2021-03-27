#version 330 core

in vec3 fragmentColor;

out vec4 outColor;

void main()
{
	outColor = vec4(fragmentColor, 0.1);
}
