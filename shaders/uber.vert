#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNorm;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform constants
{
	mat4 renderMatrix;
} PushConstants;

void main()
{
	gl_Position = PushConstants.renderMatrix * vec4(inPos, 1.0f);
	outColor = vec4(inNorm, 1.0f);
}