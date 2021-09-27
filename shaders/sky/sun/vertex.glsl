layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTextureCoords;

out vec2 fragmentTextureCoords;

uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(vertexPosition, 1.0);
	gl_Position.z = gl_Position.w;
	fragmentTextureCoords = vertexTextureCoords;
}
