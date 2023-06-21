#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 inNorm;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTex;

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform constants
{
	mat4 transform;
	uint textureIndex;
} pushConstants;

layout (set = 1, binding = 0) uniform sampler2D textures[];

void main()
{
	outColor = texture(textures[pushConstants.textureIndex], inTex);
	//outColor = vec4(inTex.x, inTex.y, 0.0f, 1.0f);
}