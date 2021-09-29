in vec3 fragmentPosition;
in vec3 fragmentNormal;
in float fragmentTextureIndex;
in vec2 fragmentTextureCoords;
in vec3 fragmentColor;

out vec4 outColor;

uniform float daylight;
uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform sampler2D textures[MAX_TEXTURE_UNITS];

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
	vec3 lightColor = vec3(1.0);
	vec3 fogColor = vec3(0x87, 0xCE, 0xEB) / vec3(0xFF);

	float ambientStrength = mix(0.3, 0.7, daylight);

	vec3 ambient = ambientStrength * lightColor;
	vec3 diffuse = 0.3 * daylight * clamp(dot(normalize(fragmentNormal), normalize(lightDir)), 0.0, 1.0) * lightColor;

	vec3 light = ambient + diffuse;

	outColor = texture(textures[int(fragmentTextureIndex + 0.5)], fragmentTextureCoords) * vec4(hsv2rgb(vec3(fragmentColor)), 1.0) * vec4(light, 1.0);
	outColor.rgb = mix(outColor.rgb, ambientStrength * vec3(fogColor), clamp(length(fragmentPosition - cameraPos) / 255.0, 0.0, 1.0));
}
