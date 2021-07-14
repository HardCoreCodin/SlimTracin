# SlimRayTracer

A minimalist and platform-agnostic interactive/real-time raytracer.<br>
Strong emphasis on simplicity, ease of use and almost no setup to get started with.<br>  
Written in plain C and can be complied in either C or C++.<br>

This project extends [SlimEngine](https://github.com/HardCoreCodin/SlimEngine).

Optional GPU support using CUDA: The same code is cross-compiled (no CUDA libraries used).<br>

Architecture:
-
The platform layer only uses operating-system headers (no standard library used).<br>
The application layer itself (non-CUDA) has no dependencies, apart from the standard math header.<br>
It is just a library that the platform layer uses - it has no knowledge of the platform.<br>

More details on this architecture [here](https://youtu.be/Ev_TeQmus68).

Features:
-
All features of SlimEngine are available here as well.<br>
Additional features include raytracing facilities:<br>

* Point lights: Can be moved around and scaled (changing their light intensity)<br>
  <img src="src/examples/1_lights.gif" alt="1_lights" height="360"><br>
  <p float="left">
    <img src="src/examples/1_lights_scene_setup_c.png" alt="1_lights_scene_setup_code" width="360">
    <img src="src/examples/1_lights_update_and_render_c.png" alt="1_lights_update_and_render_code" height="300">
  </p>
* Shapes: Spheres, Boxes, Tetrahedra with UV-based transparency<br>
  <img src="src/examples/2_shapes.gif" alt="2_shapes" height="360"><br>
  <p float="left">
    <img src="src/examples/2_shapes_scene_setup_c.png" alt="2_shapes_scene_setup_code" width="360">
    <img src="src/examples/2_shapes_update_and_render_c.png" alt="2_shapes_update_and_render_code" height="370">
  </p>
  <p float="left">
    <img src="src/examples/2_shapes_uv_transparency.gif" alt="2_shapes_uve_transparency" height="360">
    <img src="src/examples/2_shapes_transparency_c.png" alt="2_shapes_transparency_code" height="360">
  </p>
* Materials: Lambert, Blinn, Phong with controllable shininess<br>
  <img src="src/examples/3_materials_classic_shaders.gif" alt="3_materials_classic_shaders" height="360"><br>
  <p float="left">
    <img src="src/examples/3_materials_classic_shaders_setup_c.png" alt="3_materials_classic_setup_code" height="450">
    <img src="src/examples/3_materials_classic_update_selection_c.png" alt="3_materials_classic_update_selection_code" height="450">
  </p>
* Materials: Reflective or Refractive with controllable trace-depth<br>
  <p float="left">  
    <img src="src/examples/4_materials_reflective.gif" alt="4_materials_reflective" height="360"><br>
    <img src="src/examples/4_materials_refractive.gif" alt="4_materials_refractive" height="360"><br>
  </p>
  <p float="left">   
    <img src="src/examples/4_materials_ref_setup_c.png" alt="4_materials_ref_setup_code" height="450">
    <img src="src/examples/4_materials_ref_update_selection_c.png" alt="4_materials_ref_update_selection_code" height="450">
  </p>
* Materials: Emissive quads can be used as rectangular area lights<br>
  <img src="src/examples/5_materials_emissive.gif" alt="5_materials_emissive" height="360"><br>
  <p float="left">   
    <img src="src/examples/5_materials_emissive_setup_c.png" alt="4_materials_ref_setup_code" height="450">
    <img src="src/examples/5_materials_emissive_update_selection_c.png" alt="4_materials_ref_update_selection_code" height="450">
  </p>  
* Meshes: Transformable and can have smooth shading using vertex normal interpolation<br>
  <img src="src/examples/6_meshes.gif" alt="6_meshes" height="360"><br>
  <p float="left">
    <img src="src/examples/6_meshes_scene_setup_c.png" alt="6_meshes_scene_setup_code" height="450">
    <img src="src/examples/6_meshes_update_and_render_c.png" alt="6_meshes_update_and_render_code" height="450">
  </p>
* Render Modes: Beauty, Depth, Normals, UVs and acceleration structures<br>
  <img src="src/examples/7_modes.gif" alt="7_modes" height="360"><br>
  Acceleration structures can be overlayed showing how they update dynamically.<br>
  The scene's BVH is re-built any time a primitive is moved or transformed.<br>
  The screen space bounds (SSB) also update when the camera changes.<br>
  <img src="src/examples/7_modes_as.gif" alt="7_modes_as" height="360"><br>
  <p float="left">
    <img src="src/examples/7_modes_setup_c.png" alt="7_modes_setup_code" height="400">
    <img src="src/examples/7_modes_update_c.png" alt="7_modes_update_code" height="400">
  </p>

