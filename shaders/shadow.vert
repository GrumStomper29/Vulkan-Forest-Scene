#version 450

layout (location = 0) in vec3 inPos;

layout (push_constant) uniform constants
{
	mat4 model;
	uint texIndex;
} pushConstants;

layout (set = 0, binding = 0) uniform CameraBuffer
{
	mat4 viewProj;
	mat4 light;
} cameraData;

void main()
{
	gl_Position = cameraData.light * pushConstants.model * vec4(inPos, 1.0f);
}