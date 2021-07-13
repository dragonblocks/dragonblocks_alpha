#version 330 core

in vec2 fragmentTextureCoords;
in vec3 fragmentColor;

out vec4 outColor;

uniform sampler2D texture0;

void main()
{
	outColor = texture(texture0, fragmentTextureCoords) * vec4(fragmentColor, 1.0);
	if (outColor.a == 0.0)
        discard;
}
