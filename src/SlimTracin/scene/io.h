#pragma once

#include "../core/base.h"
#include "../core/types.h"

u32 getTextureMemorySize(char* file_path, Platform *platform) {
    void *file = platform->openFileForReading(file_path);

    Texture texture;
    platform->readFromFile(&texture.width,  sizeof(u16),  file);
    platform->readFromFile(&texture.height, sizeof(u16),  file);
    platform->readFromFile(&texture.mipmap, sizeof(bool), file);

    u16 mip_width  = texture.width;
    u16 mip_height = texture.height;

    u32 memory_size = 0;
    texture.mip_count = 0;
    do {
        memory_size += sizeof(TextureMip);
        memory_size += (mip_width + 1) * (mip_height + 1) * sizeof(TexelQuad);

        mip_width /= 2;
        mip_height /= 2;
        texture.mip_count++;
    } while (texture.mipmap && mip_width > 2 && mip_height > 2);

    platform->closeFile(file);

    return memory_size;
}

void loadTextureFromFile(Texture *texture, char* file_path, Platform *platform, Memory *memory) {
    void *file = platform->openFileForReading(file_path);
    platform->readFromFile(&texture->width,  sizeof(u16),  file);
    platform->readFromFile(&texture->height, sizeof(u16),  file);
    platform->readFromFile(&texture->mipmap, sizeof(bool), file);
    platform->readFromFile(&texture->wrap,   sizeof(bool), file);
    platform->readFromFile(&texture->mip_count, sizeof(u8), file);

    texture->mips = (TextureMip*)allocateMemory(memory, sizeof(TextureMip) * texture->mip_count);

    u32 size, height, stride;
    TextureMip *texture_mip = texture->mips;
    for (u8 mip_index = 0; mip_index < texture->mip_count; mip_index++, texture_mip++) {
        platform->readFromFile(&texture_mip->width,  sizeof(u16), file);
        platform->readFromFile(&texture_mip->height, sizeof(u16), file);

        height = texture_mip->height + 1;
        stride = texture_mip->width  + 1;

        size = sizeof(TexelQuad) * height * stride;
        texture_mip->texel_quads = (TexelQuad*)allocateMemory(memory, size);
        platform->readFromFile(texture_mip->texel_quads, size, file);
    }

    platform->closeFile(file);
}


u32 getMeshMemorySize(Mesh *mesh, char *file_path, Platform *platform) {
    void *file = platform->openFileForReading(file_path);

    platform->readFromFile(&mesh->aabb,           sizeof(AABB), file);
    platform->readFromFile(&mesh->vertex_count,   sizeof(u32),  file);
    platform->readFromFile(&mesh->triangle_count, sizeof(u32),  file);
    platform->readFromFile(&mesh->edge_count,     sizeof(u32),  file);
    platform->readFromFile(&mesh->uvs_count,      sizeof(u32),  file);
    platform->readFromFile(&mesh->normals_count,  sizeof(u32),  file);
    platform->readFromFile(&mesh->bvh.node_count, sizeof(u32),  file);
    platform->readFromFile(&mesh->bvh.height,     sizeof(u32),  file);

    u32 memory_size = 0;
    memory_size += mesh->vertex_count   * sizeof(vec3);
    memory_size += mesh->triangle_count * sizeof(TriangleVertexIndices);
    memory_size += mesh->edge_count     * sizeof(EdgeVertexIndices);
    memory_size += mesh->triangle_count * sizeof(Triangle);

    if (mesh->uvs_count) {
        memory_size += sizeof(vec2) * mesh->uvs_count;
        memory_size += sizeof(TriangleVertexIndices) * mesh->triangle_count;
    }
    if (mesh->normals_count) {
        memory_size += sizeof(vec3) * mesh->normals_count;
        memory_size += sizeof(TriangleVertexIndices) * mesh->triangle_count;
    }

    platform->closeFile(file);

    memory_size += getBVHMemorySize(mesh->triangle_count);

    return memory_size;
}

void loadMeshFromFile(Mesh *mesh, char *file_path, Platform *platform, Memory *memory) {
    void *file = platform->openFileForReading(file_path);

    mesh->vertex_normals          = null;
    mesh->vertex_normal_indices   = null;
    mesh->vertex_uvs              = null;
    mesh->vertex_uvs_indices      = null;
    mesh->triangles               = null;
    mesh->bvh.nodes               = null;
    mesh->bvh.leaf_ids            = null;

    platform->readFromFile(&mesh->aabb,           sizeof(AABB), file);
    platform->readFromFile(&mesh->vertex_count,   sizeof(u32),  file);
    platform->readFromFile(&mesh->triangle_count, sizeof(u32),  file);

    initBVH(&mesh->bvh, mesh->triangle_count, memory);

    platform->readFromFile(&mesh->edge_count,     sizeof(u32),  file);
    platform->readFromFile(&mesh->uvs_count,      sizeof(u32),  file);
    platform->readFromFile(&mesh->normals_count,  sizeof(u32),  file);
    platform->readFromFile(&mesh->bvh.node_count, sizeof(u32),  file);
    platform->readFromFile(&mesh->bvh.height,     sizeof(u32),  file);

    mesh->vertex_positions        = (vec3*                 )allocateMemory(memory, sizeof(vec3)                  * mesh->vertex_count);
    mesh->vertex_position_indices = (TriangleVertexIndices*)allocateMemory(memory, sizeof(TriangleVertexIndices) * mesh->triangle_count);
    mesh->edge_vertex_indices     = (EdgeVertexIndices*    )allocateMemory(memory, sizeof(EdgeVertexIndices)     * mesh->edge_count);
    mesh->triangles               = (Triangle*             )allocateMemory(memory, sizeof(Triangle)              * mesh->triangle_count);

    platform->readFromFile(mesh->vertex_positions,             sizeof(vec3)                  * mesh->vertex_count,   file);
    platform->readFromFile(mesh->vertex_position_indices,      sizeof(TriangleVertexIndices) * mesh->triangle_count, file);
    platform->readFromFile(mesh->edge_vertex_indices,          sizeof(EdgeVertexIndices)     * mesh->edge_count,     file);
    if (mesh->uvs_count) {
        mesh->vertex_uvs         = (vec2*                 )allocateMemory(memory, sizeof(vec2)                  * mesh->uvs_count);
        mesh->vertex_uvs_indices = (TriangleVertexIndices*)allocateMemory(memory, sizeof(TriangleVertexIndices) * mesh->triangle_count);
        platform->readFromFile(mesh->vertex_uvs,               sizeof(vec2)                  * mesh->uvs_count,      file);
        platform->readFromFile(mesh->vertex_uvs_indices,       sizeof(TriangleVertexIndices) * mesh->triangle_count, file);
    }
    if (mesh->normals_count) {
        mesh->vertex_normals          = (vec3*                 )allocateMemory(memory, sizeof(vec3)                  * mesh->normals_count);
        mesh->vertex_normal_indices   = (TriangleVertexIndices*)allocateMemory(memory, sizeof(TriangleVertexIndices) * mesh->triangle_count);
        platform->readFromFile(mesh->vertex_normals,                sizeof(vec3)                  * mesh->normals_count,  file);
        platform->readFromFile(mesh->vertex_normal_indices,         sizeof(TriangleVertexIndices) * mesh->triangle_count, file);
    }

    platform->readFromFile(mesh->triangles,                    sizeof(Triangle)              * mesh->triangle_count, file);
    platform->readFromFile(mesh->bvh.nodes,                    sizeof(BVHNode)               * mesh->bvh.node_count, file);
    platform->readFromFile(mesh->bvh.leaf_ids,                 sizeof(u32)                   * mesh->triangle_count, file);

    platform->closeFile(file);
}

void saveMeshToFile(Mesh *mesh, char* file_path, Platform *platform) {
    void *file = platform->openFileForWriting(file_path);

    platform->writeToFile(&mesh->aabb,           sizeof(AABB), file);
    platform->writeToFile(&mesh->vertex_count,   sizeof(u32),  file);
    platform->writeToFile(&mesh->triangle_count, sizeof(u32),  file);
    platform->writeToFile(&mesh->edge_count,     sizeof(u32),  file);
    platform->writeToFile(&mesh->uvs_count,      sizeof(u32),  file);
    platform->writeToFile(&mesh->normals_count,  sizeof(u32),  file);
    platform->writeToFile(&mesh->bvh.node_count, sizeof(u32),  file);
    platform->writeToFile(&mesh->bvh.height,     sizeof(u32),  file);

    platform->writeToFile(mesh->vertex_positions,        sizeof(vec3)                  * mesh->vertex_count,   file);
    platform->writeToFile(mesh->vertex_position_indices, sizeof(TriangleVertexIndices) * mesh->triangle_count, file);
    platform->writeToFile(mesh->edge_vertex_indices,     sizeof(EdgeVertexIndices)     * mesh->edge_count,     file);
    if (mesh->uvs_count) {
        platform->writeToFile(mesh->vertex_uvs,          sizeof(vec2)                  * mesh->uvs_count,      file);
        platform->writeToFile(mesh->vertex_uvs_indices,  sizeof(TriangleVertexIndices) * mesh->triangle_count, file);
    }
    if (mesh->normals_count) {
        platform->writeToFile(mesh->vertex_normals,        sizeof(vec3)                  * mesh->normals_count,  file);
        platform->writeToFile(mesh->vertex_normal_indices, sizeof(TriangleVertexIndices) * mesh->triangle_count, file);
    }

    platform->writeToFile(mesh->triangles,               sizeof(Triangle)              * mesh->triangle_count, file);
    platform->writeToFile(mesh->bvh.nodes,               sizeof(BVHNode)               * mesh->bvh.node_count, file);
    platform->writeToFile(mesh->bvh.leaf_ids,            sizeof(u32)                   * mesh->triangle_count, file);

    platform->closeFile(file);
}


void loadSceneFromFile(Scene *scene, char* file_path, Platform *platform) {
    void *file = platform->openFileForReading(file_path);

    String current_file = scene->settings.file;
    platform->readFromFile(&scene->settings, sizeof(SceneSettings), file);
    scene->settings.file = current_file;

    if (scene->cameras)
        for (u32 i = 0; i < scene->settings.cameras; i++) {
            platform->readFromFile(scene->cameras + i, sizeof(Camera), file);
            scene->cameras[i].transform.right_direction   = &scene->cameras[i].transform.rotation_matrix.X;
            scene->cameras[i].transform.up_direction      = &scene->cameras[i].transform.rotation_matrix.Y;
            scene->cameras[i].transform.forward_direction = &scene->cameras[i].transform.rotation_matrix.Z;
        }

    if (scene->primitives)
        for (u32 i = 0; i < scene->settings.primitives; i++)
            platform->readFromFile(scene->primitives + i, sizeof(Primitive), file);

    platform->closeFile(file);
}

void saveSceneToFile(Scene *scene, char* file_path, Platform *platform) {
    void *file = platform->openFileForWriting(file_path);

    platform->writeToFile(&scene->settings, sizeof(SceneSettings), file);

    if (scene->cameras)
        for (u32 i = 0; i < scene->settings.cameras; i++)
            platform->writeToFile(scene->cameras + i, sizeof(Camera), file);

    if (scene->primitives)
        for (u32 i = 0; i < scene->settings.primitives; i++)
            platform->writeToFile(scene->primitives + i, sizeof(Primitive), file);

    platform->closeFile(file);
}