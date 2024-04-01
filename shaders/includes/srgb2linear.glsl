vec4 srgb2linear(vec4 srgb) {
	vec3 bLess = step(vec3(0.04045), srgb.xyz);
	vec3 linOut = mix(srgb.xyz / vec3(12.92), pow((srgb.xyz + vec3(0.055))/vec3(1.055), vec3(2.4)), bLess);
}