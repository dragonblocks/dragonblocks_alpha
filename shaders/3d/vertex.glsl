layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in float vertexTextureIndex;
layout(location = 3) in vec2 vertexTextureCoords;
layout(location = 4) in vec3 vertexColor;

out vec3 fragmentNormal;
out float fragmentTextureIndex;
out vec2 fragmentTextureCoords;
out vec3 fragmentColor;

uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(vertexPosition, 1.0);

	fragmentNormal = vertexNormal;
	fragmentTextureIndex = vertexTextureIndex;
	fragmentTextureCoords = vertexTextureCoords;
	fragmentColor = vertexColor;
}
