layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

out vec3 fragmentPosition;
out vec3 fragmentNormal;
out vec3 fragmentTextureCoordinates;
out float fragmentLight;

uniform mat4 model;
uniform mat4 VP;
uniform float daylight;
uniform float ambientLight;
uniform vec3 lightDir;

void main()
{
	vec4 worldSpace = model * vec4(vertexPosition, 1.0);
	gl_Position = VP * worldSpace;

	fragmentPosition = worldSpace.xyz;
	fragmentNormal = vertexNormal;
	fragmentTextureCoordinates = vertexPosition;

	float diffuseLight = 0.3 * daylight * clamp(dot(normalize(fragmentNormal), normalize(lightDir)), 0.0, 1.0);
	fragmentLight = ambientLight + diffuseLight;
}
