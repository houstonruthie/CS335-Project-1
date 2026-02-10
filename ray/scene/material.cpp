#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI *traceUI;

#include "../fileio/images.h"
#include <glm/gtx/io.hpp>
#include <iostream>

using namespace std;
extern bool debugMode;

Material::~Material() {}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene *scene, const ray &r, const isect &i) const {
  // Start with ambient term
  glm::dvec3 color = ka(i) * scene->ambient();

  // Surface normal at intersection
  glm::dvec3 N = glm::normalize(i.getN());

  // View direction (toward the camera)
  glm::dvec3 V = glm::normalize(-r.getDirection());

  // Loop over all lights in the scene
  for (const auto &pLight : scene->getAllLights()) {
    // Direction to light
    glm::dvec3 P = r.at(i.getT());
    glm::dvec3 L = glm::normalize(pLight->getDirection(P));
    const double eps = 1e-6;


    // Light color
    glm::dvec3 lightColor = pLight->getColor();

    // Diffuse (Lambert)
double NdotL = std::max(0.0, glm::dot(N, L));

// Distance attenuation
double atten = pLight->distanceAttenuation(P);

// Specular (Phong)
glm::dvec3 R = glm::reflect(-L, N);
double RdotV = std::max(0.0, glm::dot(R, V));

// Compute components
glm::dvec3 diffuse =
    kd(i) * lightColor * NdotL;

glm::dvec3 specular =
    ks(i) * lightColor * pow(RdotV, shininess(i));

// Construct shadow ray from surface point toward light
ray shadowRay(
    P + eps * N,   // origin (offset to avoid self-intersection)
    L,             // direction toward light
    glm::dvec3(1.0),
    ray::SHADOW
);

// Shadow attenuation
glm::dvec3 shadow = pLight->shadowAttenuation(shadowRay, P);


// Accumulate
color += atten * shadow * (diffuse + specular);


  }

  return color;
}


TextureMap::TextureMap(string filename) {
  data = readImage(filename.c_str(), width, height);
  if (data.empty()) {
    width = 0;
    height = 0;
    string error("Unable to load texture map '");
    error.append(filename);
    error.append("'.");
    throw TextureMapException(error);
  }
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2 &coord) const {
  // YOUR CODE HERE
  //
  // In order to add texture mapping support to the
  // raytracer, you need to implement this function.
  // What this function should do is convert from
  // parametric space which is the unit square
  // [0, 1] x [0, 1] in 2-space to bitmap coordinates,
  // and use these to perform bilinear interpolation
  // of the values.

  return glm::dvec3(1, 1, 1);
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const {
  // YOUR CODE HERE
  //
  // In order to add texture mapping support to the
  // raytracer, you need to implement this function.

  return glm::dvec3(1, 1, 1);
}

glm::dvec3 MaterialParameter::value(const isect &is) const {
  if (0 != _textureMap)
    return _textureMap->getMappedValue(is.getUVCoordinates());
  else
    return _value;
}

double MaterialParameter::intensityValue(const isect &is) const {
  if (0 != _textureMap) {
    glm::dvec3 value(_textureMap->getMappedValue(is.getUVCoordinates()));
    return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
  } else
    return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}
