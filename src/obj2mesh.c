#ifdef COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "./SlimTracin/core/types.h"
#include "./SlimTracin/math/vec3.h"
#include "./SlimTracin/math/mat3.h"
#include "./SlimTracin/render/acceleration_structures/builder_top_down.h"

enum VertexAttributes {
    VertexAttributes_None,
    VertexAttributes_Positions,
    VertexAttributes_PositionsAndUVs,
    VertexAttributes_PositionsUVsAndNormals
};

int obj2mesh(char* obj_file_path, char* mesh_file_path, bool invert_winding_order) {
    Mesh mesh;
    mesh.aabb.min.x = mesh.aabb.min.y = mesh.aabb.min.z = 0;
    mesh.aabb.max.x = mesh.aabb.max.y = mesh.aabb.max.z = 0;
    mesh.triangle_count = 0;
    mesh.normals_count = 0;
    mesh.vertex_count = 0;
    mesh.edge_count = 0;
    mesh.uvs_count = 0;
    mesh.bvh.node_count = 0;
    mesh.vertex_normals          = null;
    mesh.vertex_normal_indices   = null;
    mesh.vertex_uvs              = null;
    mesh.vertex_uvs_indices      = null;

    FILE* file;
    file = fopen(obj_file_path, "r");
    char line[1024];

    enum VertexAttributes vertex_attributes = VertexAttributes_None;
    while (fgets(line, 1024, file)) {
        if (strncmp(line, (char*)"vn ", 2) == 0) mesh.normals_count++;
        if (strncmp(line, (char*)"vt ", 2) == 0) mesh.uvs_count++;
        if (strncmp(line, (char*)"v ", 2) == 0) mesh.vertex_count++;
        if (strncmp(line, (char*)"f ", 2) == 0) {
            mesh.triangle_count++;
            if (vertex_attributes == VertexAttributes_None) {
                int forward_slash_count = 0;
                char *character = line;
                while (*character) {
                    if ((*character) == '/') forward_slash_count++;
                    character++;
                }
                switch (forward_slash_count) {
                    case 0: vertex_attributes = VertexAttributes_Positions; break;
                    case 3: vertex_attributes = VertexAttributes_PositionsAndUVs; break;
                    case 6: vertex_attributes = VertexAttributes_PositionsUVsAndNormals; break;
                    default: break;
                }
            }
        }
    }
    fclose(file);

    mesh.triangles               = (Triangle*             )malloc(sizeof(Triangle             ) * mesh.triangle_count);
    mesh.vertex_position_indices = (TriangleVertexIndices*)malloc(sizeof(TriangleVertexIndices) * mesh.triangle_count);
    mesh.vertex_positions        = (                 vec3*)malloc(sizeof(vec3                 ) * mesh.vertex_count);
    mesh.edge_vertex_indices     = (    EdgeVertexIndices*)malloc(sizeof(EdgeVertexIndices    ) * mesh.triangle_count * 3);

    if (vertex_attributes == VertexAttributes_PositionsUVsAndNormals) {
        mesh.vertex_normals        = (                 vec3*)malloc(sizeof(vec3                 ) * mesh.normals_count);
        mesh.vertex_normal_indices = (TriangleVertexIndices*)malloc(sizeof(TriangleVertexIndices) * mesh.triangle_count);
        mesh.vertex_uvs            = (                 vec2*)malloc(sizeof(vec2                 ) * mesh.uvs_count);
        mesh.vertex_uvs_indices    = (TriangleVertexIndices*)malloc(sizeof(TriangleVertexIndices) * mesh.triangle_count);
    } else if (vertex_attributes == VertexAttributes_PositionsAndUVs) {
        mesh.vertex_uvs            = (                 vec2*)malloc(sizeof(vec2                 ) * mesh.uvs_count);
        mesh.vertex_uvs_indices    = (TriangleVertexIndices*)malloc(sizeof(TriangleVertexIndices) * mesh.triangle_count);
    }

    mesh.bvh.nodes    = (BVHNode*)malloc(sizeof(BVHNode) * mesh.triangle_count * 2);
    mesh.bvh.leaf_ids = (u32*    )malloc(sizeof(u32)     * mesh.triangle_count);

    BVHBuilder builder;
    builder.build_iterations = (BuildIteration*)malloc(sizeof(BuildIteration) * mesh.triangle_count);
    builder.leaf_nodes       = (BVHNode*       )malloc(sizeof(BVHNode)        * mesh.triangle_count);
    builder.leaf_ids         = (u32*           )malloc(sizeof(u32)            * mesh.triangle_count);
    builder.sort_stack       = (i32*           )malloc(sizeof(i32)            * mesh.triangle_count);

    PartitionAxis *pa = builder.partition_axis;
    for (u8 i = 0; i < 3; i++, pa++) {
        pa->sorted_leaf_ids     = (u32* )malloc(sizeof(u32)  * mesh.triangle_count);
        pa->left.aabbs          = (AABB*)malloc(sizeof(AABB) * mesh.triangle_count);
        pa->right.aabbs         = (AABB*)malloc(sizeof(AABB) * mesh.triangle_count);
        pa->left.surface_areas  = (f32* )malloc(sizeof(f32)  * mesh.triangle_count);
        pa->right.surface_areas = (f32* )malloc(sizeof(f32)  * mesh.triangle_count);
    }

    vec3 *vertex_position = mesh.vertex_positions;
    vec3 *vertex_normal = mesh.vertex_normals;
    vec2 *vertex_uvs = mesh.vertex_uvs;
    TriangleVertexIndices *vertex_position_indices = mesh.vertex_position_indices;
    TriangleVertexIndices *vertex_normal_indices = mesh.vertex_normal_indices;
    TriangleVertexIndices *vertex_uvs_indices = mesh.vertex_uvs_indices;

    u8 v1_id = 0;
    u8 v2_id = invert_winding_order ? 2 : 1;
    u8 v3_id = invert_winding_order ? 1 : 2;

    file = fopen(obj_file_path, (char*)"r");
    while (fgets(line, 1024, file)) {
        // Vertex information
        if (strncmp(line, (char*)"v ", 2) == 0) {
            sscanf(line, (char*)"v %f %f %f", &vertex_position->x, &vertex_position->y, &vertex_position->z);
            vertex_position++;
        } else if (strncmp(line, (char*)"vn ", 2) == 0) {
            sscanf(line, (char*)"vn %f %f %f", &vertex_normal->x, &vertex_normal->y, &vertex_normal->z);
            vertex_normal++;
        } else if (strncmp(line, (char*)"vt ", 2) == 0) {
            sscanf(line, (char*)"vt %f %f", &vertex_uvs->x, &vertex_uvs->y);
            vertex_uvs++;
        } else if (strncmp(line, (char*)"f ", 2) == 0) {
            int vertex_indices[3];
            int uvs_indices[3];
            int normal_indices[3];

            switch (vertex_attributes) {
                case VertexAttributes_Positions:
                    sscanf(
                            line, (char*)"f %d %d %d",
                            &vertex_indices[v1_id],
                            &vertex_indices[v2_id],
                            &vertex_indices[v3_id]
                    );
                    break;
                case VertexAttributes_PositionsAndUVs:
                    sscanf(
                            line, (char*)"f %d/%d %d/%d %d/%d",
                            &vertex_indices[v1_id], &uvs_indices[v1_id],
                            &vertex_indices[v2_id], &uvs_indices[v2_id],
                            &vertex_indices[v3_id], &uvs_indices[v3_id]
                    );
                    vertex_uvs_indices->ids[0] = uvs_indices[0] - 1;
                    vertex_uvs_indices->ids[1] = uvs_indices[1] - 1;
                    vertex_uvs_indices->ids[2] = uvs_indices[2] - 1;
                    vertex_uvs_indices++;
                    break;
                case VertexAttributes_PositionsUVsAndNormals:
                    sscanf(
                            line, (char*)"f %d/%d/%d %d/%d/%d %d/%d/%d",
                            &vertex_indices[v1_id], &uvs_indices[v1_id], &normal_indices[v1_id],
                            &vertex_indices[v2_id], &uvs_indices[v2_id], &normal_indices[v2_id],
                            &vertex_indices[v3_id], &uvs_indices[v3_id], &normal_indices[v3_id]
                    );
                    vertex_uvs_indices->ids[0] = uvs_indices[0] - 1;
                    vertex_uvs_indices->ids[1] = uvs_indices[1] - 1;
                    vertex_uvs_indices->ids[2] = uvs_indices[2] - 1;
                    vertex_normal_indices->ids[0] = normal_indices[0] - 1;
                    vertex_normal_indices->ids[1] = normal_indices[1] - 1;
                    vertex_normal_indices->ids[2] = normal_indices[2] - 1;
                    vertex_normal_indices++;
                    vertex_uvs_indices++;
                    break;
                default:
                    return 1;
            }
            vertex_position_indices->ids[0] = vertex_indices[0] - 1;
            vertex_position_indices->ids[1] = vertex_indices[1] - 1;
            vertex_position_indices->ids[2] = vertex_indices[2] - 1;
            vertex_position_indices++;
        }
    }
    fclose(file);

//    // Dog/Monkey >
//    mat3 rot45 = getMat3Identity();
//    rot45.X.x = 0.70710678118f;
//    rot45.X.z = 0.70710678118f;
//    rot45.Z.x = -0.70710678118f;
//    rot45.Z.z = 0.70710678118f;
//    mat3 rot90 = mulMat3(rot45, rot45);
////    mat3 rot = mulMat3(rot45, rot90); // Dog
//    mat3 rot = rot90; // Monkey
//
//    vertex_position = mesh.vertex_normals;
//    for (u32 i = 0; i < mesh.normals_count; i++, vertex_position++)
//        *vertex_position = mulVec3Mat3(*vertex_position, rot);
//    // Dog/Monkey <

    vertex_position = mesh.vertex_positions;
    for (u32 i = 0; i < mesh.vertex_count; i++, vertex_position++) {
//        *vertex_position = mulVec3Mat3(*vertex_position, rot); // Dog/Monkey
        mesh.aabb.min.x = mesh.aabb.min.x < vertex_position->x ? mesh.aabb.min.x : vertex_position->x;
        mesh.aabb.min.y = mesh.aabb.min.y < vertex_position->y ? mesh.aabb.min.y : vertex_position->y;
        mesh.aabb.min.z = mesh.aabb.min.z < vertex_position->z ? mesh.aabb.min.z : vertex_position->z;
        mesh.aabb.max.x = mesh.aabb.max.x > vertex_position->x ? mesh.aabb.max.x : vertex_position->x;
        mesh.aabb.max.y = mesh.aabb.max.y > vertex_position->y ? mesh.aabb.max.y : vertex_position->y;
        mesh.aabb.max.z = mesh.aabb.max.z > vertex_position->z ? mesh.aabb.max.z : vertex_position->z;
    }

    vec3 centroid = scaleVec3(addVec3(mesh.aabb.min, mesh.aabb.max), 0.5f);
    if (nonZeroVec3(centroid)) {
        mesh.aabb.min = subVec3(mesh.aabb.min, centroid);
        mesh.aabb.max = subVec3(mesh.aabb.max, centroid);
        vertex_position = mesh.vertex_positions;
        for (u32 i = 0; i < mesh.vertex_count; i++, vertex_position++)
            *vertex_position = subVec3(*vertex_position, centroid);
    }

//    // Dog/Monkey >
////    f32 scale = 0.2f; // dog
//    f32 scale = 4.0f; // dog
//    vertex_position = mesh.vertex_positions;
//    for (u32 i = 0; i < mesh.vertex_count; i++, vertex_position++)
//        *vertex_position = scaleVec3(*vertex_position, scale);
//    mesh.aabb.min = scaleVec3(mesh.aabb.min, scale);
//    mesh.aabb.max = scaleVec3(mesh.aabb.max, scale);
//    // Dog/Monkey <

    EdgeVertexIndices current_edge_vertex_indices, *edge_vertex_indices;
    vertex_position_indices = mesh.vertex_position_indices;
    for (u32 i = 0; i < mesh.triangle_count; i++, vertex_position_indices++) {
        for (u8 from = 0, to = 1; from < 3; from++, to = (to + 1) % 3) {
            current_edge_vertex_indices.from = vertex_position_indices->ids[from];
            current_edge_vertex_indices.to   = vertex_position_indices->ids[to];
            if (current_edge_vertex_indices.from > current_edge_vertex_indices.to) {
                u32 temp = current_edge_vertex_indices.from;
                current_edge_vertex_indices.from = current_edge_vertex_indices.to;
                current_edge_vertex_indices.to = temp;
            }

            bool found = false;
            edge_vertex_indices = mesh.edge_vertex_indices;
            for (u32 e = 0; e < mesh.edge_count; e++, edge_vertex_indices++) {
                if (edge_vertex_indices->from == current_edge_vertex_indices.from &&
                    edge_vertex_indices->to   == current_edge_vertex_indices.to) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                mesh.edge_vertex_indices[mesh.edge_count] = current_edge_vertex_indices;
                mesh.edge_count++;
            }
        }
    }

    updateMeshBVH(&mesh, &builder);

    file = fopen(mesh_file_path, (char*)"wb");

    fwrite(&mesh.aabb,           sizeof(AABB), 1, file);
    fwrite(&mesh.vertex_count,   sizeof(u32),  1, file);
    fwrite(&mesh.triangle_count, sizeof(u32),  1, file);
    fwrite(&mesh.edge_count,     sizeof(u32),  1, file);
    fwrite(&mesh.uvs_count,      sizeof(u32),  1, file);
    fwrite(&mesh.normals_count,  sizeof(u32),  1, file);
    fwrite(&mesh.bvh.node_count, sizeof(u32),  1, file);
    fwrite(&mesh.bvh.height,     sizeof(u32),  1, file);

    fwrite( mesh.vertex_positions,        sizeof(vec3)                 , mesh.vertex_count,   file);
    fwrite( mesh.vertex_position_indices, sizeof(TriangleVertexIndices), mesh.triangle_count, file);
    fwrite( mesh.edge_vertex_indices,     sizeof(EdgeVertexIndices)    , mesh.edge_count,     file);
    if (mesh.uvs_count) {
        fwrite(mesh.vertex_uvs,          sizeof(vec2)                  , mesh.uvs_count,      file);
        fwrite(mesh.vertex_uvs_indices,  sizeof(TriangleVertexIndices) , mesh.triangle_count, file);
    }
    if (mesh.normals_count) {
        fwrite(mesh.vertex_normals,        sizeof(vec3)                  , mesh.normals_count,  file);
        fwrite(mesh.vertex_normal_indices, sizeof(TriangleVertexIndices) , mesh.triangle_count, file);
    }

    fwrite( mesh.triangles,               sizeof(Triangle)             , mesh.triangle_count, file);
    fwrite( mesh.bvh.nodes,               sizeof(BVHNode)              , mesh.bvh.node_count, file);
    fwrite( mesh.bvh.leaf_ids,            sizeof(u32)                  , mesh.triangle_count, file);

    fclose(file);

    return 0;
}
int EndsWith(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int main(int argc, char *argv[]) {
    // Error if less than 2 arguments were provided
    bool valid_input = argc >= 3 && (EndsWith(argv[1], ".obj") && (EndsWith(argv[2], ".mesh")));
    if (!valid_input) {
        printf("Exactly 2 file paths need to be provided: A '.obj' file (input) then a '.mesh' file (output)");
        return 1;
    }

    char* src_file_path = argv[1];
    char* trg_file_path = argv[2];
    bool invert_winding_order = false;
    for (u8 i = 3; i < (u8)argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'i') invert_winding_order = true;
        else {
            printf("Unknown argument: %s", argv[i]);
            valid_input = false;
            break;
        }
    }
    return valid_input ? obj2mesh(src_file_path, trg_file_path, invert_winding_order) : 1;
}