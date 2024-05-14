// Various implementations of attenuating a light source
//  The physically correct formula is inverse square law: 1/d^2
//  This doesn't play so nicely with clustered lighting as we need a cutoff for intersections

// ---- Cusped & Non-Cusped Distance Attenuation -----------------------------------------------------------------
//  Formula from the following article:
//  (https://lisyarus.github.io/blog/posts/point-light-attenuation.html)
// 
//       Li = A(1 - s^2)^2 / (1 + Fs^2) | s < 1
//           where s = d / R, A is maximum intensity, and F is a user controlled falloff.
// 
// Physically correct attenuation for reference:
//  vec3 Li = light.color.rgb * light.color.a / (lightDist * lightDist);

float attDistanceNoCusp(float d, float R, float A, float F) {
    float s = d / R;

    if (s >= 1.0) return 0.0;

    float s2 = s * s;
    float one_minus_s2 = 1 - s2;

    return A * one_minus_s2 * one_minus_s2 / (1.0 + F * s2);
}

// Variation with a cusp approximates a spherical light
//  and adds a C^1 discontinuity at max range by dropping s2 -> s in the denominator
float attDistanceCusp(float d, float R, float A, float F) {
    float s = d / R;

    if (s >= 1.0) return 0.0;

    float s2 = s * s;
    float one_minus_s2 = 1 - s2;

    return A * one_minus_s2 * one_minus_s2 / (1.0 + F * s);
}

// ---- Unity Distance Attenuation -------------------------------------------------------------------------------
// https://geom.io/bakery/wiki/index.php?title=Point_Light_Attenuation
float attDistanceUnity(float d, float R) {
    float s = d / R;
    float sx5 = s * 5.0;

    return 1.0 / (sx5 * sx5 + 1);
}

// ---- Frostbite PBR Attenuation (Distance & Angle) -------------------------------------------------------------
// From Siggraph 2014: "Moving Frostbite to Physically based rendering V3"
//   Course notes, p28-33
//   https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
// 

float smoothDistanceAtt(float distSquared, float invSqrAttRadius) {
    float factor = distSquared * invSqrAttRadius;
    float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0); // saturate equivalent
    return smoothFactor * smoothFactor;
}

//
//   Att = 1 / d^2 * saturate(1 - x^n / R^n )^2
// 
// s = Light Size (not range), typically a small value e.g. 0.01.
float attDistanceFrosbite(vec3 unnormalizedLightVector, float invSqrAttRadius, float s) {
    float sqrDist = dot(unnormalizedLightVector, unnormalizedLightVector);
    float attenuation = 1.0 / (max(sqrDist, s * s)); // Inverse square law, clamped at asymptote
    attenuation *= smoothDistanceAtt(sqrDist, invSqrAttRadius);

    return attenuation;
}

float attAngleFrostbite(
    vec3 normalizedLightVector, vec3 lightDir, 
    float lightAngleScale, float lightAngleOffset
) {
    float cd = dot(lightDir, normalizedLightVector);
    float attenuation = clamp(cd * lightAngleScale + lightAngleOffset, 0.0, 1.0);
    // smooth the transition
    attenuation *= attenuation;

    return attenuation;
}

// ---------------------------------------------------------------------------------------------------------------