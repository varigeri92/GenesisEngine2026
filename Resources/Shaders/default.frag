#version 450

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;

//output write
layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(albedoTexture, inUV);
}
