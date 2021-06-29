cmake_minimum_required(VERSION 3.8)

project(SlimRayTracer_demo_scene)
add_executable(SlimRayTracer_demo_scene WIN32 src/examples/demo_scene.c)


project(obj2mesh)
add_executable(obj2mesh src/obj2mesh.c)