// Attenuate a light source
//  The physically correct formula is inverse square law: 1/d^2
//  This doesn't play so nicely with clustered lighting as we need a cutoff for intersections
//  So instead the following formula is used:
//       Li = A(1 - s^2)^2 / (1 + Fs^2) | s < 1
//           where s = d / R, A is maximum intensity, and F is a user controlled falloff.
// 
//  (https://lisyarus.github.io/blog/posts/point-light-attenuation.html)
// 
// Physically correct attenuation for reference:
//  vec3 Li = light.color.rgb * light.color.a / (lightDist * lightDist);

// Variation with a cusp approximates a spherical light
//  and adds a C^1 discontinuity at max range by dropping s2 -> s in the denominator
float attenuateCusp(float d, float R, float A, float F) {
    float s = d / R;

    if (s >= 1.0) return 0.0;

    float s2 = s * s;
    float one_minus_s2 = 1 - s2;

    return A * one_minus_s2 * one_minus_s2 / (1.0 + F * s);
}

float attenuateNoCusp(float d, float R, float A, float F) {
    float s = d / R;

    if (s >= 1.0) return 0.0;

    float s2 = s * s;
    float one_minus_s2 = 1 - s2;

    return A * one_minus_s2 * one_minus_s2 / (1.0 + F * s2);
}