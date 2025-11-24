#pragma once

#include "Mesh.h"
#include <vector>

class MeshFactory {
  public:
    // Create basic shapes

    MeshFactory() = delete; // Static class, no instances

    static Mesh CreateTriangle();

    static Mesh CreateQuad();

    static Mesh CreateCube();

    static Mesh CreateSphere(int segments = 32, int rings = 16);

    static Mesh CreateCapsule(float radius = 0.5f, float height = 1.0f, int segments = 16);

    static Mesh CreateCylinder(float radius = 0.5f, float height = 1.0f, int segments = 16);

  private:
    // Helper functions
    static void addVertex(std::vector<float>& vertices, float x, float y, float z, float r, float g,
                          float b);
};
