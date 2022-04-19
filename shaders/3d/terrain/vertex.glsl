layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTextureCoordinates;
layout(location = 3) in float vertexTextureIndex;
layout(location = 4) in vec3 vertexColor;

out vec3 fragmentPosition;
out vec2 fragmentTextureCoordinates;
out float fragmentTextureIndex;
out vec3 fragmentColor;

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
	fragmentTextureCoordinates = vertexTextureCoordinates;
	fragmentTextureIndex = vertexTextureIndex;
	fragmentColor = vertexColor;

	fragmentColor *= ambientLight + 0.3 * daylight * clamp(dot(normalize(vertexNormal), normalize(lightDir)), 0.0, 1.0);
}
