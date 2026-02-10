#include "cubeMap.h"
#include "../scene/material.h"
#include "../ui/TraceUI.h"
#include "ray.h"
extern TraceUI *traceUI;

glm::dvec3 CubeMap::getColor(ray r) const {
    glm::dvec3 d = glm::normalize(r.getDirection());

    double x = d.x;
    double y = d.y;
    double z = d.z;

    double ax = std::abs(x);
    double ay = std::abs(y);
    double az = std::abs(z);

    int face;
    double u, v;

    if (ax >= ay && ax >= az) {
        // X face
        if (x > 0) {
            face = 0;  // +X
            u = -z / ax;
            v = -y / ax;
        } else {
            face = 1;  // -X
            u =  z / ax;
            v = -y / ax;
        }
    } else if (ay >= ax && ay >= az) {
        // Y face
        if (y > 0) {
            face = 2;  // +Y
            u =  x / ay;
            v =  z / ay;
        } else {
            face = 3;  // -Y
            u =  x / ay;
            v = -z / ay;
        }
    } else {
        // Z face
        if (z > 0) {
            face = 4;  // +Z
            u =  x / az;
            v = -y / az;
        } else {
            face = 5;  // -Z
            u = -x / az;
            v = -y / az;
        }
    }

    // Map from [-1, 1] → [0, 1]
    u = 0.5 * (u + 1.0);
    v = 0.5 * (v + 1.0);

    // Clamp for safety
    u = glm::clamp(u, 0.0, 1.0);
    v = glm::clamp(v, 0.0, 1.0);
    // Sample the cube face texture
    if (!tMap[face]) {
        return glm::dvec3(0.0); // fallback background
    }
    return tMap[face]->getMappedValue(glm::dvec2(u, v));
}

//return glm::dvec3(1.0, 0.0, 0.0); // bright red

CubeMap::CubeMap() {}

CubeMap::~CubeMap() {}

void CubeMap::setNthMap(int n, TextureMap *m) {
  if (m != tMap[n].get())
    tMap[n].reset(m);
}
