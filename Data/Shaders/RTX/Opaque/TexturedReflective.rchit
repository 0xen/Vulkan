#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "../utils/HitgroupHelpers.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;

layout(location = 0) rayPayloadInNV RayPayload inRayPayload;

layout(location = 1) rayPayloadNV RayPayload outRayPayload;

void main()
{

  	ivec3 ind = ivec3(
	  	indices.i[3 * gl_PrimitiveID], 
	  	indices.i[3 * gl_PrimitiveID + 1],
	  	indices.i[3 * gl_PrimitiveID + 2]);


  	Vertex v0 = unpackVertex(ind.x);
  	Vertex v1 = unpackVertex(ind.y);
  	Vertex v2 = unpackVertex(ind.z);


	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	vec3 normal = normalize(HitBarycentrics(v0.nrm, v1.nrm, v2.nrm, barycentrics));

	vec2 uv = HitBarycentrics(v0.texCoord, v1.texCoord, v2.texCoord, barycentrics);

	vec3 texColor = texture(textureSamplers[0], uv).xyz;



	vec3 metalness = texture(textureSamplers[1], uv).xyz;



	vec3 viewVector = normalize(gl_WorldRayDirectionNV);

	vec3 reflectVec = reflect(viewVector, normal);

	vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;

    // Min-max range of the ray that we spawn
	const float tmin = 0.001;
	const float tmax = 10000.0;

   	int MISS_SHADER_INDEX = 0;

   	traceNV(topLevelAS, gl_RayFlagsOpaqueNV | gl_RayFlagsCullBackFacingTrianglesNV,
		0xFF, 0, 0, MISS_SHADER_INDEX, origin.xyz, tmin, reflectVec.xyz, tmax, 1);

	vec3 rayColor = outRayPayload.color.rgb;
	//rayColor.rgb *= vec3(metalness,metalness,metalness);
	//= outRayPayload.color.rgb * metalness;
	texColor = rayColor;






	inRayPayload.color.rgb = texColor;
}


