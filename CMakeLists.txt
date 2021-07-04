cmake_minimum_required(VERSION 3.8)

project(SlimRayTracer_demo_scene)
add_executable(SlimRayTracer_demo_scene WIN32 src/examples/demo_scene.c)
set_target_properties(SlimRayTracer_demo_scene PROPERTIES C_STANDARD 90)

set(CMAKE_CUDA_STANDARD 11)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 52)
endif()
#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CUDA_VERBOSE_BUILD ON)

project(SlimRayTracer_demo_sceneXPU CUDA)
add_executable(SlimRayTracer_demo_sceneXPU WIN32 src/examples/demo_scene.cu)
set_target_properties(SlimRayTracer_demo_sceneXPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON C_STANDARD 90)

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