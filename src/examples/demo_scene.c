#define USE_DEMO_SCENE
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

String file_paths[3];
char string_buffers[3][100];

void setScene(Scene *scene) {
    initSSB(&scene->ssb, scene->settings.primitives, &app->memory);
    initBVH(&scene->bvh, scene->settings.primitives, &app->memory);

#ifdef USE_DEMO_SCENE
    initBVHBuilder(&bvh_builder, scene, &app->memory);

    if (scene->settings.meshes) {
        Mesh *mesh = scene->meshes;
        for (u32 m = 0; m < scene->settings.meshes; m++, mesh++) {
            updateMeshBVH(mesh, &bvh_builder);
            scene->mesh_bvh_node_counts[m] = mesh->bvh.node_count;
        }
    }

    updateSceneBVH(scene, &bvh_builder);
#endif


    uploadSceneBVH(scene);
    uploadMeshBVHs(scene);

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

    quat identity_orientation;
    identity_orientation.axis = getVec3Of(0);
    identity_orientation.amount = 1;

    Material* material = scene->materials;
    for (int i = 0; i < MATERIAL_COUNT; i++, material++) {
        material->diffuse = getVec3Of(1);
        material->roughness = 1;
        material->shininess = 1;
        material->n1_over_n2 = IOR_AIR / IOR_GLASS;
        material->n2_over_n1 = IOR_GLASS / IOR_AIR;
    }

    phong_material->diffuse.z = 0.4f;
    diffuse_material->diffuse.x = 0.3f;
    diffuse_material->diffuse.z = 0.2f;
    diffuse_material->diffuse.z = 0.5f;

    // Back-right cube position:
    Primitive *primitive = scene->primitives;
    primitive->material_id = reflective_material_id;
    primitive->position.x = 9;
    primitive->position.y = 4;
    primitive->position.z = 3;

    // Back-left cube position:
    primitive++;
    primitive->material_id = phong_material_id;
    primitive->position.x = 10;
    primitive->position.z = 1;

    // Front-left cube position:
    primitive++;
    primitive->material_id = reflective_material_id;
    primitive->position.x = -1;
    primitive->position.z = -5;

    // Front-right cube position:
    primitive++;
    primitive->material_id = blinn_material_id;
    primitive->position.x = 10;
    primitive->position.z = -8;

    vec3 y_axis;
    y_axis.x = 0;
    y_axis.y = 1;
    y_axis.z = 0;
    quat rotation = getRotationAroundAxis(y_axis, 0.3f);
//    rotation = rotateAroundAxis(rotation, X_AXIS, 0.4f);
//    rotation = rotateAroundAxis(rotation, Z_AXIS, 0.5f);

    u8 radius = 1;
    for (u8 i = 0; i < 4; i++, radius++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Box;
        primitive->scale = getVec3Of((f32)radius * 0.5f);
        primitive->position.x -= 4;
        primitive->position.z += 2;
        primitive->position.y = radius;
        primitive->rotation = rotation;
        rotation = mulQuat(rotation, rotation);
        primitive->rotation = identity_orientation;
    }

    // Back-right tetrahedron position:
    primitive++;
    primitive->material_id = reflective_material_id;
    primitive->position.x = 3;
    primitive->position.y = 4;
    primitive->position.z = 8;

    // Back-left tetrahedron position:
    primitive++;
    primitive->material_id = phong_material_id;
    primitive->position.x = 4;
    primitive->position.z = 6;

    // Front-left tetrahedron position:
    primitive++;
    primitive->material_id = reflective_material_id;
    primitive->position.x = -3;
    primitive->position.z = 0;

    // Front-right tetrahedron position:
    primitive++;
    primitive->material_id = blinn_material_id;
    primitive->position.x = 4;
    primitive->position.z = -3;

    radius = 1;

    rotation = getRotationAroundAxis(y_axis, 0.3f);
    for (u8 i = 4; i < 8; i++, radius++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Tetrahedron;
        primitive->scale = getVec3Of((f32)radius * 0.5f);
        primitive->position.x -= 4;
        primitive->position.z += 2;
        if (i > 4) primitive->position.y = radius;
        primitive->rotation = rotation;
        rotation = mulQuat(rotation, rotation);
    }

    // Back-left sphere:
    primitive++;
    primitive->material_id = 1;
    primitive->position.x = -1;
    primitive->position.z = 5;

    // Back-right sphere:
    primitive++;
    primitive->material_id = 2;
    primitive->position.x = 4;
    primitive->position.z = 6;

    // Front-left sphere:
    primitive++;
    primitive->material_id = 4;
    primitive->position.x = -3;
    primitive->position.z = 0;

    // Front-right sphere:
    primitive++;
    primitive->material_id = 5;
    primitive->position.x = 4;
    primitive->position.z = -8;

    radius = 1;
    for (u8 i = 8; i < 12; i++, radius++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Sphere;
        primitive->scale = getVec3Of(radius);
        primitive->position.y = radius;
        primitive->rotation = identity_orientation;
    }

    vec3 scale = getVec3Of(1);
    for (u8 i = 12; i < 18; i++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Quad;
        primitive->scale = scale;
        primitive->material_id = 0;
        primitive->position = getVec3Of(0);
        primitive->rotation = identity_orientation;
    }

    // Bottom quad:
    primitive = scene->primitives + 12;
    primitive->scale.x = 40;
    primitive->scale.z = 40;

    // Top quad:
    primitive++;
    primitive->scale.x = 40;
    primitive->scale.z = 40;
    primitive->position.y = 40;
    primitive->rotation.axis.x = 1;
    primitive->rotation.amount = 0;

    // Left quad:
    primitive++;
    primitive->scale.x = 20;
    primitive->scale.z = 40;
    primitive->position.x = -40;
    primitive->position.y = 20;
    primitive->rotation.axis.z = -HALF_SQRT2;
    primitive->rotation.amount = +HALF_SQRT2;

    // Right quad:
    primitive++;
    primitive->scale.x = 20;
    primitive->scale.z = 40;
    primitive->position.x = 40;
    primitive->position.y = 20;
    primitive->rotation.axis.z = HALF_SQRT2;
    primitive->rotation.amount = HALF_SQRT2;

    // Back quad:
    primitive++;
    primitive->scale.x = 40;
    primitive->scale.z = 20;
    primitive->position.z = -40;
    primitive->position.y = +20;
    primitive->rotation.axis.x = HALF_SQRT2;
    primitive->rotation.amount = HALF_SQRT2;

    // Front quad:
    primitive++;
    primitive->scale.x = 40;
    primitive->scale.z = 20;
    primitive->position.z = 40;
    primitive->position.y = 20;
    primitive->rotation.axis.x = -HALF_SQRT2;
    primitive->rotation.amount = +HALF_SQRT2;

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
}

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
    Primitive *primitive = scene->primitives;
    for (u32 i = 0; i < scene->settings.primitives; i++, primitive++)
        switch (primitive->type) {
            case PrimitiveType_Mesh:
                drawMesh(viewport, Color(primitive->color),
                         &scene->meshes[primitive->id], primitive, false);
                break;
            case PrimitiveType_Coil:
            case PrimitiveType_Helix:
                drawCurve(viewport, Color(primitive->color),
                          &scene->curves[primitive->id], primitive,
                          CURVE_STEPS);
                break;
            case PrimitiveType_Box:
                drawBox(viewport, Color(primitive->color),
                        &scene->boxes[primitive->id], primitive,
                        BOX__ALL_SIDES);
                break;
            case PrimitiveType_Grid:
                drawGrid(viewport, Color(primitive->color),
                         &scene->grids[primitive->id], primitive);
                break;
            default:
                break;
        }
}
void setupViewport(Viewport *viewport) {
    HUD *hud = &viewport->hud;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    setCountersInHUD(hud, &app->time.timers.update);
    setString(&hud->lines[0].title, "Fps    : ");
    setString(&hud->lines[1].title, "mic-s/f: ");
    hud->lines[0].title_color = hud->lines[1].title_color = Green;
    hud->lines[0].value_color = hud->lines[1].value_color = Green;

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
    } else {
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
        onViewportChanged(&app->scene, viewport);
}
void updateAndRender() {
    Timer *timer = &app->time.timers.update;
    Controls *controls = &app->controls;
    Viewport *viewport = &app->viewport;
    Mouse *mouse = &controls->mouse;
    Scene *scene = &app->scene;

    startFrameTimer(timer);

    if (mouse->is_captured)
        navigateViewport(viewport, timer->delta_time);
    else
        manipulateSelection(scene, viewport, controls);

    if (!controls->is_pressed.alt)
        updateViewport(viewport, mouse);

//    drawSceneToViewport(scene, viewport);
    onRender(scene, viewport);
    drawSelection(scene, viewport, controls);

    if (viewport->settings.show_hud) {
        setCountersInHUD(&viewport->hud, timer);
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

    if (app->controls.is_pressed.ctrl &&
        !is_pressed &&
        key == 'S' ||
        key == 'Z')
    {
        last_scene_io_is_save = key == 'S';
        String file = file_paths[2];
        if (last_scene_io_is_save)
            saveSceneToFile(  scene, file, platform);
        else
            loadSceneFromFile(scene, file, platform);

        scene_io_time = app->time.getTicks();
    }
    NavigationMove *move = &app->viewport.navigation.move;
    if (key == 'R') move->up       = is_pressed;
    if (key == 'F') move->down     = is_pressed;
    if (key == 'W') move->forward  = is_pressed;
    if (key == 'A') move->left     = is_pressed;
    if (key == 'S') move->backward = is_pressed;
    if (key == 'D') move->right    = is_pressed;

    ViewportSettings *settings = &app->viewport.settings;
    if (!is_pressed && key == app->controls.key_map.tab)
        settings->show_hud = !settings->show_hud;
}
void setupScene(Scene *scene) {
    Primitive *grid_primitive  = &scene->primitives[0];
    Primitive *dragon_prim     = &scene->primitives[1];
    Primitive *suzanne1_prim   = &scene->primitives[2];
    Primitive *suzanne2_prim   = &scene->primitives[3];
    Primitive *helix_primitive = &scene->primitives[4];
    Primitive *coil_primitive  = &scene->primitives[5];
    Primitive *box_primitive   = &scene->primitives[6];

    helix_primitive->type = PrimitiveType_Helix;
    coil_primitive->type  = PrimitiveType_Coil;
    grid_primitive->type  = PrimitiveType_Grid;
    box_primitive->type   = PrimitiveType_Box;
    box_primitive->color   = Yellow;
    grid_primitive->color  = Green;
    coil_primitive->color  = Magenta;
    helix_primitive->color = Cyan;
    helix_primitive->position.x = -3;
    helix_primitive->position.y = 4;
    helix_primitive->position.z = 2;
    coil_primitive->position.x = 4;
    coil_primitive->position.y = 4;
    coil_primitive->position.z = 2;
    box_primitive->id = grid_primitive->id = helix_primitive->id = 0;
    coil_primitive->id  = 1;
    grid_primitive->scale.x = 5;
    grid_primitive->scale.z = 5;
    scene->curves[0].revolution_count = 10;
    scene->curves[1].revolution_count = 30;

    grid_primitive->type = PrimitiveType_Grid;
    grid_primitive->scale.x = 5;
    grid_primitive->scale.z = 5;
    initGrid(scene->grids,11, 11);

    suzanne1_prim->position.x = 10;
    suzanne1_prim->position.z = 5;
    suzanne1_prim->position.y = 4;
    suzanne1_prim->type = PrimitiveType_Mesh;
    suzanne1_prim->color = Magenta;

    *suzanne2_prim = *suzanne1_prim;
    suzanne2_prim->position.x = -10;
    suzanne2_prim->color = Cyan;

    *dragon_prim = *suzanne1_prim;
    dragon_prim->id = 1;
    dragon_prim->position.z = 10;
    dragon_prim->color = Blue;
}

void onResize(u16 width, u16 height) {
    onViewportChanged(&app->scene, &app->viewport);
}

void initApp(Defaults *defaults) {
//    char *file_names[3];
//    file_names[0] = "suzanne.mesh";
//    file_names[1] = "dragon.mesh";
//    file_names[2] = "this.scene";
//
//    u32 dir_len = getDirectoryLength(__FILE__);
//    for (u8 i = 0; i < 3; i++) {
//        file_paths[i].char_ptr = string_buffers[i];
//        copyToString(&file_paths[i], __FILE__, 0);
//        copyToString(&file_paths[i], file_names[i], dir_len);
//    }

//    defaults->settings.scene.mesh_files = file_paths;
    defaults->additional_memory_size = Megabytes(7) +  Kilobytes(64);
    defaults->settings.scene.point_lights = POINT_LIGHT_COUNT;
    defaults->settings.scene.materials    = MATERIAL_COUNT;
    defaults->settings.scene.primitives   = PRIMITIVE_COUNT;

//    defaults->settings.scene.boxes      = 1;
//    defaults->settings.scene.grids      = 1;
//    defaults->settings.scene.curves     = 2;
//    defaults->settings.scene.meshes     = 2;
//    defaults->settings.scene.primitives = 7;
    defaults->settings.viewport.hud_line_count = 2;
    scene_io_time = 0;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onButtonDown;
    app->on.mouseButtonDoubleClicked = onDoubleClick;
    app->on.windowResize = onResize;
    app->on.windowRedraw  = updateAndRender;
    app->on.sceneReady    = setScene;
    app->on.viewportReady = setupViewport;
}