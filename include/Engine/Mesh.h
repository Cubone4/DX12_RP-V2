#pragma once
#include <Engine/Transform.h>
#include <Engine/Vertex.h>
#include <vector>

struct Mesh{
    Transform transform;
    std::vector<Vertex> vertices;
};