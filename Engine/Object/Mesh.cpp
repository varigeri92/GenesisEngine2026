#include "gnspch.h"
#include "Mesh.h"

void gns::Mesh::FreeCPUSide()
{
    indices.clear();
    indices.reserve(0);
    positions.clear();
    positions.reserve(0);
    colors.clear();
    colors.reserve(0);
    normals.clear();
    normals.reserve(0);
    tangents.clear();
    tangents.reserve(0);
    bitangents.clear();
    bitangents.reserve(0);
}
