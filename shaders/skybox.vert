#version 450

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 outPos;

layout (push_constant) uniform constants
{
	mat4 transform;
	uint textureIndex;
} pushConstants;

layout (set = 0, binding = 0) uniform CameraBuffer
{
	mat4 viewProj;
	mat4 lightSpaceMatrix;
} cameraData;

void main()
{
	outPos = inPos;
	gl_Position = pushConstants.transform * vec4(inPos, 1.0f);
}