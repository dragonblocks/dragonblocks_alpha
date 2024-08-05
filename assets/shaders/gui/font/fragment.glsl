in vec2 fragmentTextureCoordinates;

out vec4 outColor;

uniform sampler2D texture0;
uniform vec4 color;

void main()
{
	outColor = vec4(1.0, 1.0, 1.0, texture(texture0, fragmentTextureCoordinates).r) * color;
}
