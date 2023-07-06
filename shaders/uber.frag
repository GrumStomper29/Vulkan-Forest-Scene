#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 inNorm;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTex;
layout (location = 3) in vec4 inLightPos;

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform constants
{
	mat4 transform;
	uint textureIndex;
} pushConstants;

layout (set = 1, binding = 0) uniform sampler2D textures[];

float shadowCalc(vec4 pos)
{
	vec3 projCoords = pos.xyz / pos.w;

	projCoords.xy = projCoords.xy * 0.5f + 0.5f;

	float closestDepth = texture(textures[1000], projCoords.xy).r;
	float currentDepth = projCoords.z;

	float bias = 0.005;

	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

	return shadow;
}

void main()
{
	float diffuse = max(dot(inNorm, vec3(1.0f, 1.0f, 1.0f)), 0.0f);

	if (pushConstants.textureIndex == 1001)
	{
		// There is no texture
		outColor = vec4(inColor, 1.0f);
	}
	else
	{
		outColor = texture(textures[pushConstants.textureIndex], inTex);
	}

	if (outColor.a <= 0.2f)
	{
		discard;
	}

	diffuse = max(diffuse, 0.7f);

	float shadow = max(1.0 - shadowCalc(inLightPos), 0.4f);

	outColor = vec4(outColor.rgb * diffuse * shadow, outColor.a);
	//outColor = vec4(1.0 - shadow, 1.0 - shadow, 1.0 - shadow, outColor.a);
}