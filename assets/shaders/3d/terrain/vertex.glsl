layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec2 vertTexCoord;
layout(location = 3) in vec3 vertColor;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec3 fragColor;
out vec4 fragPosLightSP;

uniform mat4 model;
uniform mat4 VP;
uniform mat4 lightVP;

void main()
{
	vec4 worldSP = model * vec4(vertPos, 1.0);
	gl_Position = VP * worldSP;

	fragPos = worldSP.xyz;
	fragNormal = vertNormal;
	fragTexCoord = vertTexCoord;
	fragColor = vertColor;
	fragPosLightSP = lightVP * vec4(fragPos, 1.0);
}
