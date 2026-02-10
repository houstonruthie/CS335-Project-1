// The main ray tracer.

#pragma warning(disable : 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/JsonParser.h"
#include "parser/Parser.h"
#include "parser/Tokenizer.h"
#include <json.hpp>

#include "ui/TraceUI.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h> // for memset

#include <fstream>
#include <iostream>

using namespace std;
extern TraceUI *traceUI;

// Use this variable to decide if you want to print out debugging messages. Gets
// set in the "trace single ray" mode in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates
// (x,y), through the projection plane, and out into the scene. All we do is
// enter the main ray-tracing method, getting things started by plugging in an
// initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

glm::dvec3 RayTracer::trace(double x, double y) {
  // Clear out the ray cache in the scene for debugging purposes,
  if (TraceUI::m_debug) {
    scene->clearIntersectCache();
  }

  ray r(glm::dvec3(0, 0, 0), glm::dvec3(0, 0, 0), glm::dvec3(1, 1, 1),
        ray::VISIBILITY);
  scene->getCamera().rayThrough(x, y, r);
  double dummy;
  glm::dvec3 ret =
      traceRay(r, glm::dvec3(1.0, 1.0, 1.0), traceUI->getDepth(), dummy);
  ret = glm::clamp(ret, 0.0, 1.0);
  return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j) {
  glm::dvec3 col(0, 0, 0);

  if (!sceneLoaded())
    return col;

  double x = double(i) / double(buffer_width);
  double y = double(j) / double(buffer_height);

  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
  col = trace(x, y);

  pixel[0] = (int)(255.0 * col[0]);
  pixel[1] = (int)(255.0 * col[1]);
  pixel[2] = (int)(255.0 * col[2]);
  return col;
}

#define VERBOSE 0

// Do recursive ray tracing! You'll want to insert a lot of code here (or places
// called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray &r, const glm::dvec3 &thresh, int depth,
                               double &t) {
    isect i;
    glm::dvec3 colorC(0.0);

#if VERBOSE
    std::cerr << "== current depth: " << depth << std::endl;
#endif

    if (scene->intersect(r, i)) {
        const Material &m = i.getMaterial();

        // ---- Local Phong shading ----
        colorC = m.shade(scene.get(), r, i);

        // Stop recursion
        if (depth <= 0) {
            return colorC;
        }

        glm::dvec3 P = r.at(i.getT());
        glm::dvec3 N = glm::normalize(i.getN());
        glm::dvec3 D = glm::normalize(r.getDirection());

        const double eps = 1e-6;

        // ==============================
        // REFLECTION
        // ==============================
        glm::dvec3 kr = m.kr(i);
        if (kr != glm::dvec3(0.0)) {
            glm::dvec3 R = glm::reflect(D, N);

            ray reflectedRay(
                P + eps * R,
                R,
                glm::dvec3(1.0),
                ray::REFLECTION
            );

            double t_reflect;
            glm::dvec3 reflectedColor =
                traceRay(reflectedRay, thresh * kr, depth - 1, t_reflect);

            colorC += kr * reflectedColor;
        }

        // ==============================
        // REFRACTION
        // ==============================
        glm::dvec3 kt = m.kt(i);
        double ior = m.index(i);

        if (kt != glm::dvec3(0.0)) {
            double cosi = glm::dot(D, N);
            double etai = 1.0;   // air
            double etat = ior;
            glm::dvec3 n = N;

            // Check if ray is inside object
            if (cosi > 0.0) {
                std::swap(etai, etat);
                n = -N;
            } else {
                cosi = -cosi;
            }

            double eta = etai / etat;
            double k = 1.0 - eta * eta * (1.0 - cosi * cosi);

            // No total internal reflection
            if (k >= 0.0) {
                glm::dvec3 T =
                    eta * D + (eta * cosi - sqrt(k)) * n;

                ray refractedRay(
                    P + eps * T,
                    T,
                    glm::dvec3(1.0),
                    ray::REFRACTION
                );

                double t_refract;
                glm::dvec3 refractedColor =
                    traceRay(refractedRay, thresh * kt, depth - 1, t_refract);

                colorC += kt * refractedColor;
            }
        }

#if VERBOSE
        std::cerr << "== depth: " << depth + 1 << " done, returning: "
                  << colorC << std::endl;
#endif
        return colorC;
    } else {
        // ==================================================
        // No intersection: ray goes to infinity
        // ==================================================
        // FIXME: Add CubeMap support here.
        // TIPS:
        //   - CubeMap object can be fetched from:
        //       traceUI->getCubeMap()
        //   - Check traceUI->cubeMap() to see if it is enabled

        
    // DEBUG BACKGROUND (sky gradient)
    glm::dvec3 D = glm::normalize(r.getDirection());
    double t = 0.5 * (D.y + 1.0);

    // Blue → white gradient
    colorC = (1.0 - t) * glm::dvec3(1.0, 1.0, 1.0)
           + t * glm::dvec3(0.4, 0.7, 1.0);

    }
    return colorC;
}


RayTracer::RayTracer()
    : scene(nullptr), buffer(0), thresh(0), buffer_width(0), buffer_height(0),
      m_bBufferReady(false) {
}

RayTracer::~RayTracer() {}

void RayTracer::getBuffer(unsigned char *&buf, int &w, int &h) {
  buf = buffer.data();
  w = buffer_width;
  h = buffer_height;
}

double RayTracer::aspectRatio() {
  return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char *fn) {
  ifstream ifs(fn);
  if (!ifs) {
    string msg("Error: couldn't read scene file ");
    msg.append(fn);
    traceUI->alert(msg);
    return false;
  }

  // Check if fn ends in '.ray'
  bool isRay = false;
  const char *ext = strrchr(fn, '.');
  if (ext && !strcmp(ext, ".ray"))
    isRay = true;

  // Strip off filename, leaving only the path:
  string path(fn);
  if (path.find_last_of("\\/") == string::npos)
    path = ".";
  else
    path = path.substr(0, path.find_last_of("\\/"));

  if (isRay) {
    // .ray Parsing Path
    // Call this with 'true' for debug output from the tokenizer
    Tokenizer tokenizer(ifs, false);
    Parser parser(tokenizer, path);
    try {
      scene.reset(parser.parseScene());
    } catch (SyntaxErrorException &pe) {
      traceUI->alert(pe.formattedMessage());
      return false;
    } catch (ParserException &pe) {
      string msg("Parser: fatal exception ");
      msg.append(pe.message());
      traceUI->alert(msg);
      return false;
    } catch (TextureMapException e) {
      string msg("Texture mapping exception: ");
      msg.append(e.message());
      traceUI->alert(msg);
      return false;
    }
  } else {
    // JSON Parsing Path
    try {
      JsonParser parser(path, ifs);
      scene.reset(parser.parseScene());
    } catch (ParserException &pe) {
      string msg("Parser: fatal exception ");
      msg.append(pe.message());
      traceUI->alert(msg);
      return false;
    } catch (const json::exception &je) {
      string msg("Invalid JSON encountered ");
      msg.append(je.what());
      traceUI->alert(msg);
      return false;
    }
  }

  if (!sceneLoaded())
    return false;

  return true;
}

void RayTracer::traceSetup(int w, int h) {
  size_t newBufferSize = w * h * 3;
  if (newBufferSize != buffer.size()) {
    bufferSize = newBufferSize;
    buffer.resize(bufferSize);
  }
  buffer_width = w;
  buffer_height = h;
  std::fill(buffer.begin(), buffer.end(), 0);
  m_bBufferReady = true;

  /*
   * Sync with TraceUI
   */

  threads = traceUI->getThreads();
  block_size = traceUI->getBlockSize();
  thresh = traceUI->getThreshold();
  samples = traceUI->getSuperSamples();
  aaThresh = traceUI->getAaThreshold();

  // YOUR CODE HERE
  // FIXME: Additional initializations
}

/*
 * RayTracer::traceImage
 *
 *	Trace the image and store the pixel data in RayTracer::buffer.
 *
 *	Arguments:
 *		w:	width of the image buffer
 *		h:	height of the image buffer
 *
 */
void RayTracer::traceImage(int w, int h) {
  // Setup buffer and parameters
  traceSetup(w, h);

  // Simple single-threaded rendering
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      tracePixel(i, j);
    }
  }
  if (samples > 0) {
    aaImage();
}
}


int RayTracer::aaImage() {
    // Check if the required parameters are initialized
    if (samples <= 0) {
        return 0; // No samples means no anti-aliasing
    }

    // Loop through each pixel
    for (int j = 0; j < buffer_height; ++j) {
        for (int i = 0; i < buffer_width; ++i) {
            glm::dvec3 color(0.0); // Initialize cumulative color for the pixel

            // Loop for each sample within the pixel
            for (int s = 0; s < samples; ++s) {
                // Generate a random offset for sub-pixel sampling
                double xOffset = ((double)rand() / RAND_MAX - 0.5) * 1.0;
                double yOffset = ((double)rand() / RAND_MAX - 0.5) * 1.0;


                // Compute the color for the ray with the offset
                glm::dvec3 sampleColor = trace((i + xOffset) / buffer_width, (j + yOffset) / buffer_height);
                color += sampleColor; // Accumulate sample color
            }

            // Average the color by the number of samples
            color /= static_cast<double>(samples);

            // Store the averaged color into the pixel buffer
            setPixel(i, j, color);
        }
    }

    return 1; // Indicate that anti-aliasing was performed
}

bool RayTracer::checkRender() {

  return true;
}

void RayTracer::waitRender() {

  return;
}


glm::dvec3 RayTracer::getPixel(int i, int j) {
  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
  return glm::dvec3((double)pixel[0] / 255.0, (double)pixel[1] / 255.0,
                    (double)pixel[2] / 255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color) {
  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;

  pixel[0] = (int)(255.0 * color[0]);
  pixel[1] = (int)(255.0 * color[1]);
  pixel[2] = (int)(255.0 * color[2]);
}
