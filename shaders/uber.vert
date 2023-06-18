#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNorm;

layout (location = 0) out vec3 outColor;

layout (push_constant) uniform constants
{
	mat4 vertexTransform;
} PushConstants;

layout (set = 0, binding = 0) uniform CameraBuffer
{
	mat4 viewProj;
} cameraData;

void main()
{
	mat4 finalMatrix = cameraData.viewProj * PushConstants.vertexTransform;
	gl_Position = finalMatrix * vec4(inPos, 1.0f);
	outColor = inNorm;
}