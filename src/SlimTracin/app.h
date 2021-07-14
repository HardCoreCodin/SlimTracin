#pragma once

#include "./core/init.h"
#include "./scene/io.h"
#include "./render/SSB.h"
#include "./render/acceleration_structures/builder_top_down.h"

typedef struct App {
    Memory memory;
    Platform platform;
    Controls controls;
    PixelGrid window_content;
    AppCallbacks on;
    Time time;
    Scene scene;
    Viewport viewport;
    BVHBuilder bvh_builder;
    bool is_running;
    void *user_data;
} App;

App *app;

void initApp(Defaults *defaults);

void _windowRedraw() {
    if (!app->is_running) return;
    if (app->on.windowRedraw) app->on.windowRedraw();
}

void _windowResize(u16 width, u16 height) {
    if (!app->is_running) return;
    updateDimensions(&app->window_content.dimensions, width, height);
    updateSceneSSB(&app->scene, &app->viewport);

    if (app->on.windowResize) app->on.windowResize(width, height);
    if (app->on.windowRedraw) app->on.windowRedraw();
}

void _keyChanged(u8 key, bool pressed) {
         if (key == app->controls.key_map.ctrl)  app->controls.is_pressed.ctrl  = pressed;
    else if (key == app->controls.key_map.alt)   app->controls.is_pressed.alt   = pressed;
    else if (key == app->controls.key_map.shift) app->controls.is_pressed.shift = pressed;
    else if (key == app->controls.key_map.space) app->controls.is_pressed.space = pressed;
    else if (key == app->controls.key_map.tab)   app->controls.is_pressed.tab   = pressed;

    if (app->on.keyChanged) app->on.keyChanged(key, pressed);
}

void _mouseButtonDown(MouseButton *mouse_button, i32 x, i32 y) {
    mouse_button->is_pressed = true;
    mouse_button->is_handled = false;

    mouse_button->down_pos.x = x;
    mouse_button->down_pos.y = y;

    if (app->on.mouseButtonDown) app->on.mouseButtonDown(mouse_button);
}

void _mouseButtonUp(MouseButton *mouse_button, i32 x, i32 y) {
    mouse_button->is_pressed = false;
    mouse_button->is_handled = false;

    mouse_button->up_pos.x = x;
    mouse_button->up_pos.y = y;

    if (app->on.mouseButtonUp) app->on.mouseButtonUp(mouse_button);
}

void _mouseButtonDoubleClicked(MouseButton *mouse_button, i32 x, i32 y) {
    app->controls.mouse.double_clicked = true;
    mouse_button->double_click_pos.x = x;
    mouse_button->double_click_pos.y = y;
    if (app->on.mouseButtonDoubleClicked) app->on.mouseButtonDoubleClicked(mouse_button);
}

void _mouseWheelScrolled(f32 amount) {
    app->controls.mouse.wheel_scroll_amount += amount * 100;
    app->controls.mouse.wheel_scrolled = true;

    if (app->on.mouseWheelScrolled) app->on.mouseWheelScrolled(amount);
}

void _mousePositionSet(i32 x, i32 y) {
    app->controls.mouse.pos.x = x;
    app->controls.mouse.pos.y = y;

    if (app->on.mousePositionSet) app->on.mousePositionSet(x, y);
}

void _mouseMovementSet(i32 x, i32 y) {
    app->controls.mouse.movement.x = x - app->controls.mouse.pos.x;
    app->controls.mouse.movement.y = y - app->controls.mouse.pos.y;
    app->controls.mouse.moved = true;

    if (app->on.mouseMovementSet) app->on.mouseMovementSet(x, y);
}

void _mouseRawMovementSet(i32 x, i32 y) {
    app->controls.mouse.pos_raw_diff.x += x;
    app->controls.mouse.pos_raw_diff.y += y;
    app->controls.mouse.moved = true;

    if (app->on.mouseRawMovementSet) app->on.mouseRawMovementSet(x, y);
}

bool initAppMemory(u64 size) {
    if (app->memory.address) return false;

    void* memory_address = app->platform.getMemory(size);
    if (!memory_address) {
        app->is_running = false;
        return false;
    }

    initMemory(&app->memory, (u8*)memory_address, size);
    return true;
}

void* allocateAppMemory(u64 size) {
    void *new_memory = allocateMemory(&app->memory, size);
    if (new_memory) return new_memory;

    app->is_running = false;
    return null;
}
void initScene(Scene *scene, SceneSettings *settings, Memory *memory, Platform *platform) {
    scene->settings = *settings;
    scene->primitives   = null;
    scene->materials    = null;
    scene->lights       = null;
    scene->area_lights  = null;
    scene->cameras      = null;
    scene->meshes       = null;
    scene->mesh_triangle_counts = null;
    scene->mesh_bvh_node_counts = null;

    scene->selection = (Selection*)allocateMemory(memory, sizeof(Selection));
    scene->selection->object_type = scene->selection->object_id = 0;
    scene->selection->changed = false;

    if (settings->meshes && settings->mesh_files) {
        scene->meshes               = (Mesh*)allocateMemory(memory, sizeof(Mesh) * settings->meshes);
        scene->mesh_bvh_node_counts = (u32* )allocateMemory(memory, sizeof(u32)  * settings->meshes);
        scene->mesh_triangle_counts = (u32* )allocateMemory(memory, sizeof(u32)  * settings->meshes);
        for (u32 i = 0; i < settings->meshes; i++) {
            loadMeshFromFile(&scene->meshes[i], settings->mesh_files[i].char_ptr, platform, memory);
            scene->mesh_bvh_node_counts[i] = scene->meshes[i].bvh.node_count;
            scene->mesh_triangle_counts[i] = scene->meshes[i].triangle_count;
        }
    }
    if (settings->materials)   {
        Material *material = scene->materials = (Material*)allocateMemory(memory, sizeof(Material) * settings->materials);
        for (u32 i = 0; i < settings->materials; i++, material++) {
            material->specular   = getVec3Of(1);
            material->emission   = getVec3Of(1);
            material->diffuse    = getVec3Of(1);
            material->roughness  = 1;
            material->shininess  = 1;
            material->n1_over_n2 = 1;
            material->n2_over_n1 = 1;
            material->flags      = LAMBERT;
        }
    }
    if (settings->lights) {
        Light *light = scene->lights = (Light*)allocateMemory(memory, sizeof(Light) * settings->lights);
        for (u32 i = 0; i < settings->lights; i++, light++) {
            light->position_or_direction = getVec3Of(0);
            light->color                 = getVec3Of(1);
            light->attenuation           = getVec3Of(1);
            light->intensity = 1;
            light->is_directional = false;
        }
    }

    if (settings->area_lights) scene->area_lights  = (AreaLight* )allocateMemory(memory, sizeof(AreaLight) * settings->area_lights);

    if (settings->cameras) {
        scene->cameras = (Camera*)allocateMemory(memory, sizeof(Camera) * settings->cameras);
        if (scene->cameras)
            for (u32 i = 0; i < settings->cameras; i++)
                initCamera(scene->cameras + i);
    }

    if (settings->primitives) {
        scene->primitives = (Primitive*)allocateMemory(memory, sizeof(Primitive) * settings->primitives);
        if (scene->primitives)
            for (u32 i = 0; i < settings->primitives; i++) {
                initPrimitive(scene->primitives + i);
                scene->primitives[i].id = i;
            }
    }

    scene->last_io_ticks = platform->getTicks();
    scene->last_io_is_save = false;

    initBVH(&scene->bvh, settings->primitives, memory);

    allocateDeviceScene(scene);
}

void _initApp(Defaults *defaults, void* window_content_memory) {
    app->is_running = true;
    app->user_data = null;
    app->memory.address = null;

    app->on.sceneReady = null;
    app->on.viewportReady = null;
    app->on.windowRedraw = null;
    app->on.windowResize = null;
    app->on.keyChanged = null;
    app->on.mouseButtonUp = null;
    app->on.mouseButtonDown = null;
    app->on.mouseButtonDoubleClicked = null;
    app->on.mouseWheelScrolled = null;
    app->on.mousePositionSet = null;
    app->on.mouseMovementSet = null;
    app->on.mouseRawMovementSet = null;

    PixelGrid *frame_buffer = &app->window_content;
    Platform *platform = &app->platform;
    Viewport *viewport = &app->viewport;
    SceneSettings *scene_settings = &defaults->settings.scene;
    ViewportSettings *viewport_settings = &defaults->settings.viewport;
    NavigationSettings *navigation_settings = &defaults->settings.navigation;
    Memory *memory = &app->memory;
    Scene *scene = &app->scene;
    BVHBuilder *builder = &app->bvh_builder;

    initTime(&app->time, platform->getTicks, platform->ticks_per_second);
    initMouse(&app->controls.mouse);
    initPixelGrid(frame_buffer, (Pixel*)window_content_memory);

    defaults->title = (char*)"";
    defaults->width = 480;
    defaults->height = 360;
    defaults->additional_memory_size = 0;

    setDefaultSceneSettings(scene_settings);
    setDefaultViewportSettings(viewport_settings);
    setDefaultNavigationSettings(navigation_settings);

    initApp(defaults);

    u64 memory_size = sizeof(Selection) + defaults->additional_memory_size;
    memory_size += scene_settings->primitives * sizeof(Primitive);
    memory_size += scene_settings->primitives * sizeof(Rect);
    memory_size += scene_settings->primitives * sizeof(vec3);
    memory_size += scene_settings->meshes     * sizeof(Mesh);
    memory_size += scene_settings->meshes     * sizeof(u32) * 2;
    memory_size += scene_settings->cameras    * sizeof(Camera);
    memory_size += scene_settings->materials  * sizeof(Material);
    memory_size += scene_settings->area_lights * sizeof(AreaLight);
    memory_size += scene_settings->lights * sizeof(Light);
    memory_size += viewport_settings->hud_line_count * sizeof(HUDLine);
    u32 max_triangle_count = 0;
    u32 max_bvh_depth = 0;
    if (scene_settings->meshes && scene_settings->mesh_files) {
        Mesh mesh;
        for (u32 i = 0; i < scene_settings->meshes; i++) {
            memory_size += getMeshMemorySize(&mesh, scene_settings->mesh_files[i].char_ptr, &app->platform);
            if (mesh.triangle_count > max_triangle_count) max_triangle_count = mesh.triangle_count;
            if (mesh.bvh.depth > max_bvh_depth) max_bvh_depth = mesh.bvh.depth;
        }
    }
    u32 max_leaf_count = scene_settings->primitives > max_triangle_count ? scene_settings->primitives : max_triangle_count;
    memory_size += getBVHMemorySize(scene_settings->primitives);
    memory_size += getBVHBuilderMemorySize(&app->scene, max_leaf_count);
    memory_size += sizeof(u32) * (scene_settings->primitives + max_bvh_depth + 2); // Trace stack sizes

    initAppMemory(memory_size);
    initScene(scene, scene_settings, memory, &app->platform);

    if (app->on.sceneReady) app->on.sceneReady(scene);

    initBVHBuilder(builder, scene, memory);
    uploadScene(scene);
    updateSceneBVH(scene, builder);
    uploadMeshBVHs(scene);

    initTrace(&viewport->trace, &app->scene, &app->memory);

    if (viewport_settings->hud_line_count)
        viewport_settings->hud_lines = (HUDLine*)allocateAppMemory(viewport_settings->hud_line_count * sizeof(HUDLine));

    initViewport(viewport,
                 viewport_settings,
                 navigation_settings,
                 scene->cameras,
                 frame_buffer);
    if (app->on.viewportReady) app->on.viewportReady(viewport);
}

#ifdef __linux__
//linux code goes here
#elif _WIN32
#include "./platforms/win32.h"
#endif