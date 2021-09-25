in vec2 fragmentTextureCoords;

out vec4 outColor;

uniform sampler2D texture0;
uniform vec4 color;

void main()
{
	outColor = vec4(1.0, 1.0, 1.0, texture(texture0, fragmentTextureCoords).r) * color;
}
