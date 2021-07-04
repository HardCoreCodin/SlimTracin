#pragma once

#include "../../shapes/line.h"
#include "../../scene/box.h"
#include "../../render/AABB.h"
#include "../../math/mat3.h"
#include "../../math/vec3.h"


typedef struct {
    AABB *aabbs;
    f32 *surface_areas;
} PartitionSide;

typedef struct {
    PartitionSide left, right;
    u32 left_node_count, *sorted_leaf_ids;
    f32 smallest_surface_area;
} PartitionAxis;

typedef struct {
    u32 start, end, node_id;
} BuildIteration;

typedef struct {
    BuildIteration *build_iterations;
    BVHNode *leaf_nodes;
    u32 *leaf_ids;
    i32 *sort_stack;
    PartitionAxis partition_axis[3];
} BVHBuilder;

u32 getBVHBuilderMemorySize(Scene *scene, u32 max_leaf_count) {
    u32 memory_size = sizeof(u32) + sizeof(i32) + 2 * (sizeof(AABB) + sizeof(f32));
    memory_size *= 3;
    memory_size += sizeof(BuildIteration) + sizeof(BVHNode) + sizeof(u32);
    memory_size *= max_leaf_count;

    return memory_size;
}

void initBVHBuilder(BVHBuilder *builder, Scene *scene, Memory *memory) {
    u32 max_triangle_count = 0;
    if (scene->settings.meshes)
        for (u32 m = 0; m < scene->settings.meshes; m++)
            if (scene->meshes[m].triangle_count > max_triangle_count)
                max_triangle_count = scene->meshes[m].triangle_count;

    u32 leaf_node_count = scene->settings.primitives > max_triangle_count ? scene->settings.primitives : max_triangle_count;

    builder->build_iterations = (BuildIteration*)allocateMemory(memory, sizeof(BuildIteration) * leaf_node_count);
    builder->leaf_nodes       = (BVHNode*       )allocateMemory(memory, sizeof(BVHNode)        * leaf_node_count);
    builder->leaf_ids         = (u32*           )allocateMemory(memory, sizeof(u32)            * leaf_node_count);
    builder->sort_stack       = (i32*           )allocateMemory(memory, sizeof(i32)            * leaf_node_count);

    PartitionAxis *pa = builder->partition_axis;
    for (u8 i = 0; i < 3; i++, pa++) {
        pa->sorted_leaf_ids     = (u32* )allocateMemory(memory, sizeof(u32)  * leaf_node_count);
        pa->left.aabbs          = (AABB*)allocateMemory(memory, sizeof(AABB) * leaf_node_count);
        pa->right.aabbs         = (AABB*)allocateMemory(memory, sizeof(AABB) * leaf_node_count);
        pa->left.surface_areas  = (f32* )allocateMemory(memory, sizeof(f32)  * leaf_node_count);
        pa->right.surface_areas = (f32* )allocateMemory(memory, sizeof(f32)  * leaf_node_count);
    }
}

i32 partitionNodesByAxis(BVHNode *nodes, u8 axis, i32 start, i32 end, u32 *leaf_ids) {
    i32 pIndex = start;
    i32 pivot = (i32)leaf_ids[end];
    i32 t;
    bool do_swap;

    for (i32 i = start; i < end; i++) {
        switch (axis) {
            case 0: do_swap = nodes[leaf_ids[i]].aabb.max.x < nodes[pivot].aabb.max.x; break;
            case 1: do_swap = nodes[leaf_ids[i]].aabb.max.y < nodes[pivot].aabb.max.y; break;
            case 2: do_swap = nodes[leaf_ids[i]].aabb.max.z < nodes[pivot].aabb.max.z; break;
        }
        if (do_swap) {
            t = (i32)leaf_ids[i];
            leaf_ids[i] = leaf_ids[pIndex];
            leaf_ids[pIndex] = t;
            pIndex++;
        }
    }
    t = (i32)leaf_ids[end];
    leaf_ids[end] = leaf_ids[pIndex];
    leaf_ids[pIndex] = t;

    return pIndex;
}

void sortNodesByAxis(BVHNode *nodes, u8 axis, i32 start, i32 end, i32 *stack, u32 *leaf_ids) {
    i32 top = -1;

    // push initial values of start and end to stack
    stack[++top] = start;
    stack[++top] = end;

    // Keep popping from stack while is not empty
    while (top >= 0) {
        // Pop h and l
        end = stack[top--];
        start = stack[top--];

        // Set pivot element at its correct position
        // in sorted array
        i32 pIndex = partitionNodesByAxis(nodes, axis, start, end, leaf_ids);

        // If there are elements on left side of pivot,
        // then push left side to stack
        if (pIndex - 1 > start) {
            stack[++top] = start;
            stack[++top] = pIndex - 1;
        }

        // If there are elements on right side of pivot,
        // then push right side to stack
        if (pIndex + 1 < end) {
            stack[++top] = pIndex + 1;
            stack[++top] = end;
        }
    }
}

void partitionBVHNodeIDs(PartitionAxis *pa, u8 axis, BVHNode *nodes, i32 *stack, u32 N) {
    u32 current_index, next_index, left_index, right_index;
    f32 current_surface_area;
    left_index = 0;
    right_index = N - 1;

    sortNodesByAxis(nodes, axis, 0, (i32)N-1, stack, pa->sorted_leaf_ids);

    AABB L = nodes[pa->sorted_leaf_ids[left_index]].aabb;
    AABB R = nodes[pa->sorted_leaf_ids[right_index]].aabb;

    pa->left.aabbs[left_index]   = L;
    pa->right.aabbs[right_index] = R;

    for (left_index = 0; left_index < N; left_index++, right_index--) {
        if (left_index) {
            L = mergeAABBs(L, nodes[pa->sorted_leaf_ids[left_index]].aabb);
            R = mergeAABBs(R, nodes[pa->sorted_leaf_ids[right_index]].aabb);

            pa->left.aabbs[left_index] = L;
            pa->right.aabbs[right_index] = R;
        }

        pa->left.surface_areas[  left_index] = getSurfaceAreaOfAABB(L);
        pa->right.surface_areas[right_index] = getSurfaceAreaOfAABB(R);
    }

    u32 last_index = right_index = N - 1;
    pa->left_node_count = left_index = next_index = 1;
    pa->smallest_surface_area = INFINITY;

    for (current_index = 0; current_index < last_index; current_index++, next_index++, left_index++, right_index--) {
        current_surface_area = (
                pa->left.surface_areas[current_index] * (f32)left_index +
                pa->right.surface_areas[  next_index] * (f32)right_index
        );
        if (pa->smallest_surface_area > current_surface_area) {
            pa->smallest_surface_area = current_surface_area;
            pa->left_node_count = next_index;
        }
    }
}

u32 splitBVHNode(BVHNode *bvh_nodes, u32 *bvh_node_count, BVHBuilder *builder, BVHNode *node, u32 start, u32 end) {
    u32 N = end - start;
    u32 *leaf_ids = builder->leaf_ids + start;

    node->first_child_id = *bvh_node_count;
    BVHNode *left_node  = bvh_nodes + (*bvh_node_count)++;
    BVHNode *right_node = bvh_nodes + (*bvh_node_count)++;

    initBVHNode(left_node);
    initBVHNode(right_node);

    f32 smallest_surface_area = INFINITY;
    PartitionAxis *chosen_partition_axis = builder->partition_axis, *pa = builder->partition_axis;

    for (u8 axis = 0; axis < 3; axis++, pa++) {
        for (u32 i = 0; i < N; i++) pa->sorted_leaf_ids[i] = leaf_ids[i];

        // Partition the BVH's primitive-ids for the current partition axis:
        partitionBVHNodeIDs(pa, axis, builder->leaf_nodes, builder->sort_stack, N);

        // Choose the current partition axis if it's smallest surface area is smallest so far:
        if (pa->smallest_surface_area < smallest_surface_area) {
            smallest_surface_area = pa->smallest_surface_area;
            chosen_partition_axis = pa;
        }
    }

    u32 left_count = chosen_partition_axis->left_node_count;
    left_node->aabb  = chosen_partition_axis->left.aabbs[left_count-1];
    right_node->aabb = chosen_partition_axis->right.aabbs[left_count];

    for (u32 i = 0; i < N; i++) leaf_ids[i] = chosen_partition_axis->sorted_leaf_ids[i];

    return start + left_count;
}

void buildBVH(BVH *bvh, BVHBuilder *builder, u32 N, u32 max_leaf_size) {
    bvh->node_count = 1;
    initBVHNode(bvh->nodes);

    if (N <= max_leaf_size) {
        bvh->nodes->aabb.min = getVec3Of(INFINITY);
        bvh->nodes->aabb.max = getVec3Of(-INFINITY);
        BVHNode *builder_node = builder->leaf_nodes;
        for (u32 i = 0; i < N; i++, builder_node++) {
            bvh->leaf_ids[i] = builder_node->first_child_id;
            bvh->nodes->aabb = mergeAABBs(bvh->nodes->aabb, builder_node->aabb);
        }
        bvh->nodes->primitive_count = N;
        bvh->nodes->first_child_id = 0;
        bvh->depth = 1;

        return;
    }

    u32 middle = splitBVHNode(bvh->nodes, &bvh->node_count, builder, bvh->nodes, 0, N);
    BuildIteration *stack = builder->build_iterations;
    u32 *leaf_id;

    stack[0].start = 0;
    stack[0].end = middle;
    stack[0].node_id = 1;

    stack[1].start = middle;
    stack[1].end = N;
    stack[1].node_id = 2;

    i32 index = 1;
    u32 leaf_count = 0;
    BuildIteration left, right;
    BVHNode *node;

    bvh->depth = 1;

    while (index >= 0) {
        left = stack[index];
        node = bvh->nodes + left.node_id;
        N = left.end - left.start;
        if (N <= max_leaf_size) {
            node->primitive_count = N;
            node->first_child_id = leaf_count;
            leaf_id = builder->leaf_ids + left.start;
            for (u32 i = 0; i < N; i++, leaf_id++)
                bvh->leaf_ids[leaf_count + i] = builder->leaf_nodes[*leaf_id].first_child_id;
            leaf_count += N;
            index--;
        } else {
            middle = splitBVHNode(bvh->nodes, &bvh->node_count, builder, node, left.start, left.end);
            right.end = left.end;
            right.start = left.end = middle;
            left.node_id  = node->first_child_id;
            right.node_id = node->first_child_id + 1;
            stack[  index] = left;
            stack[++index] = right;
            if (index > (i32)bvh->depth) bvh->depth = (u32)index;
        }
    }

    bvh->nodes->aabb = mergeAABBs(bvh->nodes[1].aabb, bvh->nodes[2].aabb);
}

void updateMeshBVH(Mesh *mesh, BVHBuilder *builder) {
    BVHNode *leaf_node = builder->leaf_nodes;
    TriangleVertexIndices *indices = mesh->vertex_position_indices;
    vec3 *v1, *v2, *v3;
    f32 diff;

    for (u32 i = 0; i < mesh->triangle_count; i++, leaf_node++, indices++) {
        v1 = mesh->vertex_positions + indices->ids[0];
        v2 = mesh->vertex_positions + indices->ids[1];
        v3 = mesh->vertex_positions + indices->ids[2];

        leaf_node->aabb.min.x = v2->x < v3->x ? v2->x : v3->x;
        leaf_node->aabb.min.y = v2->y < v3->y ? v2->y : v3->y;
        leaf_node->aabb.min.z = v2->z < v3->z ? v2->z : v3->z;

        leaf_node->aabb.max.x = v2->x > v3->x ? v2->x : v3->x;
        leaf_node->aabb.max.y = v2->y > v3->y ? v2->y : v3->y;
        leaf_node->aabb.max.z = v2->z > v3->z ? v2->z : v3->z;

        leaf_node->aabb.min.x = leaf_node->aabb.min.x < v1->x ? leaf_node->aabb.min.x : v1->x;
        leaf_node->aabb.min.y = leaf_node->aabb.min.y < v1->y ? leaf_node->aabb.min.y : v1->y;
        leaf_node->aabb.min.z = leaf_node->aabb.min.z < v1->z ? leaf_node->aabb.min.z : v1->z;

        leaf_node->aabb.max.x = leaf_node->aabb.max.x > v1->x ? leaf_node->aabb.max.x : v1->x;
        leaf_node->aabb.max.y = leaf_node->aabb.max.y > v1->y ? leaf_node->aabb.max.y : v1->y;
        leaf_node->aabb.max.z = leaf_node->aabb.max.z > v1->z ? leaf_node->aabb.max.z : v1->z;

        diff = leaf_node->aabb.max.x - leaf_node->aabb.min.x;
        if (diff < 0) diff = -diff;
        if (diff < EPS) {
            leaf_node->aabb.min.x -= EPS;
            leaf_node->aabb.max.x += EPS;
        }

        diff = leaf_node->aabb.max.y - leaf_node->aabb.min.y;
        if (diff < 0) diff = -diff;
        if (diff < EPS) {
            leaf_node->aabb.min.y -= EPS;
            leaf_node->aabb.max.y += EPS;
        }

        diff = leaf_node->aabb.max.z - leaf_node->aabb.min.z;
        if (diff < 0) diff = -diff;
        if (diff < EPS) {
            leaf_node->aabb.min.z -= EPS;
            leaf_node->aabb.max.z += EPS;
        }

        leaf_node->first_child_id = builder->leaf_ids[i] = i;
    }

    buildBVH(&mesh->bvh, builder, mesh->triangle_count, MAX_TRIANGLES_PER_MESH_BVH_NODE);

    mat3 m3;
    Triangle *triangle = mesh->triangles;
    u32 *triangle_id = mesh->bvh.leaf_ids;
    for (u32 i = 0; i < mesh->triangle_count; i++, triangle++, triangle_id++) {
        indices = mesh->vertex_position_indices + *triangle_id;

        v1 = &mesh->vertex_positions[indices->ids[0]];
        v2 = &mesh->vertex_positions[indices->ids[1]];
        v3 = &mesh->vertex_positions[indices->ids[2]];

        m3.X = subVec3(*v3, *v1);
        m3.Y = subVec3(*v2, *v1);
        m3.Z = crossVec3(m3.Y, m3.X);
        m3.Z = normVec3(m3.Z);

        indices = mesh->vertex_normal_indices + *triangle_id;

        triangle->world_to_tangent = invMat3(m3);
        triangle->normal = m3.Z;
        triangle->position = *v1;
        triangle->vertex_normals[0] = mesh->vertex_normals[indices->ids[0]];
        triangle->vertex_normals[1] = mesh->vertex_normals[indices->ids[1]];
        triangle->vertex_normals[2] = mesh->vertex_normals[indices->ids[2]];
    }
}

void updateSceneBVH(Scene *scene, BVHBuilder *builder) {
    BVHNode *leaf_node = builder->leaf_nodes;
    Primitive *primitive  = scene->primitives;

    for (u32 i = 0; i < scene->settings.primitives; i++, primitive++, leaf_node++) {
        if (primitive->type == PrimitiveType_Mesh)
            leaf_node->aabb = scene->meshes[primitive->id].aabb;
        else
            leaf_node->aabb = getPrimitiveAABB(primitive);
        transformAABB(&leaf_node->aabb, primitive);
        leaf_node->first_child_id = builder->leaf_ids[i] = i;
    }

    buildBVH(&scene->bvh, builder, scene->settings.primitives, MAX_OBJS_PER_SCENE_BVH_NODE);
}