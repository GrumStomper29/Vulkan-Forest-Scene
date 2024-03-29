#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNorm;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTex;

layout (location = 0) out vec3 outNorm;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outTex;
layout (location = 3) out vec4 fragPosLightSpace;

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
	mat4 finalMatrix = cameraData.viewProj * pushConstants.transform;
	gl_Position = finalMatrix * vec4(inPos, 1.0f);

	mat3 transform = inverse(transpose(mat3(pushConstants.transform)));
	outNorm = normalize(inNorm * transform);
	outColor = inColor;
	outTex = inTex;

	fragPosLightSpace = cameraData.lightSpaceMatrix * pushConstants.transform * vec4(inPos, 1.0f);
}