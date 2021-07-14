#pragma once

#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "aabb.h"

typedef struct {
    BVHNode node;
    u32 depth, parent_id;
    bool is_first_child, is_collapsed;
} BottomUpBvhBuilderNode;

typedef struct {
    AABB overall_aabb;
    u32 primitive_count,
        max_primitives_per_leaf;
    u32 *ids;
    u64 *z_curve_distances;
    Obj *objects;
    vec3d *centroids;
    BottomUpBvhBuilderNode *nodes;
} BottomUpBvhBuilder;

#define isLeaf(  builder_node) ((builder_node)->node.primitive_count != 0)
#define isParent(builder_node) ((builder_node)->node.primitive_count == 0)

#define TRAVERSAL_COST 1
#define SEARCH_RADIUS 14

inline u64 spreadOut64Bits(u64 a) {
    u64 x = a & 0x1fffff;
    x = (x | x << 32) & 0x1f00000000ffff;
    x = (x | x << 16) & 0x1f0000ff0000ff;
    x = (x | x << 8) & 0x100f00f00f00f00f;
    x = (x | x << 4) & 0x10c30c30c30c30c3;
    x = (x | x << 2) & 0x1249249249249249;
    return x;
}

inline u64 getZCurveDistanceOf(vec3d position) {
    return (
        spreadOut64Bits((u64)position.x) << 0 |
        spreadOut64Bits((u64)position.y) << 1 |
        spreadOut64Bits((u64)position.z) << 2
    );
}

u64 *z_curve_distances = 0;

void initBottomUpBvhBuilder(BottomUpBvhBuilder *builder, u32 max_primitive_count) {
    u32 max_node_count = max_primitive_count * 4;
    builder->objects   = Allocate(Obj,   max_primitive_count);
    builder->centroids = Allocate(vec3d, max_primitive_count);
    builder->nodes     = Allocate(BottomUpBvhBuilderNode, max_node_count * 3);
    builder->ids       = Allocate(u32,   max_primitive_count + max_node_count * MAX_PRIMITIVES_PER_LEAF);
    builder->z_curve_distances = Allocate(u64, max_primitive_count);
    builder->max_primitives_per_leaf = MAX_PRIMITIVES_PER_LEAF;
}

void freeBottomUpBvhBuilder(BottomUpBvhBuilder *builder) {
    if (builder->ids)       free(builder->ids);
    if (builder->nodes)     free(builder->nodes);
    if (builder->objects)   free(builder->objects);
    if (builder->centroids) free(builder->centroids);
    if (builder->z_curve_distances) free(builder->z_curve_distances);
}

int sortCmpZCurveDistances(const void *lhs, const void *rhs) {
    u64 l = z_curve_distances[*((u32*)(lhs))];
    u64 r = z_curve_distances[*((u32*)(rhs))];
    return l < r ? -1 : (l > r ? 1 : 0);
}

void buildBvhBottomUp(BVH *bvh, BottomUpBvhBuilder *builder) {
    // The build process goes through several distinct steps (all happening here in sequence):
    // =======================================================================================
    // 1) Generate an array of primitive indices, sorted based on their relative Z-curve distances:
    //    a: Initialize an array of ids for the primitives (0 -> N-1).
    //    b: For each primitive, use it's centroid to compute a (relative)distance on a 1D Z-curve (that goes in 3D).
    //    c: Sort the array of primitive indices based on their respective relative distance on the Z-curve.
    // 2) Initialize an array of bvh (leaf)nodes for each primitive, ordered based on the sorted array of indices.
    // 3) Starting from the initial leaf nodes, create parent nodes iteratively until there's only one left (the root).
    //    At each iteration:
    //    a: Scan through the remaining nodes, looking for optimal neighboring pairs to create parents for.
    //    b: Scan through the pairs of nodes found, creating parents for them (all leaf nodes still having 1 primitive).
    //    c: Parent nodes are put in a 'next iteration' array so at the end of an iteration swap the current/next arrays.
    // 4) Collapse leaf nodes up-to their parents (leaving holes in the array) so that they represent multiple primitives
    // 5) Write the final node tree into the BVH itself, while packing the sparse array resulted from the leaf-collapse.

    // For each of the 3 axis, compute the multiplication factors needed to place the centroids into a 0->1 range
    // within the bounds of the overall aabb, and then mapping those up to the range of the z-order (of a single axis):
    f64 fx = (f64)(1 << 21) / ((f64)builder->overall_aabb.max.x - (f64)builder->overall_aabb.min.x);
    f64 fy = (f64)(1 << 21) / ((f64)builder->overall_aabb.max.y - (f64)builder->overall_aabb.min.y);
    f64 fz = (f64)(1 << 21) / ((f64)builder->overall_aabb.max.z - (f64)builder->overall_aabb.min.z);

    u32 i, j, begin, end, max_primitives_per_leaf = builder->max_primitives_per_leaf;
    u32 N = builder->primitive_count;
    u32 node_count = 2 * N - 1;

    // The first N entries are reserved for another purpose in a final step.
    // The subsequence range has space for the sub-ranges for primitive ids for each leaf-node (after leaf-collapsing).
    u32 *node_primitive_ids = builder->ids + N;

    // 1) Generate an array of primitive indices, sorted based on their relative Z-curve distances:
    {
        vec3d centroid;
        u32 *primitive_indices = builder->ids;
        z_curve_distances = builder->z_curve_distances;

        // For each primitive, compute a Z-curve relative distance (on a 1D Z-curve) from the position of it's centroid,
        // and initialize an array of ids for the primitives (0 -> N-1), then sort it based on their Z-curve distances:
        for (i = 0; i < N; i++) {
            // Convert the centroid into the space of the overall aabb's bounds, and then into the space of the z-curve:
            centroid.x = (builder->centroids[i].x - builder->overall_aabb.min.x) * fx;
            centroid.y = (builder->centroids[i].y - builder->overall_aabb.min.y) * fy;
            centroid.z = (builder->centroids[i].z - builder->overall_aabb.min.z) * fz;

            z_curve_distances[i] = getZCurveDistanceOf(centroid);
            primitive_indices[i] = i;
        }

        qsort(primitive_indices, N, sizeof(u32), sortCmpZCurveDistances);
    }

    // 2) Initialize an array of bvh (leaf)nodes for each primitive, ordered based on the sorted array of indices.
    {
        // In step-3 all leaf nodes always ever represent a single primitive (leaf-collapsing happens later in step-4).
        // Because of that, it's simpler to copy/move nodes around along with their (single)primitive-id(s) if they just
        // contain the actual id of the primitive they represent (as opposed to an index into an indirection array).
        // This also then frees the builder->ids array data to be reused for other purposes.

        BottomUpBvhBuilderNode *node = builder->nodes + node_count;
        for (i = 0; i < N; i++, node++) {
            j = builder->ids[i];
            *node = builder->nodes[j];

            node->node.first_child_id = j;
            node->node.primitive_count = 1;
            node->depth = 1;
            node->parent_id = 0;
        }
    }

    // 3) Starting from the initial leaf nodes, create parent nodes iteratively until there's only one left (the root).
    {
        // The same array of ids is now reused for a different purpose, so is referenced again from a different variable
        // with a more semantically-relevant name (for clarity). The new purpose is for tracking neighboring node-ids.
        u32 *neighbors = builder->ids;
        u32 insertion_index = node_count; // Start inserting nodes at the back of the array
        u32 best_candidate_index, next_nodes_count, current_nodes_count = N;
        f32 best_candidate_values, candidate_value;
        u32 right_node_id, left_node_id;
        AABB merged_aabb;

        BottomUpBvhBuilderNode *out_nodes     = builder->nodes;
        BottomUpBvhBuilderNode *current_nodes = builder->nodes + node_count;
        BottomUpBvhBuilderNode *next_nodes    = builder->nodes + node_count + node_count;
        BottomUpBvhBuilderNode *tmp_nodes, *left_node, *right_node, *parent_node;

        while (current_nodes_count > 1) {

            // a: Scan through the remaining nodes, looking for optimal neighboring pairs to create parents for:
            for (i = 0; i < current_nodes_count; ++i) {
                begin = i > SEARCH_RADIUS ? i - SEARCH_RADIUS : 0;
                end   = i + SEARCH_RADIUS + 1 < current_nodes_count ? i + SEARCH_RADIUS + 1 : current_nodes_count;

                best_candidate_index  = begin == i ? end : begin;
                best_candidate_values = INFINITY;

                left_node  = current_nodes + i;
                right_node = current_nodes + begin;
                for (j = begin; j < end; right_node++, j++) if (i != j) {
                        merged_aabb = mergeAABBs(left_node->node.aabb, right_node->node.aabb);
                        candidate_value = getSurfaceAreaOfAABB(merged_aabb);
                        if (candidate_value < best_candidate_values) {
                            best_candidate_values = candidate_value;
                            best_candidate_index = j;
                        }
                    }

                neighbors[i] = best_candidate_index;
            }

            next_nodes_count = 0;

            // b: Scan through the pairs of nodes found, creating parents for them (all leaf nodes still having 1 primitive).
            for (i = 0; i < current_nodes_count; ++i) {
                j = neighbors[i];
                if (neighbors[j] == i) { // Merge the pair of nodes if they both chose each other as their best match
                    if (i > j)           // ...unless that already happened when the other one was visited...
                        continue;

                    // Set the children in the output nodes:
                    insertion_index -= 2;
                    left_node_id = insertion_index;
                    right_node_id = left_node_id + 1;

                    left_node  = out_nodes + left_node_id;
                    right_node = out_nodes + right_node_id;

                    *left_node  = current_nodes[i];
                    *right_node = current_nodes[j];

                    // This is needed later, after collapsing leaf nodes, when packing the resulting sparse array:
                    left_node->is_first_child  = true;
                    right_node->is_first_child = false;

                    // Child nodes get placed into the output array earlier that their parents, so when they do, the final
                    // index of where their parent node will end up landing at in the output array, isn't known yet.
                    // Only when their parent node itself get placed into the output array does this gets decided upon.
                    // So if any of the "child" nodes being placed now is also a parent, it means it's child-nodes had
                    // already been placed, so their `parent_id` field can now be set to the (now-known)parent-node index:
                    if (isParent(left_node)) {
                        out_nodes[left_node->node.first_child_id + 0].parent_id = left_node_id;
                        out_nodes[left_node->node.first_child_id + 1].parent_id = left_node_id;
                    }
                    if (isParent(right_node)) {
                        out_nodes[right_node->node.first_child_id + 0].parent_id = right_node_id;
                        out_nodes[right_node->node.first_child_id + 1].parent_id = right_node_id;
                    }

                    // Put the parent node in the nodes of the next iteration, then initialize it:
                    parent_node = next_nodes + next_nodes_count++;
                    parent_node->depth = left_node->depth > right_node->depth ? left_node->depth : right_node->depth;
                    parent_node->depth++;
                    parent_node->node.primitive_count = 0;
                    parent_node->node.first_child_id = insertion_index;
                    parent_node->node.aabb = mergeAABBs(left_node->node.aabb, right_node->node.aabb);
                } else // Put the current node in the nodes of the next iteration:
                    next_nodes[next_nodes_count++] = current_nodes[i];
            }

            // Swap current/next nodes:
            tmp_nodes           = current_nodes;
            current_nodes       = next_nodes;
            current_nodes_count = next_nodes_count;
            next_nodes          = tmp_nodes;
        }

        // Finalize the root node in the output nodes array:
        out_nodes[0] = current_nodes[0];
        if (isParent(out_nodes)) {
            out_nodes[out_nodes->node.first_child_id + 0].parent_id = 0;
            out_nodes[out_nodes->node.first_child_id + 1].parent_id = 0;
        }
    }

    // 4) Collapse leaf nodes up-to their parents (leaving holes in the array) so that they represent multiple primitives
    {
        // The initial tree has now been built, having each leaf node referencing a single primitive(index).
        // The next step is leaf collapsing, where each leaf node may end-up referencing multiple primitives (indices).
        // Each leaf node would now thus need to get it's own sub-array(sub-range) space to hold it's primitive indices.
        // For that, prime each node's sub-range with the index of just the one primitive it currently references:
        u32 *primitive_id = node_primitive_ids;
        BottomUpBvhBuilderNode *out_node = builder->nodes;
        for (i = 0; i < node_count; i++, out_node++, primitive_id += max_primitives_per_leaf)
            if (isLeaf(out_node))
                *primitive_id = out_node->node.first_child_id;

        u32 Nl, Nr, Np, right_node_id, left_node_id, *parent_primitive_ids;
        BottomUpBvhBuilderNode *left_node, *right_node, *parent_node;
        f32 collapsed_cost, uncollapsed_cost, SAl, SAr, SAp;
        bool collapse_leaf_nodes = true;
        while (collapse_leaf_nodes) {
            collapse_leaf_nodes = false;

            for (i = 0, parent_node = builder->nodes; i < node_count; i++, parent_node++) {
                if (!isParent(parent_node))
                    continue;

                left_node_id = parent_node->node.first_child_id;
                left_node = builder->nodes + left_node_id;
                if (!isLeaf(left_node)) continue;

                right_node_id = left_node_id + 1;
                right_node = builder->nodes + right_node_id;
                if (!isLeaf(right_node)) continue;

                // Default the 2 child-nodes to being un-collapsed so that in the next step they won't be skipped:
                left_node->is_collapsed = right_node->is_collapsed = false;

                Nl = left_node->node.primitive_count;
                Nr = right_node->node.primitive_count;
                Np = Nr + Nl;
                if (Np > max_primitives_per_leaf)  // Don't collapse too much
                    continue;

                // Check if it's worth it to collapse this parent-node's leaf-children into itself:
                SAp = getSurfaceAreaOfAABB(parent_node->node.aabb);
                SAl = getSurfaceAreaOfAABB(left_node->node.aabb);
                SAr = getSurfaceAreaOfAABB(right_node->node.aabb);
                uncollapsed_cost = (f32)Nl * SAl + (f32)Nr * SAr;
                collapsed_cost = ((f32)Np - (f32)TRAVERSAL_COST) * SAp;
                if (collapsed_cost <= uncollapsed_cost) { // Worth collapsing - do it:
                    parent_node->node.primitive_count = Np;
                    parent_node->depth = 1;

                    // Copy over the primitive ids of the children to their parent:
                    parent_primitive_ids = node_primitive_ids + (max_primitives_per_leaf * i);

                    primitive_id = node_primitive_ids + (max_primitives_per_leaf * left_node_id);
                    for (j = 0; j < Nl; j++) *(parent_primitive_ids++) = *(primitive_id++);

                    primitive_id = node_primitive_ids + (max_primitives_per_leaf * right_node_id);
                    for (j = 0; j < Nr; j++) *(parent_primitive_ids++) = *(primitive_id++);

                    // Mark the 2 collapsed child-nodes as being collapsed so that in the next step they will be skipped:
                    left_node->is_collapsed = right_node->is_collapsed = true;

                    if (Np < max_primitives_per_leaf)
                        // This leaf node may still be able to be collapsed-up further, so keep trying:
                        collapse_leaf_nodes = true;
                }
            }
        }
    }

    // 5) Write the final node tree into the BVH itself, while packing the sparse array resulted from the leaf-collapse.
    {
        // The first N entries will now be used to hold the final primitive indices of the (final)leaf nodes.
        u32 *out_primitive_id = builder->ids;
        u32 primitive_id_insertion_index = 0;
        u32 insertion_index = 0;
        u32 *primitive_id;
        BottomUpBvhBuilderNode *node = builder->nodes;
        bvh->depth = 1;
        bvh->node_count = 0;

        for (i = 0; i < node_count; i++, node++) {
            if (isLeaf(node)) {
                if (node->is_collapsed)
                    continue;

                // Copy primitive ids over to the final (dense)array of primitive indices:
                primitive_id = node_primitive_ids + (max_primitives_per_leaf * i);
                for (j = 0; j < node->node.primitive_count; j++)
                    *(out_primitive_id++) = *(primitive_id++);

                node->node.first_child_id = primitive_id_insertion_index;
                primitive_id_insertion_index += node->node.primitive_count;
            } else { // isParent(node):
                if (node->depth > bvh->depth)
                    bvh->depth = node->depth;

                // Set 'parent_id' on this node's children to it's new-placement index:
                builder->nodes[node->node.first_child_id + 0].parent_id = insertion_index;
                builder->nodes[node->node.first_child_id + 1].parent_id = insertion_index;
            }

            // If this node is it's parent's left child, set it's parent's 'first_child_id' to it's new-placement index:
            if (i && node->is_first_child)
                bvh->nodes[node->parent_id].first_child_id = insertion_index;

            bvh->nodes[insertion_index++] = node->node;
            bvh->node_count++;
        }
    }
}

void reinsert(BottomUpBvhBuilderNode *nodes, u32 in_id, u32 out_id) {
    BottomUpBvhBuilderNode *in_node  = nodes + in_id;
    BottomUpBvhBuilderNode *out_node = nodes + out_id;

    u32 sibling_node_id = in_node->is_first_child ? in_id + 1 : in_id - 1;
    if (isParent(out_node)) {
        nodes[out_node->node.first_child_id + 0].parent_id = sibling_node_id;
        nodes[out_node->node.first_child_id + 1].parent_id = sibling_node_id;
    }

    BottomUpBvhBuilderNode *sibling_node = nodes + sibling_node_id;
    BottomUpBvhBuilderNode *parent_node  = nodes + in_node->parent_id;
    if (isParent(sibling_node)) {
        nodes[sibling_node->node.first_child_id + 0].parent_id = in_node->parent_id;
        nodes[sibling_node->node.first_child_id + 1].parent_id = in_node->parent_id;
    }
    *parent_node  = *sibling_node;
    *sibling_node = *out_node;

    // Re-insert it into the destination
    out_node->node.aabb = mergeAABBs(out_node->node.aabb, in_node->node.aabb);
    out_node->node.first_child_id = in_id < sibling_node_id ? in_id : sibling_node_id;
    out_node->node.primitive_count= 0;

    in_node->parent_id = sibling_node->parent_id = out_id;
}

void updateMeshBvhBottomUp(Mesh *mesh, MeshAccelerationStructure *mesh_acceleration_structure, BottomUpBvhBuilder *builder) {
    builder->overall_aabb.min = getVec3Of(0);
    builder->overall_aabb.max = getVec3Of(0);

    BottomUpBvhBuilderNode *node = builder->nodes;
    vec3d *centroid = builder->centroids;

    TriangleVertexIndices *indices = mesh->triangle_vertex_indices;
    vec3 *v1, *v2, *v3, min, max;
    f32 diff;

    for (u32 t = 0; t < mesh->triangle_count; t++, node++, indices++, centroid++) {
        v1 = mesh->vertex_positions + indices->ids[0];
        v2 = mesh->vertex_positions + indices->ids[1];
        v3 = mesh->vertex_positions + indices->ids[2];
        centroid->x = (v1->x + v2->x + v3->x) / 3.0;
        centroid->y = (v1->y + v2->y + v3->y) / 3.0;
        centroid->z = (v1->z + v2->z + v3->z) / 3.0;

        min = minVec3(minVec3(*v1, *v2), *v3);
        max = maxVec3(maxVec3(*v1, *v2), *v3);

        diff = max.x - min.x;
        if (diff < 0) diff = -diff;
        if (diff < EPS) {
            min.x -= EPS;
            max.x += EPS;
        }

        diff = max.y - min.y;
        if (diff < 0) diff = -diff;
        if (diff < EPS) {
            min.y -= EPS;
            max.y += EPS;
        }

        diff = max.z - min.z;
        if (diff < 0) diff = -diff;
        if (diff < EPS) {
            min.z -= EPS;
            max.z += EPS;
        }

        node->node.aabb.min = min;
        node->node.aabb.max = max;

        builder->overall_aabb = mergeAABBs(builder->overall_aabb, node->node.aabb);
    }

    Triangle *triangle = mesh_acceleration_structure->triangles;
    BVH *bvh = &mesh_acceleration_structure->bvh;
    u32 N = mesh->triangle_count;
    if (N <= MAX_TRIANGLES_PER_MESH_BVH_NODE) {
        for (u32 i = 0; i < N; i++) *(triangle++) = mesh->triangles[i];

        bvh->nodes->aabb = builder->overall_aabb;
        bvh->nodes->primitive_count = N;
        bvh->nodes->first_child_id = 0;
        bvh->depth = 1;
    } else {
        builder->primitive_count = N;
        builder->max_primitives_per_leaf = MAX_TRIANGLES_PER_MESH_BVH_NODE;
        buildBvhBottomUp(bvh, builder);

        for (u32 i = 0; i < N; i++) *(triangle++) = mesh->triangles[builder->ids[i]];
    }
}

void updateSceneBvhBottomUp(Scene *scene, AccelerationStructures *acceleration_structures, BottomUpBvhBuilder *builder) {
    builder->overall_aabb.min = getVec3Of(0);
    builder->overall_aabb.max = getVec3Of(0);

    BottomUpBvhBuilderNode *builder_node = builder->nodes;
    vec3d *centroid = builder->centroids;
    Obj *object = builder->objects;

    Mesh *mesh = scene->meshes;
    MeshAccelerationStructure *mesh_as = acceleration_structures->meshes;
    for (u32 mesh_id = 0; mesh_id < scene->counts.meshes; mesh_id++, mesh++, mesh_as++, builder_node++, object++, centroid++) {
        object->id = mesh_id;
        object->type = MeshType;

        builder_node->node.aabb = mesh_as->bvh.nodes->aabb;

        transformAABB(&builder_node->node.aabb, &mesh->primitive);
        builder->overall_aabb = mergeAABBs(builder->overall_aabb, builder_node->node.aabb);

        centroid->x = (builder_node->node.aabb.max.x - builder_node->node.aabb.min.x) / 2.0;
        centroid->y = (builder_node->node.aabb.max.y - builder_node->node.aabb.min.y) / 2.0;
        centroid->z = (builder_node->node.aabb.max.z - builder_node->node.aabb.min.z) / 2.0;
    }

    Primitive *geometry = scene->primitives;
    for (u32 sphere_id = 0; sphere_id < scene->counts.primitives; sphere_id++, geometry++, builder_node++, object++, centroid++) {
        object->id = sphere_id;
        object->type = GeoTypeSphere;

        builder_node->node.aabb.min = getVec3Of(-1);
        builder_node->node.aabb.max = getVec3Of(+1);

        transformAABB(&builder_node->node.aabb, geometry);
        builder->overall_aabb = mergeAABBs(builder->overall_aabb, builder_node->node.aabb);

        centroid->x = (f64)geometry->position.x;
        centroid->y = (f64)geometry->position.y;
        centroid->z = (f64)geometry->position.z;
    }

    object = acceleration_structures->scene.objects;
    BVH *bvh = &acceleration_structures->scene.bvh;
    u32 N = scene->counts.meshes + scene->counts.primitives;
    if (N <= MAX_OBJS_PER_SCENE_BVH_NODE) {
        bvh->nodes->aabb = builder->overall_aabb;
        bvh->nodes->primitive_count = N;
        bvh->nodes->first_child_id = 0;
        bvh->depth = 1;

        for (u32 i = 0; i < N; i++) *(object++) = builder->objects[i];
    } else {
        builder->primitive_count = N;
        builder->max_primitives_per_leaf = MAX_OBJS_PER_SCENE_BVH_NODE;
        buildBvhBottomUp(bvh, builder);

        for (u32 i = 0; i < N; i++) *(object++) = builder->objects[builder->ids[i]];
    }
}