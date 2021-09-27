in vec3 fragmentTextureCoords;

out vec4 outColor;

uniform bool transparency;
uniform float daylight;
uniform samplerCube textures[2];

void main()
{
	outColor = mix(texture(textures[1], fragmentTextureCoords), texture(textures[0], fragmentTextureCoords), daylight);

	if (transparency) {
		float f = 0.2;
		float e = 2.0;

		outColor.a = pow((outColor.r + outColor.g) / 2.0 + f, e) / pow(1.0 + f, e);
	}
}
