cmake_minimum_required(VERSION 3.8)

project(SlimRayTracer_demo_scene)
add_executable(SlimRayTracer_demo_scene WIN32 src/examples/demo_scene.c)


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