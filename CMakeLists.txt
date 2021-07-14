cmake_minimum_required(VERSION 3.8)

project(1_lights)
add_executable(lights WIN32 src/examples/1_lights.c)
set_target_properties(lights PROPERTIES C_STANDARD 90)

project(2_shapes)
add_executable(2_shapes WIN32 src/examples/2_shapes.c)
set_target_properties(2_shapes PROPERTIES C_STANDARD 90)

project(3_materials_classic)
add_executable(3_materials_classic WIN32 src/examples/3_materials_classic_shaders.c)
set_target_properties(3_materials_classic PROPERTIES C_STANDARD 90)

project(4_materials_ref)
add_executable(4_materials_ref WIN32 src/examples/4_materials_reflective_refractive.c)
set_target_properties(4_materials_ref PROPERTIES C_STANDARD 90)

project(5_materials_emissive)
add_executable(5_materials_emissive WIN32 src/examples/5_materials_emissive.c)
set_target_properties(5_materials_emissive PROPERTIES C_STANDARD 90)

project(6_mesh)
add_executable(6_meshes WIN32 src/examples/6_meshes.c)
set_target_properties(6_meshes PROPERTIES C_STANDARD 90)

project(7_modes)
add_executable(7_modes WIN32 src/examples/7_modes.c)
set_target_properties(7_modes PROPERTIES C_STANDARD 90)

#project(SlimRayTracer_3_area_lights)
#add_executable(SlimRayTracer_3_area_lights WIN32 src/examples/3_area_lights.c)
#set_target_properties(SlimRayTracer_3_area_lights PROPERTIES C_STANDARD 90)


set(CMAKE_CUDA_STANDARD 11)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 52)
endif()
#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CUDA_VERBOSE_BUILD ON)

#project(SlimRayTracer_1_shapes_XPU CUDA)
#add_executable(SlimRayTracer_1_shapes_XPU WIN32 src/examples/1_shapes.cu)
#set_target_properties(SlimRayTracer_1_shapes_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON C_STANDARD 90)
#
#project(SlimRayTracer_2_mesh_XPU CUDA)
#add_executable(SlimRayTracer_2_meshes_XPU WIN32 src/examples/2_meshes.cu)
#set_target_properties(SlimRayTracer_2_meshes_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON C_STANDARD 90)
#
#project(SlimRayTracer_3_area_lights_XPU CUDA)
#add_executable(SlimRayTracer_3_area_lights_XPU WIN32 src/examples/3_area_lights.cu)
#set_target_properties(SlimRayTracer_3_area_lights_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON C_STANDARD 90)

project(obj2mesh)
add_executable(obj2mesh src/obj2mesh.c)

add_compile_options(
        $<$<CONFIG:RELEASE>:-O3>
        $<$<CONFIG:RELEASE>:-Oi>
        $<$<CONFIG:RELEASE>:-Mt>
        $<$<CONFIG:DEBUG>:-JMC>
        $<$<CONFIG:DEBUG>:-MTd>
        $<$<CONFIG:DEBUG>:-Z7>
        -WX-
        -W4
        -wd4100
        -GR-
        -EHa-
        -GL
        -GS
        -FC
        -permissive-
        -sdl
        -nologo
        -analyze-
        -fp:fast
        -Zc:inline
        -diagnostics:caret
        -errorReport:none
)