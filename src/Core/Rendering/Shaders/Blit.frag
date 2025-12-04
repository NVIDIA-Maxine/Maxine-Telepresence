R""(
#version 420 core

out vec4 FragColor;
in vec2 texCoords;

layout(binding = 0) uniform sampler2D screenTexture;  // rgba

void main() {
  vec3 col = texture(screenTexture, texCoords.st).rgb;
  FragColor = vec4(col, 1.0);
}
)""
