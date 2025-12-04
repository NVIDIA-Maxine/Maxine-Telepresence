R""(
#version 450 core

out vec4 FragColor;
in vec2 texCoords;

layout(binding = 0) uniform sampler2D screenTexture;  // rgba

uniform float vignetteIntensity = 0.25;
uniform vec2 quiltTiling = vec2(1.0, 1.0);

vec3 vignette(vec3 color)
{
	vec2 uv = vec2(fract(texCoords.x * quiltTiling.x), fract(texCoords.y * quiltTiling.y));
	uv *= vec2(1.0, 1.0) - uv;
	float vig = uv.x * uv.y * 16.0; // multiply with sth for intensity
	vec3 col = color * pow(vig, vignetteIntensity); // change pow for modifying the extend of the  vignette
	return col;
}

void main()
{
	vec3 col = texelFetch(screenTexture, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0).rgb;
	col = vignette(col);
    FragColor = vec4(col, 1.0);
}
)""
