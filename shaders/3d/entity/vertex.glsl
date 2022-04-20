layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

out vec3 fragmentPosition;
out vec3 fragmentTextureCoordinates;
out float fragmentLight;

uniform mat4 model;
uniform mat4 VP;
uniform float daylight;
uniform float ambientLight;
uniform vec3 lightDir;
uniform float depthOffset;

void main()
{
	vec4 worldSpace = model * vec4(vertexPosition, 1.0);
	gl_Position = VP * worldSpace;
	if (gl_Position.z > -1.0)
		gl_Position.z = max(-1.0, gl_Position.z - depthOffset);

	fragmentPosition = worldSpace.xyz;
	fragmentTextureCoordinates = vertexPosition;

	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vec3 normal = normalize(normalMatrix * vertexNormal);
	fragmentLight = ambientLight + 0.3 * daylight * clamp(dot(normal, normalize(lightDir)), 0.0, 1.0);
}
