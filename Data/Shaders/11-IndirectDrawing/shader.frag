#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (set = 1, binding = 0) uniform sampler2D diffuseTex;


layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() 
{
	//outColor = vec4(inColor, 1.0f);
	outColor = texture(diffuseTex, inUV);
}
