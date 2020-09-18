#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "../utils/HitgroupHelpers.glsl"

layout(location = 0) rayPayloadInNV RayPayload rayPayload;

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


	rayPayload.color.rgb = texColor;//vec3(0,0,1);
}


