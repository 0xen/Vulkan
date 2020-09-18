#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "../utils/Structures.glsl"

layout(binding = 2, set = 0) readonly uniform CameraBuffer {Camera camera; };

layout(location = 0) rayPayloadInNV RayPayload rayPayload;

void main()
{
   	// View-independent background gradient to simulate a basic sky background
	const vec3 gradientStart = vec3(0.0, 0.6, 1.0);
	const vec3 gradientEnd = vec3(1.0, 0.6, 0.0);
	vec3 unitDir = normalize(gl_WorldRayDirectionNV);
	float t = 0.5 * (unitDir.y + 1.0);
	rayPayload.color.rgb = (1.0-t) * gradientEnd + t * gradientStart;
}