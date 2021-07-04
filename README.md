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
* Classical shaders (Lambert, Phong, Blinn) and reflection shader.
* Parametric 3D shapes (Sphere, Box, Tetrahedra).
* Full mesh support with interpolated vertex normals.
* Spatial acceleration structures for scene and meshes (construction and traversal).<br>
* Screen-space acceleration structure for primary rays.<br>