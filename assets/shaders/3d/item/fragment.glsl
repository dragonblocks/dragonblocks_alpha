in vec3 fragmentPosition;
in vec3 fragmentColor;

out vec4 outColor;

uniform vec3 fogColor;
uniform vec3 cameraPos;

void main()
{
	outColor = vec4(fragmentColor, 1.0);
	outColor.rgb = mix(outColor.rgb, fogColor, clamp(length(fragmentPosition - cameraPos) / VIEW_DISTANCE, 0.0, 1.0));

	if (outColor.a == 0.0)
		discard;
}
