in vec3 fragmentPosition;
in vec2 fragmentTextureCoordinates;
in float fragmentTextureIndex;
in vec3 fragmentColor;

out vec4 outColor;

uniform vec3 fogColor;
uniform vec3 cameraPos;

#if TEXURE_BATCH_UNITS > 1
uniform sampler2D textures[8];
#else
uniform sampler2D texture0;
#endif

void main()
{
#if TEXURE_BATCH_UNITS > 1
	vec4 texel = texture(textures[int(fragmentTextureIndex + 0.5)], fragmentTextureCoordinates);
#else
	vec4 texel = texture(texture0, fragmentTextureCoordinates);
#endif

	outColor = texel * vec4(fragmentColor, 1.0);
	outColor.rgb = mix(outColor.rgb, fogColor, clamp(length(fragmentPosition - cameraPos) / VIEW_DISTANCE, 0.0, 1.0));

	if (outColor.a == 0.0)
		discard;
}
