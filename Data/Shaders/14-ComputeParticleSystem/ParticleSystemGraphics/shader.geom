#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec4 inColor[];


layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;

void main() {    
	float scale = 0.3f;

    vec4 position = gl_in[0].gl_Position;
	
    outColor = inColor[0];
    outUV = vec2( 0.0f, 0.0f );
    gl_Position = position + (vec4(-0.5f, -0.5f, 0.0f, 0.0f) * scale);
    EmitVertex();
    outColor = inColor[0];
    outUV = vec2( 1.0f, 0.0f );
    gl_Position = position + (vec4(0.5f, -0.5f, 0.0f, 0.0f) * scale);
    EmitVertex();
    outColor = inColor[0];
    outUV = vec2( 0.0f, 1.0f );
    gl_Position = position + (vec4(-0.5f, 0.5f, 0.0f, 0.0f) * scale);
    EmitVertex();
    outColor = inColor[0];
    outUV = vec2( 1.0f, 1.0f );
    gl_Position = position + (vec4(0.5f, 0.5f, 0.0f, 0.0f) * scale);
    EmitVertex();


    EndPrimitive();

}  