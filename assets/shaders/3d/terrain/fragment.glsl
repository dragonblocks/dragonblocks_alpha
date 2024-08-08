in vec3 fragPos;
in vec3 fragNormal;
centroid in vec2 fragTexCoord;
in vec3 fragColor;
in vec4 fragPosLightSP;

out vec4 outColor;

uniform vec3 fogColor;
uniform vec3 cameraPos;

uniform float daylight;
uniform float ambientLight;
uniform vec3 lightDir;

uniform sampler2D atlasTexture;
uniform sampler2D shadowMap;

#define SHADOWS
#define SHADOWS_PFC
#define FOG

float compute_shadow()
{
	float shadow = 0.0;

#ifdef SHADOWS
	vec3 lightTexSP = (fragPosLightSP.xyz / fragPosLightSP.w) * 0.5 + 0.5;

#ifdef SHADOWS_PFC
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for (int x = -1; x <= 1; x++) {
	for (int y = -1; y <= 1; y++) {
		vec2 offset = vec2(x, y) * texelSize;
#else
		vec2 offset = vec2(0);
#endif

        float shadowDepth = texture(shadowMap, lightTexSP.xy + offset).r;
        shadow += lightTexSP.z > shadowDepth ? 1.0 : 0.0;

#ifdef SHADOWS_PFC
	}}
	shadow /= 9.0;
#endif

#endif

    return shadow;
}

float compute_diffuse()
{
	return clamp(dot(normalize(fragNormal), normalize(lightDir)), 0.0, 1.0);;
}

vec3 compute_light()
{
	return vec3(ambientLight + 0.3 * daylight * (1.0 - compute_shadow()) * compute_diffuse());
}

vec3 compute_fog(vec3 color)
{
#ifdef FOG
	float fog = clamp(length(fragPos - cameraPos) / VIEW_DISTANCE, 0.0, 1.0);
	return mix(color, fogColor, fog);
#else
	return color;
#endif
}

void main()
{
	outColor = texture(atlasTexture, fragTexCoord);

	if (outColor.a == 0.0)
		discard;

	outColor *= vec4(fragColor, 1.0);
	outColor.rgb *= compute_light();
	outColor.rgb = compute_fog(outColor.rgb);
}
