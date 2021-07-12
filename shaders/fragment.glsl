#version 330 core

in vec2 fragmentTextureCoords;

out vec4 outColor;

uniform sampler2D texture0;

void main()
{
	outColor = texture(texture0, fragmentTextureCoords);
	if (outColor.a == 0.0)
        discard;
}
