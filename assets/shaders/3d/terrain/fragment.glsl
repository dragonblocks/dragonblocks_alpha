in vec3 fragmentPosition;
centroid in vec2 fragmentTextureCoordinates;
in vec3 fragmentColor;

out vec4 outColor;

uniform vec3 fogColor;
uniform vec3 cameraPos;

uniform sampler2D atlas_texture;

void main()
{
	vec4 texel = texture(atlas_texture, fragmentTextureCoordinates);

	outColor = texel * vec4(fragmentColor, 1.0);
	outColor.rgb = mix(outColor.rgb, fogColor, clamp(length(fragmentPosition - cameraPos) / VIEW_DISTANCE, 0.0, 1.0));

	if (outColor.a == 0.0)
		discard;
}
