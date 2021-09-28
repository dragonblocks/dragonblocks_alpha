in vec3 fragmentTextureCoords;

out vec4 outColor;

uniform float daylight;
uniform samplerCube texture0;

float reverseMix(float value, float min, float max)
{
	return clamp((value - min) / (max - min), 0.0, 1.0);
}

float strengthen(float value, float exponent, float max)
{
	return min((1.0 - pow(1.0 - value, exponent)) / (1.0 - pow(1.0 - max, exponent)), 1.0);
}

void main()
{
	float height = normalize(fragmentTextureCoords).y;

	vec4 topColor = texture(texture0, vec3(0.0, 1.0, 0.0));
	vec4 bottomColor = texture(texture0, vec3(1.0, 0.11, 0.5));
	vec4 expectedColor = mix(bottomColor, topColor, height);

	vec4 dayColor = texture(texture0, fragmentTextureCoords);

	float cloudFactor = reverseMix(length(dayColor.rg - expectedColor.rg), 0.15, length(vec2(1.0)));

	outColor = vec4(dayColor.rgb, mix(cloudFactor, strengthen(cloudFactor, 8.0, 0.1), daylight));
}
