#version 460 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in float vertexTextureIndex;
layout(location = 2) in vec2 vertexTextureCoords;
layout(location = 3) in vec3 vertexColor;

out float fragmentTextureIndex;
out vec2 fragmentTextureCoords;
out vec3 fragmentColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(vertexPosition, 1.0);

    fragmentTextureIndex = vertexTextureIndex;
    fragmentTextureCoords = vertexTextureCoords;
	fragmentColor = vertexColor;
}

