in vec3 fragmentPosition;
in vec3 fragmentNormal;
in float fragmentTextureIndex;
in vec2 fragmentTextureCoords;
in vec3 fragmentColor;

out vec4 outColor;

uniform vec3 fogColor;
uniform vec3 cameraPos;
uniform sampler2D textures[MAX_TEXTURE_UNITS];

void main()
{
	outColor = texture(textures[int(fragmentTextureIndex + 0.5)], fragmentTextureCoords) * vec4(fragmentColor, 1.0);
	outColor.rgb = mix(outColor.rgb, fogColor, clamp(length(fragmentPosition - cameraPos) / RENDER_DISTANCE, 0.0, 1.0));

	if (outColor.a == 0.0)
		discard;
}
