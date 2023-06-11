#include "mesh.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobj/tiny_obj_loader.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace Graphics
{
	
	std::vector<std::uint32_t> loadOBJ(std::string_view path, std::vector<Vertex>& writeVertices)
	{
		tinyobj::ObjReaderConfig readerConfig{};
		readerConfig.vertex_color = true;
		readerConfig.triangulate = true;

		tinyobj::ObjReader reader{};

		reader.ParseFromFile(static_cast<std::string>(path), readerConfig);

		auto& attrib{ reader.GetAttrib() };
		auto& shapes{ reader.GetShapes() };
		auto& materials{ reader.GetMaterials() };

		int materialIndex{ 0 };

		if (!reader.Error().empty())
		{
			std::cout << "tinyobj: error: " << reader.Error() << '\n';
		}

		if (!reader.Warning().empty())
		{
			std::cout << "tinyobj: warning: " << reader.Warning() << '\n';
		}
		
		std::vector<std::uint32_t> indices{};
		std::uint32_t startVert{ static_cast<std::uint32_t>(writeVertices.size()) };

		for (std::size_t s{ 0 }; s < shapes.size(); ++s)
		{
			std::size_t indexOffset{ 0 };
			for (std::size_t f{ 0 }; f < shapes[s].mesh.num_face_vertices.size(); ++f)
			{
				std::size_t fv{ shapes[s].mesh.num_face_vertices[f] };

				for (std::size_t v{ 0 }; v < fv; ++v)
				{
					tinyobj::index_t index{ shapes[s].mesh.indices[indexOffset + v] };

					Vertex newVertex{};
					newVertex.pos.x = attrib.vertices[3 * index.vertex_index + 0];
					newVertex.pos.y = -attrib.vertices[3 * index.vertex_index + 1];
					newVertex.pos.z = -(attrib.vertices[3 * index.vertex_index + 2]);

					if (index.normal_index >= 0)
					{
						newVertex.norm.x = attrib.normals[3 * index.normal_index + 0];
						newVertex.norm.y = attrib.normals[3 * index.normal_index + 1];
						newVertex.norm.z = attrib.normals[3 * index.normal_index + 2];
					}

					if (index.texcoord_index >= 0)
					{
						newVertex.tex.x = attrib.texcoords[2 * index.texcoord_index + 0];
						newVertex.tex.y = 1 - (attrib.texcoords[2 * index.texcoord_index + 1]);
					}

					newVertex.col.r = attrib.colors[3 * index.vertex_index + 0];
					newVertex.col.g = attrib.colors[3 * index.vertex_index + 1];
					newVertex.col.b = attrib.colors[3 * index.vertex_index + 2];

					int material{ shapes[s].mesh.material_ids[f] };

					writeVertices.push_back(newVertex);
					indices.push_back(indices.size() + startVert);
				}

				indexOffset += fv;
			}
		}

		return indices;
	}

}