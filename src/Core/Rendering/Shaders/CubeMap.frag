R""(
#version 450 core

layout (location = 0) out vec4 FragColor;

in vec3 texCoord;

uniform samplerCube skybox;

void main()
{
	FragColor = vec4(texture(skybox, texCoord).rgb, 1.0);
}
)""
