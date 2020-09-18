
struct Camera
{
    mat4 projection;
    mat4 position;
};


struct RayPayload
{
  vec4 color;
};

struct Vertex
{
  vec3 pos;
  vec3 nrm;
  vec3 color;
  vec2 texCoord;
  int matIndex;
};