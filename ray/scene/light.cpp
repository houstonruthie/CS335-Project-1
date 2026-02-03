#include <cmath>
#include <iostream>

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

using namespace std;

double DirectionalLight::distanceAttenuation(const glm::dvec3 &) const {
  // distance to light is infinite, so f(di) goes to 0.  Return 1.
  return 1.0;
}

glm::dvec3 DirectionalLight::shadowAttenuation(const ray &, const glm::dvec3 &p) const {
  // Direction from point toward the light

  glm::dvec3 L = -orientation;

  // Small offset to avoid self-intersection
  const double eps = 1e-6;
  ray shadowRay(p + eps * L, L, glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);

  isect i;
  if (scene->intersect(shadowRay, i)) {
    return glm::dvec3(0.0, 0.0, 0.0); // fully shadowed
  }

  return glm::dvec3(1.0, 1.0, 1.0); // fully lit
}


glm::dvec3 DirectionalLight::getColor() const { return color; }

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3 &) const {
  return -orientation;
}

double PointLight::distanceAttenuation(const glm::dvec3 &P) const {
  double d = glm::length(position - P);

  // Quadratic attenuation: 1 / (1 + c * d^2)
  double atten = 1.0 / (1.0 + 0.1 * d * d);

  // Clamp so we never brighten the light
  return std::min(1.0, atten);
}


glm::dvec3 PointLight::getColor() const { return color; }

glm::dvec3 PointLight::getDirection(const glm::dvec3 &P) const {
  return glm::normalize(position - P);
}

glm::dvec3 PointLight::shadowAttenuation(const ray &, const glm::dvec3 &p) const {
  glm::dvec3 toLight = position - p;
  double maxDist = glm::length(toLight);
  glm::dvec3 L = glm::normalize(toLight);

  const double eps = 1e-6;
  ray shadowRay(p + eps * L, L, glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
  isect i;
  if (scene->intersect(shadowRay, i)) {
    if (i.getT() < maxDist) {
      return glm::dvec3(0.0, 0.0, 0.0); // blocked before light
    }
  }

  return glm::dvec3(1.0, 1.0, 1.0);
}


#define VERBOSE 0

