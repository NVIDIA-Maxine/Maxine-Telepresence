R""(
#version 420 core

out vec4 FragColor;
in vec2 texCoords;

layout(binding = 0) uniform sampler2D foregroundTexture;  // outRGBA

uniform float invShadowScale = 0.8;
uniform float shadowOpacity = 0.5;
uniform float shadowLod = 5.0;
uniform vec2 shadowOffset = vec2(0.0);
uniform float sharpenForeground = 0.0;

// Scale X coord from the center of the image, scale Y coord from the top of the image.
uniform vec2 shadowOrigin = vec2(0.5, 1.0);

// Over operator with pre-multiplied alpha.
vec4 CompositeOver(vec4 src, vec4 dst) {
  return src + dst * (1.0 - src.a);
}

void main() {
  vec4 fgRGBA = texture(foregroundTexture, texCoords.st);

  if (sharpenForeground != 0) {
    float scale = 1.0 + sharpenForeground;
    float minLevel = textureQueryLod(foregroundTexture, texCoords.st).x;
    float maxLevel = minLevel + 5.0;
    // Get RGBA and convert from premultiplied alpha to straight alpha.
    vec4 rawRGBAPrev = textureLod(foregroundTexture, texCoords.st, maxLevel);
    rawRGBAPrev.rgb *= min(1.0/rawRGBAPrev.a, 1e3);
    // Initialize fgRGBA from the base color.
    vec3 sharpenedRGB = rawRGBAPrev.rgb;
    for (float level = maxLevel - 1.0; level >= minLevel; level -= 1.0) {
      // Get RGBA and convert from premultiplied alpha to straight alpha.
      vec4 rawRGBACurr = textureLod(foregroundTexture, texCoords.st, level);
      rawRGBACurr.rgb *= min(1.0/rawRGBACurr.a, 1e3);
      // Enhance the detail in this layer and add to fgRGBA
      sharpenedRGB += scale * (rawRGBACurr.rgb - rawRGBAPrev.rgb);
      rawRGBAPrev = rawRGBACurr;
    }
    // Convert from straight alpha back to premultiplied alpha and store.
    fgRGBA.rgb = fgRGBA.a * sharpenedRGB;
  }

  vec4 outRGBA = vec4(0.0);

  if (shadowOpacity != 0) {
    // Compute texture location to sample foreground alpha for drop shadow.
    vec2 shadowCoords = shadowOrigin + invShadowScale * (texCoords - shadowOrigin) - shadowOffset;
    // Instead of sampling the texture once, take 4 samples and average them to remove blocky artifacts.
    vec2 px = vec2(pow(2.0, shadowLod)) / vec2(textureSize(foregroundTexture, 0));
    float fgAlpha = 0.25 * ((textureLod(foregroundTexture, shadowCoords + 0.5 * vec2( px.x,  px.y), shadowLod - 1.0).a +
                             textureLod(foregroundTexture, shadowCoords + 0.5 * vec2( px.x, -px.y), shadowLod - 1.0).a) +
                            (textureLod(foregroundTexture, shadowCoords + 0.5 * vec2(-px.x,  px.y), shadowLod - 1.0).a +
                             textureLod(foregroundTexture, shadowCoords + 0.5 * vec2(-px.x, -px.y), shadowLod - 1.0).a));
    outRGBA = vec4(0.0, 0.0, 0.0, clamp(shadowOpacity, 0.0, 1.0) * fgAlpha);
  }

  outRGBA = CompositeOver(fgRGBA, outRGBA);

  FragColor = outRGBA;
}
)""
