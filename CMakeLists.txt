cmake_minimum_required(VERSION 3.8)

project(obj2mesh)
add_executable(obj2mesh src/obj2mesh.c)

project(bmp2texture)
add_executable(bmp2texture src/bmp2texture.c)

project(01_Lights)
add_executable(01_Lights WIN32 src/examples/01_Lights.c)

project(02_Geometry)
add_executable(02_Geometry WIN32 src/examples/02_Geometry.c)

project(03_BlinnPhong)
add_executable(03_BlinnPhong WIN32 src/examples/03_BlinnPhong.c)

project(04_GlassMirror)
add_executable(04_GlassMirror WIN32 src/examples/04_GlassMirror.c)

project(05_PBR)
add_executable(05_PBR WIN32 src/examples/05_PBR.c)

project(06_AreaLights)
add_executable(06_AreaLights WIN32 src/examples/06_AreaLights.c)

project(07_Meshes)
add_executable(07_Meshes WIN32 src/examples/07_Meshes.c)

project(08_Modes)
add_executable(08_Modes WIN32 src/examples/08_Modes.c)

project(09_Textures)
add_executable(09_Textures WIN32 src/examples/09_Textures.c)


# NOTE: Uncomment below if you have an NVIDIA GPU and have CUDA installed:

set(CMAKE_CUDA_STANDARD 11)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 52)
endif()
set(CUDA_VERBOSE_BUILD ON)


project(01_Lights_XPU CUDA)
add_executable(01_Lights_XPU WIN32 src/examples/01_Lights.cu)
set_target_properties(01_Lights_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

project(02_Geometry_XPU CUDA)
add_executable(02_Geometry_XPU WIN32 src/examples/02_Geometry.cu)
set_target_properties(02_Geometry_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

project(03_BlinnPhong_XPU CUDA)
add_executable(03_BlinnPhong_XPU WIN32 src/examples/03_BlinnPhong.cu)
set_target_properties(03_BlinnPhong_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

project(04_GlassMirror_XPU CUDA)
add_executable(04_GlassMirror_XPU WIN32 src/examples/04_GlassMirror.cu)
set_target_properties(04_GlassMirror_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

project(05_PBR_XPU CUDA)
add_executable(05_PBR_XPU WIN32 src/examples/05_PBR.cu)
set_target_properties(05_PBR_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

project(06_AreaLights_XPU CUDA)
add_executable(06_AreaLights_XPU WIN32 src/examples/06_AreaLights.cu)
set_target_properties(06_AreaLights_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

project(07_Meshes_XPU CUDA)
add_executable(07_Meshes_XPU WIN32 src/examples/07_Meshes.cu)
set_target_properties(07_Meshes_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

project(08_Modes_XPU CUDA)
add_executable(08_Modes_XPU WIN32 src/examples/08_Modes.cu)
set_target_properties(08_Modes_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

project(09_Textures_XPU CUDA)
add_executable(09_Textures_XPU WIN32 src/examples/09_Textures.cu)
set_target_properties(09_Textures_XPU PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

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