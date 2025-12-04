R""(
#version 450 core

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec4 aNormal;
layout(location = 2) in vec2 aTexCoords;

layout(location = 0) out struct {
    // vec4 position;
    // vec4 normal;
    vec2 texCoords;
} vsOut;

// Imports the view matrix, projection matrix, and model matrix
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  vsOut.texCoords = aTexCoords;
  gl_Position = projectionMatrix * viewMatrix * modelMatrix * aPosition;
}
)""
