in vec3 fragmentTextureCoords;

out vec4 outColor;

uniform float daylight;
uniform samplerCube textures[2];

void main()
{
	vec4 topColor = texture(textures[0], vec3(0.0, 1.0, 0.0));
	vec4 bottomColor = texture(textures[0], vec3(1.0, 0.11, 0.5));

	vec4 dayColor = mix(bottomColor, topColor, normalize(fragmentTextureCoords).y);
	vec4 nightColor = texture(textures[1], fragmentTextureCoords);

	outColor = mix(nightColor, dayColor, daylight);
}
