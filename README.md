<img src="SlimTracin_logo.png" alt="SlimTracin_logo"><br>

<img src="src/examples/GPU.gif" alt="GPU" height="450"><br>

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

* <b><u>Point lights</b>:</u> Can be moved around and scaled (changing their light intensity)<br>
  <img src="src/examples/01_Lights.gif" alt="01_Lights" height="360"><br>
  <p float="left">
    <img src="src/examples/01_Lights_setup.png" alt="01_Lights_setup" width="360">
    <img src="src/examples/01_Lights_update.png" alt="01_Lights_update" height="300">
  </p>
* <b><u>Shapes</b>:</u> Spheres, Boxes, Tetrahedra with UV-based transparency<br>
  <img src="src/examples/02_Geometry.gif" alt="02_Geometry" height="360"><br>
  <p float="left">
    <img src="src/examples/02_Geometry_setup.png" alt="2_shapes_scene_setup_code" width="360">
    <img src="src/examples/02_Geometry_update.png" alt="2_shapes_update_and_render_code" height="370">
    <img src="src/examples/02_Geometry_transparency.png" alt="2_shapes_transparency_code" height="360">
  </p>
* <b><u>Materials</b>:</u> Lambert, Blinn, Phong with controllable metal<br>
  <img src="src/examples/03_BlinnPhong.gif" alt="03_BlinnPhong" height="360"><br>
  <p float="left">
    <img src="src/examples/03_BlinnPhong_setup.png" alt="3_materials_classic_setup_code" height="450">
    <img src="src/examples/03_BlinnPhong_selection.png" alt="3_materials_classic_update_selection_code" height="450">
  </p>
* <b><u>Materials</b>:</u> Reflective or Refractive with controllable trace-height<br>
  <img src="src/examples/04_GlassMirror.gif" alt="4_materials_reflective" height="360"><br>
  <p float="left">   
    <img src="src/examples/04_GlassMirror_setup.png" alt="4_materials_ref_setup_code" height="360">
    <img src="src/examples/04_GlassMirror_selection.png" alt="4_materials_ref_update_selection_code" height="300">
  </p>
* <b><u>Materials</b>:</u> Physically Based (Cook-Torrance BRDF)<br>
  <img src="src/examples/05_PBR.gif" alt="05_PBR" height="360"><br>
  <img src="src/examples/05_PBR.png" alt="05_PBR" height="450">  
* <b><u>Materials</b>:</u> Emissive quads can be used as rectangular area lights<br>
  <img src="src/examples/06_AreaLights.gif" alt="06_AreaLights" height="360"><br>
  <p float="left">   
    <img src="src/examples/06_AreaLights_setup.png" alt="06_AreaLights_setup" height="360">
    <img src="src/examples/06_AreaLights_selection.png" alt="06_AreaLights_selection" height="300">
  </p>  
* <b><u>Meshes</b>:</u> Transformable and can have smooth shading using vertex normal interpolation<br>
  <img src="src/examples/07_Meshes.gif" alt="07_Meshes" height="360"><br>
  <p float="left">
    <img src="src/examples/07_Meshes_setup.png" alt="07_Meshes_setup" height="450">
    <img src="src/examples/07_Meshes_update.png" alt="07_Meshes_update" height="450">
  </p>
* <b><u>Render Modes</b>:</u> Beauty, Depth, Normals, UVs and acceleration structures<br>
  <img src="src/examples/08_Modes.gif" alt="08_Modes" height="360"><br>
  Acceleration structures can be overlayed showing how they update dynamically.<br>
  The scene's BVH is re-built any time a primitive is moved or transformed.<br>
  The screen space bounds (SSB) also update when the camera changes.<br>
  <img src="src/examples/08_Modes_BVH.gif" alt="08_Modes_BVH" height="360"><br>
  <p float="left">
    <img src="src/examples/08_Modes_setup.png" alt="08_Modes_setup" height="400">
    <img src="src/examples/08_Modes_update.png" alt="08_Modes_update" height="400">
  </p>
* <b><u>Textures</b>:</u> Albedo, Normal<br>
  <img src="src/examples/09_Textures.gif" alt="09_Textures" height="360"><br>
  Textures can be loaded from files, for use as albedo and normal maps.<br>
  <img src="src/examples/09_Textures.png" alt="09_Textures" height="400"><br>
* <b><u>XPU</b>:</u> CPU vs. GPU (CUDA)<br>
  <img src="src/examples/XPU.gif" alt="XPU" height="360"><br>
  Compiling using CUDA allows for toggling between rendering on the CPU or the GPU.<br>
  <img src="src/examples/XPU.png" alt="XPU" height="400">
* <b><u>obj2mesh</b>:</u> Also privided is a separate CLI tool for converting `.obj` files to `.mesh` files.<br>
  It is also written in plain C so can be compiled in either C or C++.<br>
  Usage is simple: `./obj2mesh my_obj_file.obj my_mesh_file.mesh`<br>
  <u><i>Note</i>:</u> A tool by the same name and with the same usage exists in <b>SlimEngine</b>.<br>
  They are not at all the same and produce different and incompatible `.mesh` files.<br>
  <b>SlimTracin</b> meshes contain additional data not present in <b>SlimEngine</b> meshes.<br>
  The additional data relates to accelerating the process of tracing rays against the meshes.<br>
  It includes a BVH and per-triangle data used for optimized ray/triangle intersection checks.<br>
  It gets computed as part of the conversion process and stored in the resulting `.mesh` files.<br>
  That is why it takes longer to convert for <b>SlimTracin</b> and why the resulting files are larger.<br>
  

<b>SlimTracin</b> does not come with any GUI functionality at this point.<br>
Some example apps have an optional HUD (heads up display) that shows additional information.<br>
It can be toggled on or off using the`tab` key.<br>

All examples are interactive using <b>SlimEngine</b>'s facilities having 2 interaction modes:
1. FPS navigation (WASD + mouse look + zooming)<br>
2. DCC application (default)<br>

Double clicking the `left mouse button` anywhere within the window toggles between these 2 modes.<btr>

Entering FPS mode captures the mouse movement for the window and hides the cursor.<br>
Navigation is then as in a typical first-person game (plus lateral movement and zooming):<br>

Move the `mouse` to freely look around (even if the cursor would leave the window border)<br>
Scroll the `mouse wheel` to zoom in and out (changes the field of view of the perspective)<br>
Hold `W` to move forward<br>
Hold `S` to move backward<br>
Hold `A` to move left<br>
Hold `D` to move right<br>
Hold `R` to move up<br>
Hold `F` to move down<br>

Exit this mode by double clicking the `left mouse button`.

The default interaction mode is similar to a typical DCC application (i.e: Maya):<br>
The mouse is not captured to the window and the cursor is visible.<br>
Holding the `right mouse button` and dragging the mouse orbits the camera around a target.<br>
Holding the `middle mouse button` and dragging the mouse pans the camera (left, right, up and down).<br>
Scrolling the `mouse wheel` dollys the camera forward and backward.<br>

Clicking the `left mouse button` selects an object in the scene that is under the cursor.<br>
Holding the `left mouse button` while hovering an object and then dragging the mouse,<br>
moves the object parallel to the screen.<br>

Holding `alt` highlights the currently selecte object by drawing a bounding box around it.<br>
While `alt` is still held, if the cursor hovers the selected object's bounding box,<br>
mouse interaction transforms the object along the plane of the bounding box that the cursor hovers on:<br>
Holding the `left mouse button` and dragging the mouse moves the object.<br>
Holding the `right mouse button` and dragging the mouse rotates the object.<br>
Holding the `middle mouse button` and dragging the mouse scales the object.<br>
<i>(`mouse wheel` interaction is disabled while `alt` is held)</i><br>

In some examples, further interaction is enabled while holding `ctrl` or `shift` <br>
using the `mouse wheel` as a virtual "slider":<br>
Holding `shift` and scrolling the `mouse wheel` cycles the assigned material for the selected object.<br>
Holding `ctrl` and scrolling the `mouse wheel` increases or decreases the trace-height*<br>
<i>(how many times rays are allowed to bounce around between reflective or refractive objects)</i><br>