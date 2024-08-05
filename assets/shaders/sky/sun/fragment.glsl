in vec2 fragmentTextureCoordinates;

out vec4 outColor;

uniform sampler2D texture0;

void main()
{
	outColor = texture(texture0, fragmentTextureCoordinates);
}
