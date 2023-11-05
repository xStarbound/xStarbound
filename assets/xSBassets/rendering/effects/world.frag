#version 110

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform bool lightMapEnabled;
uniform vec2 lightMapSize;
uniform vec2 tileLightMapSize;
uniform sampler2D lightMap;
uniform sampler2D tileLightMap;
uniform float lightMapMultiplier;

// xSB-2: Custom shader parameters.
uniform vec3 param1;
uniform vec3 param2;
uniform vec3 param3;
uniform vec3 param4;
uniform vec3 param5;
uniform vec3 param6;

varying vec2 fragmentTextureCoordinate;
varying float fragmentTextureIndex;
varying vec4 fragmentColor;
varying float fragmentLightMapMultiplier;
varying vec2 fragmentLightMapCoordinate;

vec4 cubic(float v) {
  vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
  vec4 s = n * n * n;
  float x = s.x;
  float y = s.y - 4.0 * s.x;
  float z = s.z - 4.0 * s.y + 6.0 * s.x;
  float w = 6.0 - x - y - z;
  return vec4(x, y, z, w);
}

vec4 bicubicSample(sampler2D texture, vec2 texcoord, vec2 texscale) {
  texcoord = texcoord - vec2(0.5, 0.5);

  float fx = fract(texcoord.x);
  float fy = fract(texcoord.y);
  texcoord.x -= fx;
  texcoord.y -= fy;

  vec4 xcubic = cubic(fx);
  vec4 ycubic = cubic(fy);

  vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y - 0.5, texcoord.y + 1.5);
  vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
  vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

  vec4 sample0 = texture2D(texture, vec2(offset.x, offset.z) * texscale);
  vec4 sample1 = texture2D(texture, vec2(offset.y, offset.z) * texscale);
  vec4 sample2 = texture2D(texture, vec2(offset.x, offset.w) * texscale);
  vec4 sample3 = texture2D(texture, vec2(offset.y, offset.w) * texscale);

  float sx = s.x / (s.x + s.y);
  float sy = s.z / (s.z + s.w);

  return mix(
    mix(sample3, sample2, sx),
    mix(sample1, sample0, sx), sy);
}

vec3 sampleLightMap(vec2 texcoord, vec2 texscale) {
  vec4 b = bicubicSample(tileLightMap, texcoord, texscale);
  vec4 a = bicubicSample(lightMap, texcoord, texscale);

  if (b.z <= 0.0)
    return a.rgb;

  return mix(a.rgb, b.rgb / b.z, b.z);
}

// FezzedOne: Sample scriptable colour adjustment function.
vec4 applyColourAdjustments(vec4 inputColourRaw, vec3 tripleSettings, float rgbMultiply, vec3 rgbMultiplier) {
  float brightness = tripleSettings[0];
  float contrast = tripleSettings[1];
  float desaturation = tripleSettings[2];

  vec3 inputColour = vec3(inputColourRaw);
  float alpha = inputColourRaw.a;

  // Adjust brightness.
  inputColour = inputColour * (brightness + 1.0);

  // Adjust contrast.
  inputColour = mix(inputColour, vec3(0.5), contrast);

  // Adjust saturation.
  vec3 monochromeBalance = vec3(0.2126, 0.7152, 0.0722);
  vec3 grey = vec3(dot(monochromeBalance, inputColour));
  inputColour = mix(inputColour, grey, desaturation);

  // Multiply by the RGB multiplier if rgbMultiply is non-zero.
  if (rgbMultiply != 0.0) {
    inputColour *= rgbMultiplier;
	}

  vec4 outputColour = vec4(inputColour, alpha);

  return outputColour;
}

void main() {
  vec4 texColor;
  if (fragmentTextureIndex > 2.9) {
    texColor = texture2D(texture3, fragmentTextureCoordinate);
  } else if (fragmentTextureIndex > 1.9) {
    texColor = texture2D(texture2, fragmentTextureCoordinate);
  } else if (fragmentTextureIndex > 0.9) {
    texColor = texture2D(texture1, fragmentTextureCoordinate);
  } else {
    texColor = texture2D(texture0, fragmentTextureCoordinate);
  }
  if (texColor.a <= 0.0)
    discard;

  vec4 finalColor = texColor * fragmentColor;
  float finalLightMapMultiplier = fragmentLightMapMultiplier * lightMapMultiplier;
  if (texColor.a == 0.99607843137)
    finalColor.a = fragmentColor.a;
  else if (lightMapEnabled && finalLightMapMultiplier > 0.0)
    finalColor.rgb *= sampleLightMap(fragmentLightMapCoordinate, 1.0 / lightMapSize) * finalLightMapMultiplier;
  if (lightMapEnabled)
    finalColor = applyColourAdjustments(finalColor, param1, param3[0], param2);
  gl_FragColor = finalColor;
}