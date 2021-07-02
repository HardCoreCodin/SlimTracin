#pragma once

#include "./types.h"
#include "../core/types.h"
#include "../math/vec3.h"
#include "../math/quat.h"
#include "../scene/primitive.h"

void initMouse(Mouse *mouse) {
    mouse->is_captured = false;

    mouse->moved = false;
    mouse->move_handled = false;

    mouse->double_clicked = false;
    mouse->double_clicked_handled = false;

    mouse->wheel_scrolled = false;
    mouse->wheel_scroll_amount = 0;
    mouse->wheel_scroll_handled = false;

    mouse->pos.x = 0;
    mouse->pos.y = 0;
    mouse->pos_raw_diff.x = 0;
    mouse->pos_raw_diff.y = 0;
    mouse->raw_movement_handled = false;

    mouse->middle_button.is_pressed = false;
    mouse->middle_button.is_handled = false;
    mouse->middle_button.up_pos.x = 0;
    mouse->middle_button.down_pos.x = 0;

    mouse->right_button.is_pressed = false;
    mouse->right_button.is_handled = false;
    mouse->right_button.up_pos.x = 0;
    mouse->right_button.down_pos.x = 0;

    mouse->left_button.is_pressed = false;
    mouse->left_button.is_handled = false;
    mouse->left_button.up_pos.x = 0;
    mouse->left_button.down_pos.x = 0;
}

void initTimer(Timer *timer, GetTicks getTicks, Ticks *ticks) {
    timer->getTicks = getTicks;
    timer->ticks    = ticks;

    timer->delta_time = 0;
    timer->ticks_before = 0;
    timer->ticks_after = 0;
    timer->ticks_diff = 0;

    timer->accumulated_ticks = 0;
    timer->accumulated_frame_count = 0;

    timer->ticks_of_last_report = 0;

    timer->seconds = 0;
    timer->milliseconds = 0;
    timer->microseconds = 0;
    timer->nanoseconds = 0;

    timer->average_frames_per_tick = 0;
    timer->average_ticks_per_frame = 0;
    timer->average_frames_per_second = 0;
    timer->average_milliseconds_per_frame = 0;
    timer->average_microseconds_per_frame = 0;
    timer->average_nanoseconds_per_frame = 0;
}

void initTime(Time *time, GetTicks getTicks, u64 ticks_per_second) {
    time->getTicks = getTicks;
    time->ticks.per_second = ticks_per_second;

    time->ticks.per_tick.seconds      = 1          / (f64)(time->ticks.per_second);
    time->ticks.per_tick.milliseconds = 1000       / (f64)(time->ticks.per_second);
    time->ticks.per_tick.microseconds = 1000000    / (f64)(time->ticks.per_second);
    time->ticks.per_tick.nanoseconds  = 1000000000 / (f64)(time->ticks.per_second);

    initTimer(&time->timers.update, getTicks, &time->ticks);
    initTimer(&time->timers.render, getTicks, &time->ticks);
    initTimer(&time->timers.aux,    getTicks, &time->ticks);

    time->timers.update.ticks_before = time->timers.update.ticks_of_last_report = getTicks();
}

void initPixelGrid(PixelGrid *pixel_grid, Pixel* pixels_memory) {
    pixel_grid->pixels = pixels_memory;
    updateDimensions(&pixel_grid->dimensions, MAX_WIDTH, MAX_HEIGHT);
}

void fillPixelGrid(PixelGrid *pixel_grid, RGBA color) {
    for (u32 i = 0; i < pixel_grid->dimensions.width_times_height; i++)
        pixel_grid->pixels[i].color = color;
}

void initBox(Box *box) {
    box->vertices.corners.front_top_left.x    = -1;
    box->vertices.corners.back_top_left.x     = -1;
    box->vertices.corners.front_bottom_left.x = -1;
    box->vertices.corners.back_bottom_left.x  = -1;

    box->vertices.corners.front_top_right.x    = 1;
    box->vertices.corners.back_top_right.x     = 1;
    box->vertices.corners.front_bottom_right.x = 1;
    box->vertices.corners.back_bottom_right.x  = 1;


    box->vertices.corners.front_bottom_left.y  = -1;
    box->vertices.corners.front_bottom_right.y = -1;
    box->vertices.corners.back_bottom_left.y   = -1;
    box->vertices.corners.back_bottom_right.y  = -1;

    box->vertices.corners.front_top_left.y  = 1;
    box->vertices.corners.front_top_right.y = 1;
    box->vertices.corners.back_top_left.y   = 1;
    box->vertices.corners.back_top_right.y  = 1;


    box->vertices.corners.front_top_left.z     = 1;
    box->vertices.corners.front_top_right.z    = 1;
    box->vertices.corners.front_bottom_left.z  = 1;
    box->vertices.corners.front_bottom_right.z = 1;

    box->vertices.corners.back_top_left.z     = -1;
    box->vertices.corners.back_top_right.z    = -1;
    box->vertices.corners.back_bottom_left.z  = -1;
    box->vertices.corners.back_bottom_right.z = -1;

    setBoxEdgesFromVertices(&box->edges, &box->vertices);
}

void initXform3(xform3 *xform) {
    mat3 I;
    I.X.x = 1; I.Y.x = 0; I.Z.x = 0;
    I.X.y = 0; I.Y.y = 1; I.Z.y = 0;
    I.X.z = 0; I.Y.z = 0; I.Z.z = 1;
    xform->matrix = xform->yaw_matrix = xform->pitch_matrix = xform->roll_matrix = xform->rotation_matrix = xform->rotation_matrix_inverted = I;
    xform->right_direction   = &xform->rotation_matrix.X;
    xform->up_direction      = &xform->rotation_matrix.Y;
    xform->forward_direction = &xform->rotation_matrix.Z;
    xform->scale.x = 1;
    xform->scale.y = 1;
    xform->scale.z = 1;
    xform->position.x = 0;
    xform->position.y = 0;
    xform->position.z = 0;
    xform->rotation.axis.x = 0;
    xform->rotation.axis.y = 0;
    xform->rotation.axis.z = 0;
    xform->rotation.amount = 1;
    xform->rotation_inverted = xform->rotation;
}

void initCamera(Camera* camera) {
    camera->focal_length = camera->zoom = CAMERA_DEFAULT__FOCAL_LENGTH;
    camera->target_distance = CAMERA_DEFAULT__TARGET_DISTANCE;
    camera->dolly = 0;
    camera->current_velocity.x = 0;
    camera->current_velocity.y = 0;
    camera->current_velocity.z = 0;
    initXform3(&camera->transform);
}

void initHUD(HUD *hud, HUDLine *lines, u32 line_count, f32 line_height, i32 position_x, i32 position_y) {
    hud->lines = lines;
    hud->line_count = line_count;
    hud->line_height = line_height;
    hud->position.x = position_x;
    hud->position.y = position_y;

    if (lines)
        for (u32 i = 0; i < line_count; i++)
            lines[i].value_color = lines[i].title_color = White;
}

void setDefaultNavigationSettings(NavigationSettings *settings) {
    settings->max_velocity  = NAVIGATION_DEFAULT__MAX_VELOCITY;
    settings->acceleration  = NAVIGATION_DEFAULT__ACCELERATION;
    settings->speeds.turn   = NAVIGATION_SPEED_DEFAULT__TURN;
    settings->speeds.orient = NAVIGATION_SPEED_DEFAULT__ORIENT;
    settings->speeds.orbit  = NAVIGATION_SPEED_DEFAULT__ORBIT;
    settings->speeds.zoom   = NAVIGATION_SPEED_DEFAULT__ZOOM;
    settings->speeds.dolly  = NAVIGATION_SPEED_DEFAULT__DOLLY;
    settings->speeds.pan    = NAVIGATION_SPEED_DEFAULT__PAN;
}

void initNavigation(Navigation *navigation, NavigationSettings *navigation_settings) {
    navigation->settings = *navigation_settings;

    navigation->turned = false;
    navigation->moved = false;
    navigation->zoomed = false;

    navigation->move.right = false;
    navigation->move.left = false;
    navigation->move.up = false;
    navigation->move.down = false;
    navigation->move.forward = false;
    navigation->move.backward = false;

    navigation->turn.right = false;
    navigation->turn.left = false;
}

void setDefaultViewportSettings(ViewportSettings *settings) {
    settings->near_clipping_plane_distance = VIEWPORT_DEFAULT__NEAR_CLIPPING_PLANE_DISTANCE;
    settings->far_clipping_plane_distance  = VIEWPORT_DEFAULT__FAR_CLIPPING_PLANE_DISTANCE;
    settings->hud_line_count = 0;
    settings->hud_lines = null;
    settings->show_hud = false;
    settings->show_BVH = false;
    settings->show_SSB = false;
    settings->use_GPU  = false;
    settings->render_mode = RenderMode_Beauty;
}

void initViewport(Viewport *viewport,
                  ViewportSettings *viewport_settings,
                  NavigationSettings *navigation_settings,
                  Camera *camera,
                  PixelGrid *frame_buffer) {
    viewport->camera = camera;
    viewport->settings = *viewport_settings;
    viewport->frame_buffer = frame_buffer;
    initBox(&viewport->default_box);
    initHUD(&viewport->hud, viewport_settings->hud_lines, viewport_settings->hud_line_count, 1, 0, 0);
    initNavigation(&viewport->navigation, navigation_settings);
}

void setDefaultSceneSettings(SceneSettings *settings) {
    settings->cameras = 1;
    settings->primitives = 0;
    settings->materials = 0;
    settings->point_lights = 0;
    settings->quad_lights = 0;
    settings->meshes = 0;
    settings->curves = 0;
    settings->boxes = 0;
    settings->grids = 0;
    settings->mesh_files = null;
    settings->file.char_ptr = null;
    settings->file.length = 0;
}

void initCurve(Curve *curve) {
    curve->thickness = 0.1f;
    curve->revolution_count = 1;
}

void initPrimitive(Primitive *primitive) {
    primitive->id = 0;
    primitive->type = PrimitiveType_None;
    primitive->color = White;
    primitive->flags = ALL_FLAGS;
    primitive->scale.x = 1;
    primitive->scale.y = 1;
    primitive->scale.z = 1;
    primitive->position.x = 0;
    primitive->position.y = 0;
    primitive->position.z = 0;
    primitive->rotation.axis.x = 0;
    primitive->rotation.axis.y = 0;
    primitive->rotation.axis.z = 0;
    primitive->rotation.amount = 1;
}

bool initGrid(Grid *grid, u8 u_segments, u8 v_segments) {
    if (!u_segments || u_segments > GRID__MAX_SEGMENTS ||
        !v_segments || v_segments > GRID__MAX_SEGMENTS)
        return false;

    grid->u_segments = u_segments;
    grid->v_segments = v_segments;

    f32 u_step = u_segments > 1 ? (2.0f / (u_segments - 1)) : 0;
    f32 v_step = v_segments > 1 ? (2.0f / (v_segments - 1)) : 0;

    for (u8 u = 0; u < grid->u_segments; u++) {
        grid->vertices.uv.u.from[u].y = grid->vertices.uv.u.to[u].y = 0;
        grid->vertices.uv.u.from[u].x = grid->vertices.uv.u.to[u].x = -1 + u * u_step;
        grid->vertices.uv.u.from[u].z = -1;
        grid->vertices.uv.u.to[  u].z = +1;
    }
    for (u8 v = 0; v < grid->v_segments; v++) {
        grid->vertices.uv.v.from[v].y = grid->vertices.uv.v.to[v].y = 0;
        grid->vertices.uv.v.from[v].z = grid->vertices.uv.v.to[v].z = -1 + v * v_step;
        grid->vertices.uv.v.from[v].x = -1;
        grid->vertices.uv.v.to[  v].x = +1;
    }

    setGridEdgesFromVertices(grid->edges.uv.u, grid->u_segments, grid->vertices.uv.u.from, grid->vertices.uv.u.to);
    setGridEdgesFromVertices(grid->edges.uv.v, grid->v_segments, grid->vertices.uv.v.from, grid->vertices.uv.v.to);

    return true;
}

void initBVHNode(BVHNode *node) {
    node->aabb.min.x = node->aabb.min.y = node->aabb.min.z = 0;
    node->aabb.max.x = node->aabb.max.y = node->aabb.max.z = 0;
    node->first_child_id = 0;
    node->primitive_count = 0;
}

void initBVH(BVH *bvh, u32 leaf_count, Memory *memory) {
    bvh->nodes    = allocateMemory(memory, sizeof(BVHNode) * leaf_count * 2);
    bvh->leaf_ids = allocateMemory(memory, sizeof(u32) * leaf_count);
}

u32 getBVHMemorySize(u32 leaf_count) {
    return leaf_count * (sizeof(u32) + sizeof(BVHNode) * 2);
}

void initTrace(Trace *trace, Scene *scene, Memory *memory) {
//    trace->quad_light_hits = scene->settings.quad_lights ?
//            allocateMemory(memory, sizeof(RayHit) * scene->settings.quad_lights) : null;

    if (scene->settings.meshes) {
        trace->closest_mesh_hit.object_type = PrimitiveType_Mesh;

        u32 max_depth = 0;
        for (u32 m = 0; m < scene->settings.meshes; m++)
            if (scene->meshes[m].bvh.depth > max_depth)
                max_depth = scene->meshes[m].bvh.depth;
        trace->mesh_stack = allocateMemory(memory, sizeof(u32) * (max_depth + 2));
    } else
        trace->mesh_stack = null;

    trace->scene_stack = allocateMemory(memory, sizeof(u32) * scene->settings.primitives);
    trace->depth = 2;
}

INLINE void prePrepRay(Ray *ray) {
    ray->octant.x = signbit(ray->direction.x) ? 3 : 0;
    ray->octant.y = signbit(ray->direction.y) ? 3 : 0;
    ray->octant.z = signbit(ray->direction.z) ? 3 : 0;
    ray->scaled_origin.x = -ray->origin.x * ray->direction_reciprocal.x;
    ray->scaled_origin.y = -ray->origin.y * ray->direction_reciprocal.y;
    ray->scaled_origin.z = -ray->origin.z * ray->direction_reciprocal.z;
}

void initSelection(Selection *selection) {
    selection->object_type = selection->object_id = 0;
    selection->changed = false;
}

void setViewportProjectionPlane(Viewport *viewport) {
    Camera *camera = viewport->camera;
    ProjectionPlane *projection_plane = &viewport->projection_plane;
    Dimensions *dimensions = &viewport->frame_buffer->dimensions;

    projection_plane->start = scaleVec3(*camera->transform.forward_direction, (f32)dimensions->width * camera->focal_length);
    projection_plane->right = scaleVec3(*camera->transform.right_direction, 1.0f - (f32)dimensions->width);
    projection_plane->down  = scaleVec3(*camera->transform.up_direction, (f32)dimensions->height - 2);
    projection_plane->start = addVec3(projection_plane->start, projection_plane->right);
    projection_plane->start = addVec3(projection_plane->start, projection_plane->down);

    projection_plane->right = scaleVec3(*camera->transform.right_direction, 2);
    projection_plane->down  = scaleVec3(*camera->transform.up_direction, -2);
}

void uploadPrimitives(Scene *scene);
void uploadLights(Scene *scene);
void uploadScene(Scene *scene);
void uploadSSB(Scene *scene);
void uploadMeshBVHs(Scene *scene);
void uploadSceneBVH(Scene *scene);
void allocateDeviceAccelerationStructures();
void allocateDeviceScene(Scene *scene);
void freeAllDeviceResources(SceneSettings *scene_counts);
void renderOnGPU(Scene *scene, Viewport *viewport);