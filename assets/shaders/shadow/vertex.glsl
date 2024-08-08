layout (location = 0) in vec3 vertexPosition;

uniform mat4 VP;
uniform mat4 model;

void main()
{
    gl_Position = VP * model * vec4(vertexPosition, 1.0);
}
