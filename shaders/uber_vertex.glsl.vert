#version 450

layout (location = 0) in vec3 inPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform constants
{
	mat4 renderMatrix;
} PushConstants;

void main()
{
	gl_Position = PushConstants.renderMatrix * vec4(inPos, 1.0f);
	outColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
}