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
	if (pushConstants.textureIndex == 1001)
	{
		// There is no texture
		outColor = vec4(inColor, 1.0f);
	}
	else
	{
		outColor = texture(textures[pushConstants.textureIndex], inTex);
	}

	if (outColor.a == 0.0f)
	{
		discard;
	}
}