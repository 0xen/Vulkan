#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (set = 1, binding = 0) uniform sampler2D diffuseTex;


layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;


const vec3 ambientLightColor = vec3(0.2f,0.2f,0.2f);
const vec3 lightDir = normalize(vec3(1.0f,0.0f,1.0f));
const vec3 lightColor = vec3(1.0f,1.0f,1.0f);


void main() 
{
	vec3 texColor = texture(diffuseTex, inUV).xyz;
	float diff = max(dot(normalize(inNormal), lightDir), 0.0f);
	vec3 diffuse = diff * lightColor;

	vec3 result = (ambientLightColor + diffuse) * texColor;


	outColor = vec4(result,1.0f);
}
