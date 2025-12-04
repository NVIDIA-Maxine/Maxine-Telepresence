R""(
#version 450 core

layout (location = 0) in vec3 aPos;

out vec3 texCoord;

// Imports the view matrix, projection matrix, and model matrix
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;

void main()
{
	// Use Rotation only
	mat4 viewRotation = mat4(mat3(viewMatrix));
	mat4 modelRotation = mat4(mat3(modelMatrix));

	// calculates current position
	vec4 vPosModelSpace = modelMatrix * vec4(aPos, 1.0f);
	
	// Outputs the positions/coordinates of all vertices
	gl_Position = projectionMatrix * viewMatrix * vPosModelSpace;
	texCoord = aPos;
}
)""
