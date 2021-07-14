#pragma once

#include "../core/types.h"
#include "../core/init.h"
#include "../math/vec3.h"
#include "../math/quat.h"
#include "../scene/primitive.h"
#include "../scene/box.h"

#include "../render/SSB.h"
#include "../render/raytracer.h"
#include "../render/shaders/common.h"
#include "../render/shaders/intersection/box.h"
#include "../render/shaders/intersection/primitives.h"

Primitive getSelectedPrimitive(Scene *scene) {
    Primitive primitive;
    Selection *selection = scene->selection;
    if (scene->selection->object_type == PrimitiveType_Light) {
        primitive.type = (enum PrimitiveType)selection->object_type;
        primitive.id   = selection->object_id;
        primitive.position = scene->lights[selection->object_id].position_or_direction;
        primitive.scale = getVec3Of(scene->lights[selection->object_id].intensity / 8);
        primitive.rotation = getIdentityQuaternion();
        primitive.flags = IS_TRANSLATED | IS_SCALED;
    } else {
        primitive = *selection->primitive;
        if (primitive.type == PrimitiveType_Mesh)
            primitive.scale = mulVec3(primitive.scale, scene->meshes[primitive.id].aabb.max);
    }
    return primitive;
}

void manipulateSelection(Scene *scene, Viewport *viewport, Controls *controls) {
    Mouse *mouse = &controls->mouse;
    Camera *camera = viewport->camera;
    Dimensions *dimensions = &viewport->frame_buffer->dimensions;
    Selection *selection = scene->selection;
    Trace *trace = &viewport->trace;
    Light *light = scene->lights;

    setViewportProjectionPlane(viewport);

    vec3 position;
    vec3 *cam_pos = &camera->transform.position;
    mat3 *rot     = &camera->transform.rotation_matrix;
    mat3 *inv_rot = &camera->transform.rotation_matrix_inverted;
    RayHit *hit = &trace->closest_hit;
    Ray ray, *local_ray = &trace->local_space_ray;
    Primitive primitive;

    selection->transformed = false;

    if (mouse->left_button.is_pressed) {
        if (!mouse->left_button.is_handled) { // This is the first frame after the left mouse button went down:
            mouse->left_button.is_handled = true;

            // Cast a ray onto the scene to find the closest object behind the hovered pixel:
            setRayFromCoords(&ray, mouse->pos, viewport);

            ray.direction_reciprocal = oneOverVec3(ray.direction);
            trace->closest_hit.distance = trace->closest_hit.distance_squared = MAX_DISTANCE;

            bool found = hitPrimitives(&ray,
                                       trace,
                                       scene,
                                       scene->bvh.leaf_ids,
                                       scene->settings.primitives,
                                       false,
                                       true,
                                       mouse->pos.x,
                                       mouse->pos.y);
            if (light) {
                for (u32 i = 0; i < scene->settings.lights; i++, light++) {
                    f32 light_radius = light->intensity / 8;
                    trace->sphere_hit.furthest = hit->distance / light_radius;
                    if (hitSphereSimple(ray.origin,
                                        ray.direction,
                                        light->position_or_direction,
                                        1.0f / light_radius,
                                        &trace->sphere_hit)) {
                        hit->distance = trace->sphere_hit.t_near * light_radius;
                        hit->position = scaleAddVec3(ray.direction, hit->distance, ray.origin);
                        hit->object_type = PrimitiveType_Light;
                        hit->object_id = i;
                        found = true;
                    }
                }
            }

            if (found) {
                // Detect if object scene->selection has changed:
                selection->changed = (
                        selection->object_type != hit->object_type ||
                        selection->object_id   != hit->object_id
                );

                // Track the object that is now selected:
                selection->object_type = hit->object_type;
                selection->object_id   = hit->object_id;

                // Capture a pointer to the selected object's position for later use in transformations:
                if (selection->object_type == PrimitiveType_Light) {
                    selection->primitive = null;
                    selection->world_position = &scene->lights[selection->object_id].position_or_direction;
                } else {
                    selection->primitive = scene->primitives + selection->object_id;
                    selection->world_position = &selection->primitive->position;
                }
                selection->transformation_plane_origin = hit->position;
                selection->world_offset = subVec3(hit->position, *selection->world_position);

                // Track how far away the hit position is from the camera along the z axis:
                position = mulVec3Mat3(subVec3(hit->position, ray.origin), *inv_rot);
                selection->object_distance = position.z;
            } else {
                if (selection->object_type)
                    selection->changed = true;
                selection->object_type = PrimitiveType_None;
                selection->primitive = null;
            }
        }
    }

    if (selection->object_type) {
        if (controls->is_pressed.alt) {
            if (mouse->left_button.is_pressed ||
                mouse->middle_button.is_pressed ||
                mouse->right_button.is_pressed) {
                if (!selection->box_side) {
                    // Cast a ray onto the bounding box of the currently selected object:
                    setRayFromCoords(&ray, mouse->pos, viewport);

                    primitive = getSelectedPrimitive(scene);

                    convertPositionAndDirectionToObjectSpace(ray.origin, ray.direction, &primitive, &local_ray->origin, &local_ray->direction);
                    selection->box_side = hitBox(hit, &local_ray->origin, &local_ray->direction, ALL_FLAGS);
                    if (selection->box_side) {
                        selection->transformation_plane_center = convertPositionToWorldSpace(hit->normal,   &primitive);
                        selection->transformation_plane_origin = convertPositionToWorldSpace(hit->position, &primitive);
                        selection->transformation_plane_normal = convertDirectionToWorldSpace(hit->normal,  &primitive);
                        selection->transformation_plane_normal = normVec3(selection->transformation_plane_normal);
                        selection->world_offset = subVec3(selection->transformation_plane_origin, *selection->world_position);
                        if (selection->object_type == PrimitiveType_Light) {
                            selection->object_scale = getVec3Of(scene->lights[selection->object_id].intensity / 8);
                            selection->object_rotation = getIdentityQuaternion();
                        } else {
                            selection->object_scale    = selection->primitive->scale;
                            selection->object_rotation = selection->primitive->rotation;
                        }
                    }
                }
                selection->transformed = true;
                setRayFromCoords(&ray, mouse->pos, viewport);
                if (hitPlane(selection->transformation_plane_origin,
                             selection->transformation_plane_normal,
                             &ray.origin,
                             &ray.direction,
                             hit)) {

                    primitive = getSelectedPrimitive(scene);

                    if (mouse->left_button.is_pressed) {
                        position = subVec3(hit->position, selection->world_offset);
                        *selection->world_position = position;
                        if (selection->primitive)
                            selection->primitive->flags |= IS_TRANSLATED;
                    } else if (mouse->middle_button.is_pressed) {
                        position      = selection->transformation_plane_origin;
                        position      = convertPositionToObjectSpace(     position, &primitive);
                        hit->position = convertPositionToObjectSpace(hit->position, &primitive);
                        position = mulVec3(hit->position, oneOverVec3(position));
                        if (selection->primitive) {
                            selection->primitive->scale = mulVec3(selection->object_scale, position);
                            selection->primitive->flags |= IS_SCALED | IS_SCALED_NON_UNIFORMLY;
                        } else {
                            if (selection->box_side == Top || selection->box_side == Bottom)
                                position.x = position.z > 0 ? position.z : -position.z;
                            else
                                position.x = position.y > 0 ? position.y : -position.y;
                            scene->lights[selection->object_id].intensity = (selection->object_scale.x * 8) * position.x;
                        }

                    } else if (mouse->right_button.is_pressed && selection->primitive) {
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
                    if (selection->primitive)
                        updatePrimitiveSSB(scene, viewport, selection->primitive);
                }
            } else
                selection->box_side = NoSide;
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
                if (selection->primitive) {
                    selection->primitive->flags |= IS_TRANSLATED;
                    updatePrimitiveSSB(scene, viewport, selection->primitive);
                }

                selection->transformed = true;
            }
        }
    }

    if (selection->transformed) {
        if (selection->object_type == PrimitiveType_Light)
            uploadLights(scene);
        else
            updateScene(scene, viewport);
    }
}

void drawSelection(Scene *scene, Viewport *viewport, Controls *controls) {
    Mouse *mouse = &controls->mouse;
    Selection *selection = scene->selection;
    Box *box = &selection->box;

    if (controls->is_pressed.alt &&
        !mouse->is_captured &&
        selection->object_type) {
        Primitive primitive = getSelectedPrimitive(scene);

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