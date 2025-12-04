R""(
#version 450 core

layout(location = 0) in struct {
    // vec4 position;
    // vec4 normal;
    vec2 texCoords;
} fsIn;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D tex;

void main()
{
  FragColor = texture(tex, fsIn.texCoords);
}
)""
