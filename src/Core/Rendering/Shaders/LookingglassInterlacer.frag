R""(
#version 420 core

in vec2 texCoords;
out vec4 fragColor;

// Calibration values
uniform float pitch;
uniform float tilt;
uniform float center;
uniform int invView;
uniform float subp;
uniform float displayAspect;
uniform int ri;
uniform int bi;

// Quilt settings
uniform vec3 tile;
uniform vec2 viewPortion;
uniform float quiltAspect;
uniform int overscan;
uniform int quiltInvert;

layout(binding = 0) uniform sampler2D screenTex; // rgba
layout(binding = 1) uniform sampler2D screenDepthTex; // single color
layout(binding = 1) uniform sampler2D rangeTexture; // single color

uniform unsigned int applyVignette;
uniform float vignetteIntensity = 0.25;

vec2 texArr(vec3 uvz)
{
	// decide which section to take from based on the z.
	float x = (mod(uvz.z, tile.x) + uvz.x) / tile.x;
	float y = (floor(uvz.z / tile.x) + uvz.y) / tile.y;
	return vec2(x, y) * viewPortion.xy;
}

// recreate CG clip function (clear pixel if any component is negative)
void clip(vec3 toclip)
{
	if (any(lessThan(toclip, vec3(0,0,0)))) discard;
}

vec3 vignette(vec3 color, vec2 uv)
{
	uv *= 1.0 - uv.yx;
	float vig = uv.x * uv.y * 15.0; // multiply with sth for intensity
	vec3 col = color * pow(vig, vignetteIntensity); // change pow for modifying the extend of the  vignette
	return col;
}

void main()
{
	float invert = 1.0;
	if (invView + quiltInvert == 1) invert = -1.0;

	// correct texel coordinates
	vec3 nuv = vec3(texCoords.xy, 0.0);
	nuv -= 0.5;

	float modx = clamp (step(quiltAspect, displayAspect) * step(float(overscan), 0.5) + step(displayAspect, quiltAspect) * step(0.5, float(overscan)), 0, 1);
	nuv.x = modx * nuv.x * displayAspect / quiltAspect + (1.0-modx) * nuv.x;
	nuv.y = modx * nuv.y + (1.0-modx) * nuv.y * quiltAspect / displayAspect; 
	nuv += 0.5;

	clip (nuv);
	clip (1.0-nuv);

	vec4 rgb[3];
		
	// compute colors individually
	for (int i=0; i < 3; i++)
	{
		nuv.z = (texCoords.x + i * subp + texCoords.y * tilt) * pitch - center;
		nuv.z = mod(nuv.z + ceil(abs(nuv.z)), 1.0);
		nuv.z *= invert;
		nuv.z *= tile.z;

		vec3 coords1 = nuv;
		vec3 coords2 = nuv;

		coords1.y = coords2.y = clamp(nuv.y, 0.005, 0.995);
		coords1.z = floor(nuv.z);
		coords2.z = ceil(nuv.z);

		vec4 col1 = texture(screenTex, texArr(coords1));
		vec4 col2 = texture(screenTex, texArr(coords2));

		rgb[i] = mix(col1, col2, nuv.z - coords1.z);
	}
		
	vec3 col = vec3(rgb[ri].r, rgb[1].g, rgb[bi].b);

	if (applyVignette > 0)
		col = vignette(col, texCoords); // we can directly use normalized input tex coords for full screen pass

	// write RGB tupel
	fragColor = vec4(col, 1.0);
	
}
)""
