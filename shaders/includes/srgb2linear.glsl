vec4 srgb2linear(vec4 srgb) {
    vec3 linear = pow(srgb.rgb, vec3(2.2));
    return vec4(linear, srgb.a);
}