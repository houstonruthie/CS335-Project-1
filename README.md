README — Project 1: Ray Tracer
Overview

This project implements a recursive ray tracer using the Whitted illumination model. The ray tracer supports:
Phong shading (ambient, diffuse, specular)
Shadows from point and directional lights
Recursive reflections and refraction
Triangle–ray intersection (Möller–Trumbore)
Phong normal interpolation
Texture mapping with UV coordinates
Cube mapping (user-selectable)
GUI and command-line execution

For full implementation details, see the project report.

Compilation Instructions:

cd raytracer
  This should include ray, scenes, and cubemaps folders
cd ray
mkdir build
cd build
cmake ..
make -j8
Tested on macOS using CMake and the provided dependencies.

Running the Program:
Preferred:
GUI Mode
cd build
./ray

Use:
File → Load Scene to load a .ray or .json file
Adjust recursion depth and other settings in the control panel
Click Render

Command-Line Mode
./ray -r <depth> input.ray output.png

Examples:
./ray -r 3 ../scenes/ray_scenes/reflection.ray output.png

./ray -r 5 \ -c ../cubemap/cubemap_autumn/autumn_negative_x.png \
../scenes/ray_scenes/scene_blank.ray \
sky.png

Notes for Grading
No extraneous edits to program execution, build configuration, or runtime behavior have been made beyond the required implementation changes specified in the project instructions.
For rendered output images and visual demonstrations of functionality, please refer to the project report.
