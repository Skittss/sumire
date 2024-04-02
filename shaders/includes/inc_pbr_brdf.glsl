#define PI 3.1415

vec3 BRDF_lambertian(vec3 albedo) {
	return albedo / vec3(PI);
}