#pragma warning(disable : 4201)

#ifndef NDEBUG
    #include <stdio.h>
    #include <stdlib.h>
    #define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
    inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true) {
        if (code != cudaSuccess) {
            fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
            if (abort) exit(code);
        }
    }
    #define INLINE __device__ __host__
#else
    #define INLINE __device__ __host__ __forceinline__
    #define gpuErrchk(ans) (ans);
#endif

#define checkErrors() gpuErrchk(cudaPeekAtLastError())
#define uploadNto(cpu_ptr, gpu_ptr, N, offset) gpuErrchk(cudaMemcpy(&((gpu_ptr)[(offset)]), (cpu_ptr), sizeof((cpu_ptr)[0]) * (N), cudaMemcpyHostToDevice))
#define uploadN(  cpu_ptr, gpu_ptr, N        ) gpuErrchk(cudaMemcpy(&((gpu_ptr)[0])       , (cpu_ptr), sizeof((cpu_ptr)[0]) * (N), cudaMemcpyHostToDevice))
#define downloadN(gpu_ptr, cpu_ptr, N)         gpuErrchk(cudaMemcpyFromSymbol(cpu_ptr     , (gpu_ptr), sizeof((cpu_ptr)[0]) * (N), 0, cudaMemcpyDeviceToHost))

#define USE_GPU_DEFAULT true

#include "./demo_scene.c"

__device__   u32 d_pixels[MAX_WIDTH * MAX_HEIGHT];

PointLight *d_point_lights;
QuadLight  *d_quad_lights;
Material   *d_materials;
Primitive  *d_primitives;
Mesh       *d_meshes;
Triangle   *d_triangles;
u32        *d_scene_bvh_leaf_ids;
BVHNode    *d_scene_bvh_nodes;
BVHNode    *d_mesh_bvh_nodes;

u32 *d_mesh_bvh_node_counts,
    *d_mesh_triangle_counts;

__global__ void d_render(ProjectionPlane projection_plane, enum RenderMode mode, vec3 camera_position, Trace trace,
                         u16 width,
                         u32 pixel_count,

                         Scene scene,
                         u32        *scene_bvh_leaf_ids,
                         BVHNode    *scene_bvh_nodes,
                         BVHNode    *mesh_bvh_nodes,
                         Mesh       *meshes,
                         Triangle   *mesh_triangles,
                         PointLight *point_lights,
                         QuadLight  *quad_lights,
                         Material   *materials,
                         Primitive  *primitives,

                         const u32 *mesh_bvh_node_counts,
                         const u32 *mesh_triangle_counts
) {
    u32 i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i >= pixel_count)
        return;

    Pixel *pixel = (Pixel *)&d_pixels[i];

    u16 x = i % width;
    u16 y = i / width;

    Ray ray;
    ray.origin = camera_position;
    ray.direction = normVec3(scaleAddVec3(projection_plane.down, y, scaleAddVec3(projection_plane.right, x, projection_plane.start)));

    scene.point_lights = point_lights;
    scene.quad_lights  = quad_lights;
    scene.materials    = materials;
    scene.primitives   = primitives;
    scene.meshes       = meshes;
    scene.bvh.nodes    = scene_bvh_nodes;
    scene.bvh.leaf_ids = scene_bvh_leaf_ids;

    u32 scene_stack[6], mesh_stack[5];
    trace.mesh_stack  = mesh_stack;
    trace.scene_stack = scene_stack;

    Mesh *mesh = meshes;
    u32 nodes_offset = 0;
    u32 triangles_offset = 0;
    for (u32 m = 0; m < scene.settings.meshes; m++, mesh++) {
        mesh->bvh.node_count = mesh_bvh_node_counts[m];
        mesh->triangle_count = mesh_triangle_counts[m];
        mesh->normals_count  = mesh_triangle_counts[m];
        mesh->triangles      = mesh_triangles + triangles_offset;
        mesh->bvh.nodes      = mesh_bvh_nodes + nodes_offset;

        nodes_offset        += mesh->bvh.node_count;
        triangles_offset    += mesh->triangle_count;
    }

    ray.direction_reciprocal = oneOverVec3(ray.direction);
    trace.closest_hit.distance = trace.closest_hit.distance_squared = INFINITY;

    rayTrace(&ray, &trace, &scene, mode, x, y, pixel);
}

void renderOnGPU(Scene *scene, Viewport *viewport) {
    setViewportProjectionPlane(viewport);

    Dimensions *dim = &viewport->frame_buffer->dimensions;
    u32 pixel_count = dim->width_times_height;
    u16 threads = 256;
    u16 blocks  = pixel_count / threads;
    if (pixel_count < threads) {
        threads = pixel_count;
        blocks = 1;
    } else if (pixel_count % threads)
        blocks++;

    d_render<<<blocks, threads>>>(
            viewport->projection_plane, viewport->settings.render_mode, viewport->camera->transform.position, viewport->trace,

            dim->width,
            pixel_count,

            *scene,
            d_scene_bvh_leaf_ids,
            d_scene_bvh_nodes,
            d_mesh_bvh_nodes,
            d_meshes,
            d_triangles,
            d_point_lights,
            d_quad_lights,
            d_materials,
            d_primitives,

            d_mesh_bvh_node_counts,
            d_mesh_triangle_counts);

    checkErrors()
    downloadN(d_pixels, (u32*)viewport->frame_buffer->pixels, dim->width_times_height)
}

void allocateDeviceScene(Scene *scene) {
    u32 total_triangles = 0;
    if (scene->settings.point_lights) gpuErrchk(cudaMalloc(&d_point_lights, sizeof(PointLight) * scene->settings.point_lights))
    if (scene->settings.quad_lights)  gpuErrchk(cudaMalloc(&d_quad_lights,  sizeof(QuadLight)  * scene->settings.quad_lights))
    if (scene->settings.primitives)   gpuErrchk(cudaMalloc(&d_primitives,   sizeof(Primitive)  * scene->settings.primitives))
    if (scene->settings.meshes) {
        for (u32 i = 0; i < scene->settings.meshes; i++)
            total_triangles += scene->meshes[i].triangle_count;

        gpuErrchk(cudaMalloc(&d_meshes,    sizeof(Mesh)     * scene->settings.meshes))
        gpuErrchk(cudaMalloc(&d_triangles, sizeof(Triangle) * total_triangles))

        gpuErrchk(cudaMalloc(&d_mesh_bvh_node_counts, sizeof(u32) * scene->settings.meshes))
        gpuErrchk(cudaMalloc(&d_mesh_triangle_counts, sizeof(u32) * scene->settings.meshes))
    }

    gpuErrchk(cudaMalloc(&d_materials,          sizeof(Material) * scene->settings.materials))
    gpuErrchk(cudaMalloc(&d_scene_bvh_leaf_ids, sizeof(u32)      * scene->settings.primitives))
    gpuErrchk(cudaMalloc(&d_scene_bvh_nodes,    sizeof(BVHNode)  * scene->settings.primitives * 2))
    gpuErrchk(cudaMalloc(&d_mesh_bvh_nodes,     sizeof(BVHNode)  * total_triangles * 2))
}

void uploadPrimitives(Scene *scene) {
    uploadN(scene->primitives, d_primitives, scene->settings.primitives)
}

void uploadLights(Scene *scene) {
    if (scene->settings.point_lights) uploadN( scene->point_lights, d_point_lights, scene->settings.point_lights)
    if (scene->settings.quad_lights)  uploadN( scene->quad_lights,  d_quad_lights,  scene->settings.quad_lights)
}

void uploadScene(Scene *scene) {
    uploadLights(scene);
    uploadPrimitives(scene);
    uploadN( scene->materials, d_materials,scene->settings.materials)
}

void uploadSceneBVH(Scene *scene) {
    uploadN(scene->bvh.nodes,    d_scene_bvh_nodes,   scene->bvh.node_count)
    uploadN(scene->bvh.leaf_ids, d_scene_bvh_leaf_ids,scene->settings.primitives)
}

void uploadMeshBVHs(Scene *scene) {
    Mesh *mesh = scene->meshes;
    u32 nodes_offset = 0;
    u32 triangles_offset = 0;
    for (u32 i = 0; i < scene->settings.meshes; i++, mesh++) {
        uploadNto(mesh->bvh.nodes, d_mesh_bvh_nodes, mesh->bvh.node_count, nodes_offset)
        uploadNto(mesh->triangles, d_triangles,      mesh->triangle_count, triangles_offset)
        nodes_offset        += mesh->bvh.node_count;
        triangles_offset    += mesh->triangle_count;
    }

    uploadN(scene->mesh_bvh_node_counts, d_mesh_bvh_node_counts, scene->settings.meshes)
    uploadN(scene->mesh_triangle_counts, d_mesh_triangle_counts, scene->settings.meshes)
}