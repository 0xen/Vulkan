#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct Camera
{
    mat4 projection;
    mat4 position;
};

layout (binding=0, set = 0) readonly uniform CameraBuffer {Camera camera; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
	gl_Position = camera.projection * camera.position * vec4(inPosition, 1.0f);
	outColor = inColor;
}