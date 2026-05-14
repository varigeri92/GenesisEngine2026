#pragma once

namespace gns
{
    struct Mesh;
}

class DefaultMesh
{
    enum class MeshType
    {
        Quad, Plane, Cube, Sphere, Capsule, Torus, Knot
    };
    
public:
    //static gns::Reference<gns::Mesh> Create(MeshType meshType);
};
