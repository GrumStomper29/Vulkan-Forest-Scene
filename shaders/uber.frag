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

layout (set = 1, binding = 1) uniform sampler2D textures[];

float shadowCalc(vec4 pos)
{
	vec3 projCoords = pos.xyz / pos.w;

	projCoords.xy = projCoords.xy * 0.5f + 0.5f;

	float closestDepth = texture(textures[1000], projCoords.xy).r;
	float currentDepth = projCoords.z;

	const float bias = 0.005f;

	float shadow = 0.0f;
	vec2 texelSize = 1.0f / textureSize(textures[1000], 0);
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pfcDepth = texture(textures[1000], projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pfcDepth ? 1.0f : 0.0f;
		}
	}
	shadow /= 9.0f;

	return shadow;
}

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

	if (outColor.a <= 0.2f)
	{
		discard;
	}

	const float ambient = 0.1f;

	const vec3 lightDir = normalize(vec3(2.0f, 1.0f, -3.0f));
	float diffuse = max(dot(inNorm, lightDir), 0.0f);
	vec3 diffuseColor = vec3(0.98f, 0.56f, 0.38f) * diffuse;

	float shadow = max(1.0 - shadowCalc(inLightPos), 0.4f);

	outColor = vec4(outColor.rgb * (ambient + diffuseColor) * shadow, outColor.a);
}