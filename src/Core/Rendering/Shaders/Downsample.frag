R""(
#version 420 core

out vec4 FragColor;
in vec2 texCoords;

layout(binding = 0) uniform sampler2D foregroundTexture;

uniform float prevLevel = 0.0;
uniform vec2 invPrevSize;

void main() {
  // Average a 4x4 region with [1,3,3,1] x [1,3,3,1] weights. 
  // We can get these weights by sampling with linear interpolation at a distance of 3/4 high-res pixel.
  vec2 d = 0.75 * invPrevSize;
  FragColor = 0.25 * (textureLod(foregroundTexture, texCoords + vec2(-d.x, -d.y), prevLevel) +
                      textureLod(foregroundTexture, texCoords + vec2( d.x, -d.y), prevLevel) + 
                      textureLod(foregroundTexture, texCoords + vec2(-d.x,  d.y), prevLevel) +
                      textureLod(foregroundTexture, texCoords + vec2( d.x,  d.y), prevLevel));
}
)""
