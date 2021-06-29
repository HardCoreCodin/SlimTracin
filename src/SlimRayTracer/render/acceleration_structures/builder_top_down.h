#pragma once

#include <stdlib.h>
#include <string.h>

#include "../../shapes/line.h"
#include "../../scene/box.h"
#include "../../render/AABB.h"


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
    PartitionAxis partition_axis[3];
} BVHBuilder;
BVHBuilder bvh_builder;

void initBVHBuilder(BVHBuilder *builder, Scene *scene, Memory *memory) {
    u32 max_triangle_count = 0;
    if (scene->settings.meshes)
        for (u32 m = 0; m < scene->settings.meshes; m++)
            if (scene->meshes[m].triangle_count > max_triangle_count)
                max_triangle_count = scene->meshes[m].triangle_count;

    u32 leaf_node_count = scene->settings.primitives > max_triangle_count ? scene->settings.primitives : max_triangle_count;

    builder->build_iterations = allocateMemory(memory, sizeof(BuildIteration) * leaf_node_count);
    builder->leaf_nodes       = allocateMemory(memory, sizeof(BVHNode)        * leaf_node_count);
    builder->leaf_ids         = allocateMemory(memory, sizeof(u32)            * leaf_node_count);

    PartitionAxis *pa = builder->partition_axis;
    for (u8 i = 0; i < 3; i++, pa++) {
        pa->sorted_leaf_ids     = allocateMemory(memory, sizeof(u32)  * leaf_node_count);
        pa->left.aabbs          = allocateMemory(memory, sizeof(AABB) * leaf_node_count);
        pa->right.aabbs         = allocateMemory(memory, sizeof(AABB) * leaf_node_count);
        pa->left.surface_areas  = allocateMemory(memory, sizeof(f32)  * leaf_node_count);
        pa->right.surface_areas = allocateMemory(memory, sizeof(f32)  * leaf_node_count);
    }
}

u32 partition(u32 *arr, u32 start, u32 end) {
    u32 pIndex = start;
    u32 pivot = arr[end];
    u32 t;
    for (u32 i = start; i < end; i++) {
        if(arr[i] < pivot) {
            t = arr[i];
            arr[i] = arr[pIndex];
            arr[pIndex] = t;
            pIndex++;
        }
    }
    t = arr[end];
    arr[end] = arr[pIndex];
    arr[pIndex] = t;

    return pIndex;
}

void quickSort(u32 *arr, u32 start, u32 end) {
    if (start < end) {
        u32 pIndex = partition(arr, start, end);
        quickSort(arr, start, pIndex-1);
        quickSort(arr, pIndex+1, end);
    }
}

BVHNode *leaf_nodes = null;

int sortCmpX(const void *lhs, const void *rhs) {
    f32 l = leaf_nodes[*((u32*)lhs)].aabb.max.x;
    f32 r = leaf_nodes[*((u32*)rhs)].aabb.max.x;
    return l < r ? -1 : (l > r ? 1 : 0);
}
int sortCmpY(const void *lhs, const void *rhs) {
    f32 l = leaf_nodes[*((u32*)lhs)].aabb.max.y;
    f32 r = leaf_nodes[*((u32*)rhs)].aabb.max.y;
    return l < r ? -1 : (l > r ? 1 : 0);
}
int sortCmpZ(const void *lhs, const void *rhs) {
    f32 l = leaf_nodes[*((u32*)lhs)].aabb.max.z;
    f32 r = leaf_nodes[*((u32*)rhs)].aabb.max.z;
    return l < r ? -1 : (l > r ? 1 : 0);
}

void partitionBVHNodeIDs(PartitionAxis *pa, u8 axis, BVHNode *nodes, u32 N) {
    u32 current_index, next_index, left_index, right_index;
    f32 current_surface_area;
    left_index = 0;
    right_index = N - 1;

    switch (axis) {
        case 0:  qsort(pa->sorted_leaf_ids, N, sizeof(u32), sortCmpX); break;
        case 1:  qsort(pa->sorted_leaf_ids, N, sizeof(u32), sortCmpY); break;
        default: qsort(pa->sorted_leaf_ids, N, sizeof(u32), sortCmpZ); break;
    }

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
    BVHNode *nodes = builder->leaf_nodes;

    node->first_child_id = *bvh_node_count;
    BVHNode *left_node  = bvh_nodes + (*bvh_node_count)++;
    BVHNode *right_node = bvh_nodes + (*bvh_node_count)++;

    initBVHNode(left_node);
    initBVHNode(right_node);

    f32 smallest_surface_area = INFINITY;
    PartitionAxis *chosen_partition_axis = builder->partition_axis, *pa = builder->partition_axis;

    u32 byte_count = sizeof(u32) * N;
    for (u8 axis = 0; axis < 3; axis++, pa++) {
        memcpy(pa->sorted_leaf_ids, leaf_ids, byte_count);

        // Partition the BVH's primitive-ids for the current partition axis:
        partitionBVHNodeIDs(pa, axis, nodes, N);

        // Choose the current partition axis if it's smallest surface area is smallest so far:
        if (pa->smallest_surface_area < smallest_surface_area) {
            smallest_surface_area = pa->smallest_surface_area;
            chosen_partition_axis = pa;
        }
    }

    u32 left_count = chosen_partition_axis->left_node_count;
    left_node->aabb  = chosen_partition_axis->left.aabbs[left_count-1];
    right_node->aabb = chosen_partition_axis->right.aabbs[left_count];

    memcpy(leaf_ids, chosen_partition_axis->sorted_leaf_ids, byte_count);

    return start + left_count;
}

void buildBVH(BVH *bvh, BVHBuilder *builder, u32 N, u32 max_leaf_size) {
    bvh->node_count = 1;
    initBVHNode(bvh->nodes);

    if (N <= max_leaf_size) {
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

    leaf_nodes = builder->leaf_nodes;
    u32 middle = splitBVHNode(bvh->nodes, &bvh->node_count, builder, bvh->nodes, 0, N);
    BuildIteration *stack = builder->build_iterations;
    u32 *leaf_id;

    stack[0].start = 0;
    stack[0].end = middle;
    stack[0].node_id = 1;

    stack[1].start = middle;
    stack[1].end = N;
    stack[1].node_id = 2;

    int index = 1;
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
            if (index > bvh->depth) bvh->depth = index;
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
}

void updateSceneBVH(Scene *scene, BVHBuilder *builder) {
    BVHNode *leaf_node = builder->leaf_nodes;
    Primitive *primitive  = scene->primitives;

    for (u32 i = 0; i < scene->settings.primitives; i++, primitive++, leaf_node++) {
        leaf_node->aabb = primitive->type == PrimitiveType_Mesh ?
                scene->meshes[primitive->id].aabb :
                getPrimitiveAABB(primitive);
        transformAABB(&leaf_node->aabb, primitive);
        leaf_node->first_child_id = builder->leaf_ids[i] = i;
    }

    buildBVH(&scene->bvh, builder, scene->settings.primitives, MAX_OBJS_PER_SCENE_BVH_NODE);
}