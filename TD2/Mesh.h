#pragma once
#include "Vertex.h"
#include <stdint.h>
#include <vector>
#include "Material.h"

struct Mesh {
	std::vector<Vertex> vertices;
	uint32_t vertexCount;
	std::vector<unsigned short> indices;
	Material mat;
};