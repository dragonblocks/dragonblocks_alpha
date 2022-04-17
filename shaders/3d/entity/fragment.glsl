in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec3 fragmentTextureCoordinates;
in float fragmentLight;

out vec4 outColor;

uniform vec3 fogColor;
uniform vec3 cameraPos;
uniform samplerCube texture0;

void main()
{
	outColor = texture(texture0, fragmentTextureCoordinates) * vec4(vec3(fragmentLight), 1.0);
	outColor.rgb = mix(outColor.rgb, fogColor, clamp(length(fragmentPosition - cameraPos) / VIEW_DISTANCE, 0.0, 1.0));

	if (outColor.a == 0.0)
		discard;
}
