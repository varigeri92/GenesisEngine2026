#pragma once
#include <glm/glm.hpp>
#include "IObject.h"

namespace gns
{
    struct BufferRange
    {
        uint32_t startIndex;
        uint32_t count;
    };
    
    struct Mesh : public gns::Object
    {
        BufferRange bufferRange;           
        std::vector<uint32_t> indices;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec4> colors;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec3> tangents;
        std::vector<glm::vec3> bitangents;
        std::vector<glm::vec2> uvs;
        //std::vector<std::vector<glm::vec2>> UVs;
    
        Mesh() = default;
        Mesh(std::string name) : Object(name){}
        Mesh(gns::Handle handle, std::string name) : Object(handle, name){}
        Mesh(gns::Handle handle, std::string name, size_t indexCount, size_t vertexCount) : Object(handle, name)
        {
            indices.reserve(indexCount);
            positions.reserve(vertexCount);
            colors.reserve(vertexCount);
            normals.reserve(vertexCount);
            tangents.reserve(vertexCount);
            bitangents.reserve(vertexCount);
            uvs.reserve(1);
        }
        
    private:
        void FreeCPUSide();
        friend class RenderSystem;
    };
}
