# SlimRayTracer

A minimalist and platform-agnostic interactive/real-time raytracer.<br>
Strong emphasis on simplicity, ease of use and almost no setup to get started with.<br>  
Written in plain C and can be complied in either C or C++.<br>

This project extends [SlimEngine](https://github.com/HardCoreCodin/SlimEngine).

Optional GPU support using CUDA: The same code is cross-compiled (no CUDA libraries used).<br>

Architecture:
-
The platform layer only flags operating-system headers (no standard library used).<br>
The application layer itself (non-CUDA) has no dependencies, apart from the standard math header.<br>
It is just a library that the platform layer flags - it has no knowledge of the platform.<br>

More details on this architecture [here](https://youtu.be/Ev_TeQmus68).

Features:
-
All features of SlimEngine are available here as well.<br>
Additional features include raytracing facilities:<br>

* Lights: Point/Directional and emissive quads.<br>
  <img src="src/examples/1_lights.gif" alt="1_lights" height="360"><br>
  Point lights can be moved around and scaled (changing their light intensity)<br>
  <p float="left">
    <img src="src/examples/1_lights_scene_setup_c.png" alt="1_lights_scene_setup_code" width="360">
    <img src="src/examples/1_lights_update_and_render_c.png" alt="1_lights_update_and_render_code" height="300">
  </p>
* Materials with reflection/refraction or classical shaders.<br>
* Parametric 3D shapes (Sphere, Box, Tetrahedra).<br>
* Meshes with vertex normal interpolation.<br>
* Spatial acceleration structures for scene and meshes.<br>
* Screen-space acceleration structure for primary rays.<br>

