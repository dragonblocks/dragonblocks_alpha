in float fragmentTextureIndex;
in vec2 fragmentTextureCoords;
in vec3 fragmentColor;

out vec4 outColor;

uniform sampler2D textures[MAX_TEXTURE_UNITS];

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
	outColor = texture(textures[int(fragmentTextureIndex + 0.5)], fragmentTextureCoords) * vec4(hsv2rgb(vec3(fragmentColor)), 1.0);
}
