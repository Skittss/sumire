#define M_PI 3.1415926535897932384626433832795

// Schlick approximations of Fresnel term
//  Based on Real-time Rendering 4th edition, p320 & 321, eqs 9.16 & 9.18
//   f0 only
vec3 BRDF_F_schlick(float VdotHplus, vec3 f0) {
	return f0 + (1.0 - f0) * pow(1.0 - VdotHplus, 5.0);
}
//   Including f90
vec3 BRDF_F_schlick(float VdotHplus, vec3 f0, vec3 f90) {
	return f0 + (f90 - f0) * pow(1.0 - VdotHplus, 5.0);
}
//   Including f90 and p
vec3 BRDF_F_schlick(float VdotHplus, vec3 f0, vec3 f90, float p) {
	return f0 + (f90 - f0) * pow(1.0 - VdotHplus, 5.0);
}

vec3 BRDF_lambertian(vec3 albedo) {
	return albedo / vec3(M_PI);
}

// Trowbridge-Reitz / GGX Distribution of Normals
//  Based on Real-time Rendering 4th edition, p340
float BRDF_D_GGX(float alpha_roughness, float NdotHplus) {
	float alpha_sq = alpha_roughness * alpha_roughness;
	float denom = (1.0 + (NdotHplus * NdotHplus) * (alpha_sq - 1.0));
	return (alpha_sq) / (M_PI * denom * denom);
}

// Heitz (2014) Smith Joint Masking Shadow Function.
//  Combination of G term and the specular denominator, calculated this way for computational efficiency
//  Based on Real-time Rendering 4th edition, p341 eq 9.43
float BRDF_V_GGX(float alpha_roughness, float NdotLplus, float NdotVplus) {
	float alpha_sq = alpha_roughness * alpha_roughness;

	float GGXV = NdotLplus * sqrt(alpha_sq + NdotVplus * NdotVplus * (1.0 - alpha_sq));
	float GGXL = NdotVplus * sqrt(alpha_sq + NdotLplus * NdotLplus * (1.0 - alpha_sq));
	
	return 0.5 / (GGXV + GGXL);
}

// Cook-Torrence Microfacet BRDF specular component (f_s) using GGX distribution.
//  f_s = DFG / 4(wr . n)(wi . n)
//
// Note: ...plus variables are floats with the Heaviside step / Positive Characteristic function applied:
//
//   X+(x) = | 1, where x >  0
//           | 0, where x <= 0
//
// i.e. applied to a dot product, reduced to clamp(a . b, 0.0, 1.0)
// See Real-time Rendering 4th Edition: p334 eq 9.25
vec3 BRDF_specular_GGX(float alpha_roughness, vec3 f0, float NdotLplus, float NdotVplus, float NdotHplus, float VdotHplus) {
	float D = BRDF_D_GGX(alpha_roughness, NdotHplus);
	vec3 F = BRDF_F_schlick(VdotHplus, f0);
	// V = G / 4(wr . n)(wi. n)
	float V = BRDF_V_GGX(alpha_roughness, NdotLplus, NdotVplus);

	return D * F * V;
}