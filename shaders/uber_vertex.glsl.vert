#version 450

layout(location = 0) out vec4 outColor;

void main()
{
	const vec3 vertices[3] = vec3[3](
		vec3(1.0f, 1.0f, 0.0f),
		vec3(-1.0f, 1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f)
	);

	const vec4 colors[3] = vec4[3](
		vec4(1.0f, 0.0f, 0.0f, 1.0f),
		vec4(0.0f, 1.0f, 0.0f, 1.0f),
		vec4(0.0f, 0.0f, 1.0f, 1.0f)
	);

	gl_Position = vec4(vertices[gl_VertexIndex], 1.0f);
	outColor = colors[gl_VertexIndex];
}