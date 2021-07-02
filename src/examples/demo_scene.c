#include "../SlimRayTracer/app.h"
#include "../SlimRayTracer/core/time.h"
#include "../SlimRayTracer/scene/io.h"
#include "../SlimRayTracer/scene/grid.h"
#include "../SlimRayTracer/scene/mesh.h"
#include "../SlimRayTracer/scene/curve.h"
#include "../SlimRayTracer/viewport/hud.h"
#include "../SlimRayTracer/viewport/navigation.h"
#include "../SlimRayTracer/viewport/manipulation.h"

// Or using the single-header file:
// #include "../SlimRayTracer.h"

#define POINT_LIGHT_COUNT 3
#define MATERIAL_COUNT 12
#define PRIMITIVE_COUNT 18

#include "../SlimRayTracer/render/raytracer.h"

void uploadLights(Scene *scene) {}
void uploadPrimitives(Scene *scene) {}
void uploadScene(Scene *scene) {}
void uploadSSB(Scene *scene) {}
void renderOnGPU(Scene *scene, Viewport *viewport) {}
void allocateDeviceScene(Scene *scene) {}
void uploadMeshBVHs(Scene *scene) {}
void uploadSceneBVH(Scene *scene) {}

u64 scene_io_time;
bool last_scene_io_is_save;

void setCountersInHUD(HUD *hud, Timer *timer) {
    printNumberIntoString(timer->average_frames_per_second,      &hud->lines[0].value);
    printNumberIntoString(timer->average_microseconds_per_frame, &hud->lines[1].value);
}
void onButtonDown(MouseButton *mouse_button) {
    app->controls.mouse.pos_raw_diff.x = 0;
    app->controls.mouse.pos_raw_diff.y = 0;
}
void onDoubleClick(MouseButton *mouse_button) {
    if (mouse_button == &app->controls.mouse.left_button) {
        app->controls.mouse.is_captured = !app->controls.mouse.is_captured;
        app->platform.setCursorVisibility(!app->controls.mouse.is_captured);
        app->platform.setWindowCapture(    app->controls.mouse.is_captured);
        onButtonDown(mouse_button);
    }
}
void drawSceneToViewport(Scene *scene, Viewport *viewport) {
    fillPixelGrid(viewport->frame_buffer, Color(Black));
    rayTrace(scene, viewport);
    if (viewport->settings.show_BVH) drawBVH(scene, viewport);
    if (viewport->settings.show_SSB) drawSSB(scene, viewport);
}
void setupViewport(Viewport *viewport) {
    HUD *hud = &viewport->hud;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    setCountersInHUD(hud, &app->time.timers.update);
    printNumberIntoString(1, &hud->lines[2].value);
    setString(&hud->lines[0].title, "Fps    : ");
    setString(&hud->lines[1].title, "mic-s/f: ");
    setString(&hud->lines[2].title, "bounces: ");
    setString(&hud->lines[3].title, "matr-id: ");
    hud->lines[0].title_color = hud->lines[1].title_color = hud->lines[2].title_color = hud->lines[3].title_color = Green;
    hud->lines[0].value_color = hud->lines[1].value_color = hud->lines[2].value_color = hud->lines[3].value_color = Green;

    xform3 *camera_xform = &viewport->camera->transform;
    camera_xform->position.y = 7;
    camera_xform->position.z = -11;
    rotateXform3(camera_xform, 0, -0.2f, 0);

    initTrace(&viewport->trace, &app->scene, &app->memory);
}
void updateViewport(Viewport *viewport, Mouse *mouse) {
    if (mouse->is_captured) {
        if (mouse->moved)         orientViewport(viewport, mouse);
        if (mouse->wheel_scrolled)  zoomViewport(viewport, mouse);
    } else if (!(mouse->wheel_scrolled && app->controls.is_pressed.shift)) {
        if (mouse->wheel_scrolled) dollyViewport(viewport, mouse);
        if (mouse->moved) {
            if (mouse->middle_button.is_pressed)
                panViewport(viewport, mouse);

            if (mouse->right_button.is_pressed &&
                !app->controls.is_pressed.alt)
                orbitViewport(viewport, mouse);
        }
    }

    if (viewport->navigation.turned ||
        viewport->navigation.moved ||
        viewport->navigation.zoomed)
        updateSceneSSB(&app->scene, viewport);
}
void updateAndRender() {
    Timer *timer = &app->time.timers.update;
    Controls *controls = &app->controls;
    Viewport *viewport = &app->viewport;
    Mouse *mouse = &controls->mouse;
    Scene *scene = &app->scene;

    startFrameTimer(timer);

    if (mouse->is_captured) {
        navigateViewport(viewport, timer->delta_time);
    } else {
        manipulateSelection(scene, viewport, controls);
        if (scene->selection.transformed)
            updateSceneBVH(scene, &app->bvh_builder);
    }

    if (!controls->is_pressed.alt)
        updateViewport(viewport, mouse);

    drawSceneToViewport(scene, viewport);
    drawSelection(scene, viewport, controls);

    if (viewport->settings.show_hud) {
        setCountersInHUD(&viewport->hud, timer);
        printNumberIntoString((i32)viewport->trace.depth, &viewport->hud.lines[2].value);
        printNumberIntoString(scene->selection.primitive ? scene->selection.primitive->material_id : 0, &viewport->hud.lines[3].value);
        drawHUD(viewport->frame_buffer, &viewport->hud);
    }
    f64 now = (f64)app->time.getTicks();
    f64 tps = (f64)app->time.ticks.per_second;
    if ((now - (f64)scene_io_time) / tps <= 2.0) {
        PixelGrid *canvas = viewport->frame_buffer;
        char *message;
        RGBA color;
        if (last_scene_io_is_save) {
            message = "Scene saved to: this.scene";
            color = Color(Yellow);
        } else {
            message = "Scene loaded from: this.scene";
            color = Color(Cyan);
        }
        i32 x = canvas->dimensions.width / 2 - 150;
        i32 y = 20;
        drawText(canvas, color, message, x, y);
    }
    resetMouseChanges(mouse);
    endFrameTimer(timer);
}
void onKeyChanged(u8 key, bool is_pressed) {
    Scene *scene = &app->scene;
    Platform *platform = &app->platform;
    NavigationMove *move = &app->viewport.navigation.move;
    if (key == 'R') move->up       = is_pressed;
    if (key == 'F') move->down     = is_pressed;
    if (key == 'W') move->forward  = is_pressed;
    if (key == 'A') move->left     = is_pressed;
    if (key == 'S') move->backward = is_pressed;
    if (key == 'D') move->right    = is_pressed;

    if (!is_pressed) {
        Viewport *viewport = &app->viewport;
        ViewportSettings *settings = &viewport->settings;
        if (key == app->controls.key_map.tab)
            settings->show_hud = !settings->show_hud;

        if (app->controls.is_pressed.ctrl &&
            key == 'S' || key == 'Z') {
            last_scene_io_is_save = key == 'S';
            char *file = scene->settings.file.char_ptr;
            if (last_scene_io_is_save)
                saveSceneToFile(  scene, file, platform);
            else {
                loadSceneFromFile(scene, file, platform);
                updateSceneBVH(scene, &app->bvh_builder);
                updateSceneSSB(scene, viewport);
            }
            scene_io_time = app->time.getTicks();
        }

        if (key == '1') settings->render_mode = RenderMode_Beauty;
        if (key == '2') settings->render_mode = RenderMode_Depth;
        if (key == '3') settings->render_mode = RenderMode_Normals;
        if (key == '4') settings->render_mode = RenderMode_UVs;

        if (key == '9') settings->show_BVH = !settings->show_BVH;
        if (key == '0') settings->show_SSB = !settings->show_SSB;
    }
}
void setupScene(Scene *scene) {
    scene->ambient_light.color.x = 0.008f;
    scene->ambient_light.color.y = 0.008f;
    scene->ambient_light.color.z = 0.014f;

    u8 wall_material_id = 0;
    u8 diffuse_material_id = 1;
    u8 phong_material_id = 2;
    u8 blinn_material_id = 3;
    u8 reflective_material_id = 4;
    u8 refractive_material_id = 5;
    u8 reflective_refractive_material_id = 6;

    Material *walls_material                 = scene->materials + wall_material_id,
            *diffuse_material               = scene->materials + diffuse_material_id,
            *phong_material                 = scene->materials + phong_material_id,
            *blinn_material                 = scene->materials + blinn_material_id,
            *reflective_material            = scene->materials + reflective_material_id,
            *refractive_material            = scene->materials + refractive_material_id,
            *reflective_refractive_material = scene->materials + reflective_refractive_material_id;

    walls_material->uses = LAMBERT;
    diffuse_material->uses = LAMBERT;
    phong_material->uses = LAMBERT | PHONG | TRANSPARENCY;
    blinn_material->uses = LAMBERT | BLINN;
    reflective_material->uses = LAMBERT | BLINN | REFLECTION;
    refractive_material->uses = LAMBERT | BLINN | REFRACTION;
    reflective_refractive_material->uses = BLINN | REFLECTION | REFRACTION;

    quat identity_orientation = getIdentityQuaternion();

    Material* material = scene->materials;
    for (int i = 0; i < MATERIAL_COUNT; i++, material++) {
        material->diffuse = getVec3Of(1);
        material->roughness = 1;
        material->shininess = 1;
        material->n1_over_n2 = IOR_AIR / IOR_GLASS;
        material->n2_over_n1 = IOR_GLASS / IOR_AIR;
    }

    phong_material->shininess = 2;
    blinn_material->shininess = 3;
    reflective_material->diffuse = getVec3Of(0.1f);
    reflective_material->specular = getVec3Of(0.9f);
    phong_material->diffuse.z = 0.4f;
    diffuse_material->diffuse.x = 0.3f;
    diffuse_material->diffuse.z = 0.2f;
    diffuse_material->diffuse.z = 0.5f;

    // Back-right cube position:
    Primitive *primitive = scene->primitives;
//    primitive->material_id = reflective_material_id;
//    primitive->position.x = 9;
//    primitive->position.y = 4;
//    primitive->position.z = 3;
//
//    // Back-left cube position:
//    primitive++;
//    primitive->material_id = phong_material_id;
//    primitive->position.x = 10;
//    primitive->position.z = 1;
//
//    // Front-left cube position:
//    primitive++;
//    primitive->material_id = reflective_material_id;
//    primitive->position.x = -1;
//    primitive->position.z = -5;
//
//    // Front-right cube position:
//    primitive++;
//    primitive->material_id = blinn_material_id;
//    primitive->position.x = 10;
//    primitive->position.z = -8;
//
//    vec3 y_axis;
//    y_axis.x = 0;
//    y_axis.y = 1;
//    y_axis.z = 0;
//    quat rotation = getRotationAroundAxis(y_axis, 0.3f);
////    rotation = rotateAroundAxis(rotation, X_AXIS, 0.4f);
////    rotation = rotateAroundAxis(rotation, Z_AXIS, 0.5f);
//
//    u8 radius = 1;
//    for (u8 i = 0; i < 4; i++, radius++) {
//        primitive = scene->primitives + i;
//        primitive->id = i;
//        primitive->type = PrimitiveType_Box;
//        primitive->scale = getVec3Of((f32)radius * 0.5f);
//        primitive->position.x -= 4;
//        primitive->position.z += 2;
//        primitive->position.y = radius;
//        primitive->rotation = rotation;
//        rotation = mulQuat(rotation, rotation);
//        primitive->rotation = identity_orientation;
//    }
//
//    // Back-right tetrahedron position:
//    primitive++;
//    primitive->material_id = reflective_material_id;
//    primitive->position.x = 3;
//    primitive->position.y = 4;
//    primitive->position.z = 8;
//
//    // Back-left tetrahedron position:
//    primitive++;
//    primitive->material_id = phong_material_id;
//    primitive->position.x = 4;
//    primitive->position.z = 6;
//
//    // Front-left tetrahedron position:
//    primitive++;
//    primitive->material_id = reflective_material_id;
//    primitive->position.x = -3;
//    primitive->position.z = 0;
//
//    // Front-right tetrahedron position:
//    primitive++;
//    primitive->material_id = blinn_material_id;
//    primitive->position.x = 4;
//    primitive->position.z = -3;
//
//    radius = 1;
//
//    rotation = getRotationAroundAxis(y_axis, 0.3f);
//    for (u8 i = 4; i < 8; i++, radius++) {
//        primitive = scene->primitives + i;
//        primitive->id = i;
//        primitive->type = PrimitiveType_Tetrahedron;
//        primitive->scale = getVec3Of((f32)radius * 0.5f);
//        primitive->position.x -= 4;
//        primitive->position.z += 2;
//        if (i > 4) primitive->position.y = radius;
//        primitive->rotation = rotation;
//        rotation = mulQuat(rotation, rotation);
//    }
//
//    // Back-left sphere:
//    primitive++;
//    primitive->material_id = 1;
//    primitive->position.x = -1;
//    primitive->position.z = 5;
//
//    // Back-right sphere:
//    primitive++;
//    primitive->material_id = 2;
//    primitive->position.x = 4;
//    primitive->position.z = 6;
//
//    // Front-left sphere:
//    primitive++;
//    primitive->material_id = 4;
//    primitive->position.x = -3;
//    primitive->position.z = 0;
//
//    // Front-right sphere:
//    primitive++;
//    primitive->material_id = 5;
//    primitive->position.x = 4;
//    primitive->position.z = -8;
//
//    radius = 1;
//    for (u8 i = 8; i < 12; i++, radius++) {
//        primitive = scene->primitives + i;
//        primitive->id = i;
//        primitive->type = PrimitiveType_Sphere;
//        primitive->scale = getVec3Of(radius);
//        primitive->position.y = radius;
//        primitive->rotation = identity_orientation;
//    }

    vec3 scale = getVec3Of(1);
    for (u8 i = 0; i < 1; i++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Quad;
        primitive->scale = scale;
        primitive->material_id = 0;
        primitive->position = getVec3Of(0);
        primitive->rotation = identity_orientation;
    }

    // Bottom quad:
    primitive = scene->primitives;// + 12;
    primitive->scale.x = 40;
    primitive->scale.z = 40;

//    // Top quad:
//    primitive++;
//    primitive->scale.x = 40;
//    primitive->scale.z = 40;
//    primitive->position.y = 40;
//    primitive->rotation.axis.x = 1;
//    primitive->rotation.amount = 0;
//
//    // Left quad:
//    primitive++;
//    primitive->scale.x = 20;
//    primitive->scale.z = 40;
//    primitive->position.x = -40;
//    primitive->position.y = 20;
//    primitive->rotation.axis.z = -HALF_SQRT2;
//    primitive->rotation.amount = +HALF_SQRT2;
//
//    // Right quad:
//    primitive++;
//    primitive->scale.x = 20;
//    primitive->scale.z = 40;
//    primitive->position.x = 40;
//    primitive->position.y = 20;
//    primitive->rotation.axis.z = HALF_SQRT2;
//    primitive->rotation.amount = HALF_SQRT2;
//
//    // Back quad:
//    primitive++;
//    primitive->scale.x = 40;
//    primitive->scale.z = 20;
//    primitive->position.z = -40;
//    primitive->position.y = +20;
//    primitive->rotation.axis.x = HALF_SQRT2;
//    primitive->rotation.amount = HALF_SQRT2;
//
//    // Front quad:
//    primitive++;
//    primitive->scale.x = 40;
//    primitive->scale.z = 20;
//    primitive->position.z = 40;
//    primitive->position.y = 20;
//    primitive->rotation.axis.x = -HALF_SQRT2;
//    primitive->rotation.amount = +HALF_SQRT2;

    PointLight *key_light = scene->point_lights;
    PointLight *fill_light = scene->point_lights + 1;
    PointLight *rim_light = scene->point_lights + 2;

    key_light->position_or_direction.x = 10;
    key_light->position_or_direction.y = 10;
    key_light->position_or_direction.z = -5;
    rim_light->position_or_direction.x = 2;
    rim_light->position_or_direction.y = 5;
    rim_light->position_or_direction.z = 12;
    fill_light->position_or_direction.x = -10;
    fill_light->position_or_direction.y = 10;
    fill_light->position_or_direction.z = -5;

    key_light->color.x = 1;
    key_light->color.y = 1;
    key_light->color.z = 0.65f;
    rim_light->color.x = 1;
    rim_light->color.y = 0.25f;
    rim_light->color.z = 0.25f;
    fill_light->color.x = 0.65f;
    fill_light->color.y = 0.65f;
    fill_light->color.z = 1;

    key_light->intensity = 1.3f * 3;
    rim_light->intensity = 1.5f * 3;
    fill_light->intensity = 1.1f * 3;

    // Suzanne 1:
    primitive++;
    primitive->position.x = 10;
    primitive->position.z = 5;
    primitive->position.y = 4;
    primitive->type = PrimitiveType_Mesh;
    primitive->color = Magenta;

    primitive++;
    *primitive = *(primitive - 1);
    primitive->position.x = -10;
    primitive->color = Cyan;

    primitive++;
    *primitive = *(primitive - 1);
    primitive->id = 1;
    primitive->position.z = 10;
    primitive->position.y = 8;
    primitive->color = Blue;
}

void onResize(u16 width, u16 height) {
    updateSceneSSB(&app->scene, &app->viewport);
}

String file_paths[2];
char string_buffers[3][100];
void initApp(Defaults *defaults) {
    String *scene_file = &defaults->settings.scene.file;
    String *mesh_file1 = &file_paths[0];
    String *mesh_file2 = &file_paths[1];

    mesh_file1->char_ptr = string_buffers[0];
    mesh_file2->char_ptr = string_buffers[1];
    scene_file->char_ptr = string_buffers[2];

    u32 offset = getDirectoryLength(__FILE__);
    mergeString(mesh_file1, __FILE__, "suzanne.mesh", offset);
    mergeString(mesh_file2, __FILE__, "dragon.mesh",  offset);
    mergeString(scene_file, __FILE__, "this.scene",   offset);
    defaults->settings.scene.meshes = 2;
    defaults->settings.scene.mesh_files = file_paths;
    defaults->settings.scene.point_lights = POINT_LIGHT_COUNT;
    defaults->settings.scene.materials    = MATERIAL_COUNT;
    defaults->settings.scene.primitives   = 4;//PRIMITIVE_COUNT + 3;
    defaults->settings.viewport.hud_line_count = 4;

    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onButtonDown;
    app->on.mouseButtonDoubleClicked = onDoubleClick;
    app->on.windowResize  = onResize;
    app->on.windowRedraw  = updateAndRender;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;

    scene_io_time = 0;
}