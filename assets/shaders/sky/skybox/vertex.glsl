layout(location = 0) in vec3 vertexPosition;

out vec3 fragmentTextureCoordinates;

uniform mat4 VP;

void main()
{
	gl_Position = VP * vec4(vertexPosition, 1.0);
	gl_Position.z = gl_Position.w;
	fragmentTextureCoordinates = vertexPosition;
}
