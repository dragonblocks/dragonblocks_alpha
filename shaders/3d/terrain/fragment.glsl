in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec2 fragmentTextureCoordinates;
in float fragmentTextureIndex;
in vec3 fragmentColor;

out vec4 outColor;

uniform vec3 fogColor;
uniform vec3 cameraPos;
uniform sampler2D textures[MAX_TEXTURE_UNITS];

void main()
{
	outColor = texture(textures[int(fragmentTextureIndex + 0.5)], fragmentTextureCoordinates) * vec4(fragmentColor, 1.0);
	outColor.rgb = mix(outColor.rgb, fogColor, clamp(length(fragmentPosition - cameraPos) / VIEW_DISTANCE, 0.0, 1.0));

	if (outColor.a == 0.0)
		discard;
}
