#include "Structures.glsl"


hitAttributeNV vec3 attribs;

layout(binding = 0, set = 1) buffer Vertices { vec4 v[]; }
vertices;
layout(binding = 1, set = 1) buffer Indices { uint i[]; }
indices;

layout (binding = 2, set = 1) uniform sampler2D[] textureSamplers;



// Number of vec4 values used to represent a vertex
uint vertexSize = 3;

Vertex unpackVertex(uint index)
{
  Vertex v;

  vec4 d0 = vertices.v[vertexSize * index];
  vec4 d1 = vertices.v[vertexSize * index + 1];
  vec4 d2 = vertices.v[vertexSize * index + 2];

  v.pos = d0.xyz;
  v.nrm = vec3(d0.w, d1.x, d1.y);
  v.color = vec3(d1.z, d1.w, d2.x);
  v.texCoord = vec2(d2.y, d2.z);
  v.matIndex = floatBitsToInt(d2.w);
  return v;
}

vec2 HitBarycentrics(vec2 a, vec2 b, vec2 c, vec3 bary)
{
  //return a + bary.x * (b - a) + bary.y * (c - a);
  return a * bary.x + b * bary.y + c * bary.z;
}

vec3 HitBarycentrics(vec3 a, vec3 b, vec3 c, vec3 bary)
{
  //return a + bary.x * (b - a) + bary.y * (c - a);
  return a * bary.x + b * bary.y + c * bary.z;
}

vec4 HitBarycentrics(vec4 a, vec4 b, vec4 c, vec3 bary)
{
  //return a + bary.x * (b - a) + bary.y * (c - a);
  return a * bary.x + b * bary.y + c * bary.z;
}

