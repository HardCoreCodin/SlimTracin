#pragma once

#include "../core/types.h"
#include "../math/vec3.h"
#include "../math/quat.h"
#include "../scene/primitive.h"
//#include "../core/time.h"
//#include "../scene/xform.h"
#include "../scene/box.h"
#include "../core/init.h"
#include "../render/shaders/common.h"
#include "../render/shaders/intersection/box.h"

INLINE bool rayHitScene(Ray *ray, RayHit *local_hit, RayHit *hit, Scene *scene) {
    bool current_found, found = false;
    vec3 Ro, Rd;
    Primitive hit_primitive, primitive;
    for (u32 i = 0; i < scene->settings.primitives; i++) {
        primitive = scene->primitives[i];
        if (primitive.type == PrimitiveType_Mesh)
            primitive.scale = mulVec3(primitive.scale, scene->meshes[primitive.id].aabb.max);

        convertPositionAndDirectionToObjectSpace(ray->origin, ray->direction, &primitive, &Ro, &Rd);

        current_found = hitBox(local_hit, &Ro, &Rd, ALL_FLAGS);
        if (current_found) {
            local_hit->position       = convertPositionToWorldSpace(local_hit->position, &primitive);
            local_hit->distance_squared = squaredLengthVec3(subVec3(local_hit->position, ray->origin));
            if (local_hit->distance_squared < hit->distance_squared) {
                *hit = *local_hit;
                hit->object_type = primitive.type;
                hit->material_id = primitive.material_id;
                hit->object_id = i;

                hit_primitive = primitive;
                found = true;
            }
        }
    }

    if (found) {
        hit->distance = sqrtf(hit->distance_squared);
        hit->normal = normVec3(convertDirectionToWorldSpace(hit->normal, &hit_primitive));
    }

    return found;
}

void manipulateSelection(Scene *scene, Viewport *viewport, Controls *controls) {
    Mouse *mouse = &controls->mouse;
    Camera *camera = viewport->camera;
    Dimensions *dimensions = &viewport->frame_buffer->dimensions;
    Selection *selection = &scene->selection;

    setViewportProjectionPlane(viewport);

    vec3 position;
    vec3 *cam_pos = &camera->transform.position;
    mat3 *rot     = &camera->transform.rotation_matrix;
    mat3 *inv_rot = &camera->transform.rotation_matrix_inverted;
    RayHit *hit = &selection->hit;
    Ray ray, *local_ray = &selection->local_ray;
    Primitive primitive;

    if (mouse->left_button.is_pressed) {
        if (!mouse->left_button.is_handled) { // This is the first frame after the left mouse button went down:
            mouse->left_button.is_handled = true;

            // Cast a ray onto the scene to find the closest object behind the hovered pixel:
            setRayFromCoords(&ray, mouse->pos, viewport);

            hit->distance_squared = INFINITY;
            if (rayHitScene(&ray, &selection->local_hit, hit, scene)) {
                // Detect if object scene->selection has changed:
                selection->changed = (
                        selection->object_type != hit->object_type ||
                        selection->object_id   != hit->object_id
                );

                // Track the object that is now selected:
                selection->object_type = hit->object_type;
                selection->object_id   = hit->object_id;
                selection->primitive   = null;

                // Capture a pointer to the selected object's position for later use in transformations:
                selection->primitive = scene->primitives + selection->object_id;
                selection->world_position = &selection->primitive->position;
                selection->transformation_plane_origin = hit->position;

                selection->world_offset = subVec3(hit->position, *selection->world_position);

                // Track how far away the hit position is from the camera along the z axis:
                position = mulVec3Mat3(subVec3(hit->position, ray.origin), *inv_rot);
                selection->object_distance = position.z;
            } else {
                if (selection->object_type)
                    selection->changed = true;
                selection->object_type = 0;
            }
        }
    }

    if (selection->object_type) {
        if (controls->is_pressed.alt) {
            bool any_mouse_button_is_pressed = (
                    mouse->left_button.is_pressed ||
                    mouse->middle_button.is_pressed ||
                    mouse->right_button.is_pressed);
            if (selection->primitive && !any_mouse_button_is_pressed) {
                // Cast a ray onto the bounding box of the currently selected object:
                setRayFromCoords(&ray, mouse->pos, viewport);
                primitive = *selection->primitive;
                if (primitive.type == PrimitiveType_Mesh)
                    primitive.scale = mulVec3(primitive.scale, scene->meshes[primitive.id].aabb.max);

                convertPositionAndDirectionToObjectSpace(ray.origin, ray.direction, &primitive, &local_ray->origin, &local_ray->direction);

                selection->box_side = hitBox(hit, &local_ray->origin, &local_ray->direction, ALL_FLAGS);
                if (selection->box_side) {
                    selection->transformation_plane_center = convertPositionToWorldSpace(hit->normal,   &primitive);
                    selection->transformation_plane_origin = convertPositionToWorldSpace(hit->position, &primitive);
                    selection->transformation_plane_normal = convertDirectionToWorldSpace(hit->normal,  &primitive);
                    selection->transformation_plane_normal = normVec3(selection->transformation_plane_normal);
                    selection->world_offset = subVec3(selection->transformation_plane_origin, *selection->world_position);
                    selection->object_scale    = selection->primitive->scale;
                    selection->object_rotation = selection->primitive->rotation;
                }
            }

            if (selection->box_side) {
                if (selection->primitive) {
                    if (any_mouse_button_is_pressed) {
                        setRayFromCoords(&ray, mouse->pos, viewport);
                        if (hitPlane(selection->transformation_plane_origin,
                                     selection->transformation_plane_normal,
                                     &ray.origin,
                                     &ray.direction,
                                     hit)) {

                            primitive = *selection->primitive;
                            if (primitive.type == PrimitiveType_Mesh)
                                primitive.scale = mulVec3(primitive.scale, scene->meshes[primitive.id].aabb.max);
                            if (mouse->left_button.is_pressed) {
                                position = subVec3(hit->position, selection->world_offset);
                                *selection->world_position = position;
                                if (selection->primitive)
                                    selection->primitive->flags |= IS_TRANSLATED;
                            } else if (mouse->middle_button.is_pressed) {
                                position      = selection->transformation_plane_origin;
                                position      = convertPositionToObjectSpace(     position, &primitive);
                                hit->position = convertPositionToObjectSpace(hit->position, &primitive);

                                selection->primitive->scale = mulVec3(selection->object_scale,
                                                                             mulVec3(hit->position, oneOverVec3(position)));
                                selection->primitive->flags |= IS_SCALED | IS_SCALED_NON_UNIFORMLY;
                            } else if (mouse->right_button.is_pressed) {
                                vec3 v1 = subVec3(hit->position,
                                                  selection->transformation_plane_center);
                                vec3 v2 = subVec3(selection->transformation_plane_origin,
                                                  selection->transformation_plane_center);
                                quat q;
                                q.axis = crossVec3(v2, v1);
                                q.amount = sqrtf(squaredLengthVec3(v1) * squaredLengthVec3(v2)) + dotVec3(v1, v2);
                                q = normQuat(q);
                                selection->primitive->rotation = mulQuat(q, selection->object_rotation);
                                selection->primitive->rotation = normQuat(selection->primitive->rotation);
                                selection->primitive->flags |= IS_ROTATED;
                            }
                        }
                    }
                }
            }
        } else {
            selection->box_side = NoSide;
            if (mouse->left_button.is_pressed && mouse->moved) {
                // Back-project the new mouse position onto a quad at a distance of the selected-object away from the camera
                position.z = selection->object_distance;

                // Screen -> NDC:
                position.x = (f32) mouse->pos.x / dimensions->h_width - 1;
                position.y = (f32) mouse->pos.y / dimensions->h_height - 1;
                position.y = -position.y;

                // NDC -> View:
                position.x *= position.z / camera->focal_length;
                position.y *= position.z / (camera->focal_length * dimensions->width_over_height);

                // View -> World:
                position = addVec3(mulVec3Mat3(position, *rot), *cam_pos);

                // Back-track by the world offset (from the hit position back to the selected-object's center):
                position = subVec3(position, selection->world_offset);
                *selection->world_position = position;
                if (selection->primitive)
                    selection->primitive->flags |= IS_TRANSLATED;
            }
        }
    }
}

void drawSelection(Scene *scene, Viewport *viewport, Controls *controls) {
    Mouse *mouse = &controls->mouse;
    Selection *selection = &scene->selection;
    Box *box = &selection->box;

    if (controls->is_pressed.alt &&
        !mouse->is_captured &&
        selection->object_type &&
        selection->primitive) {
        Primitive primitive = *selection->primitive;
        if (primitive.type == PrimitiveType_Mesh)
            primitive.scale = mulVec3(primitive.scale, scene->meshes[primitive.id].aabb.max);

        initBox(box);
        drawBox(viewport, Color(Yellow), box, &primitive, BOX__ALL_SIDES);
        if (selection->box_side) {
            RGBA color = Color(White);
            switch (selection->box_side) {
                case Left:  case Right:  color = Color(Red);   break;
                case Top:   case Bottom: color = Color(Green); break;
                case Front: case Back:   color = Color(Blue);  break;
                case NoSide: break;
            }
            drawBox(viewport, color, box, &primitive, selection->box_side);
        }
    }
}