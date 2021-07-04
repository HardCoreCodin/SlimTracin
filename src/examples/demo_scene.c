#include "../SlimRayTracer/app.h"
#include "../SlimRayTracer/core/time.h"
#include "../SlimRayTracer/viewport/hud.h"
#include "../SlimRayTracer/viewport/navigation.h"
#include "../SlimRayTracer/viewport/manipulation.h"
#include "../SlimRayTracer/render/raytracer.h"

enum HUDLineID {
    HUD_LINE__FPS,
    HUD_LINE__GPU,
    HUD_LINE__Width,
    HUD_LINE__Height,
    HUD_LINE__Bounces,
    HUD_LINE__MatID,
    HUD_LINE__BVH,
    HUD_LINE__SSB,
    HUD_LINE__Mode
};

u64 scene_io_time;
bool last_scene_io_is_save;

void setDimensionsInHUD(HUD *hud, i32 width, i32 height) {
    printNumberIntoString(width,  &hud->lines[HUD_LINE__Width ].value);
    printNumberIntoString(height, &hud->lines[HUD_LINE__Height].value);
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
    renderScene(scene, viewport);
    if (viewport->settings.show_BVH) drawBVH(scene, viewport);
    if (viewport->settings.show_SSB) drawSSB(scene, viewport);
}
void setupViewport(Viewport *viewport) {
    Dimensions *dim = &viewport->frame_buffer->dimensions;
    u16 fps = app->time.timers.update.average_frames_per_second;
    i32 bounces = (i32)viewport->trace.depth;
    HUD *hud = &viewport->hud;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    setDimensionsInHUD(hud, dim->width, dim->height);

    HUDLine *line = hud->lines;
    enum HUDLineID line_id;
    for (u8 i = 0; i < (u8)hud->line_count; i++, line++) {
        NumberString *num = &line->value;
        String *str = &line->value.string;
        String *alt = &line->alternate_value;

        line_id = (enum HUDLineID)(i);
        switch (line_id) {
            case HUD_LINE__GPU:
            case HUD_LINE__SSB:
            case HUD_LINE__BVH:
                setString(str, (char*)("On"));
                setString(alt, (char*)("Off"));
                line->alternate_value_color = DarkGreen;
                line->invert_alternate_use = true;
                line->use_alternate = line_id == HUD_LINE__BVH ? &viewport->settings.show_BVH :
                                      line_id == HUD_LINE__SSB ? &viewport->settings.show_SSB :
                                      &viewport->settings.use_GPU;
                break;
            case HUD_LINE__FPS:     printNumberIntoString(fps,         num); break;
            case HUD_LINE__Width:   printNumberIntoString(dim->width,  num); break;
            case HUD_LINE__Height:  printNumberIntoString(dim->height, num); break;
            case HUD_LINE__Bounces: printNumberIntoString(bounces,     num); break;
            case HUD_LINE__MatID:   printNumberIntoString(0,   num); break;
            case HUD_LINE__Mode:
                setRenderModeString(viewport->settings.render_mode, str);
                break;
            default:
                break;
        }

        str = &line->title;
        switch (line_id) {
            case HUD_LINE__FPS:     setString(str, (char*)"Fps    : "); break;
            case HUD_LINE__GPU:     setString(str, (char*)"Use GPU: "); break;
            case HUD_LINE__SSB:     setString(str, (char*)"ShowSSB: "); break;
            case HUD_LINE__BVH:     setString(str, (char*)"ShowBVH: "); break;
            case HUD_LINE__Width:   setString(str, (char*)"Width  : "); break;
            case HUD_LINE__Height:  setString(str, (char*)"Height : "); break;
            case HUD_LINE__Bounces: setString(str, (char*)"Bounces: "); break;
            case HUD_LINE__MatID:   setString(str, (char*)"Mat. id: "); break;
            case HUD_LINE__Mode:    setString(str, (char*)"Mode   : "); break;
            default:
                break;
        }
    }

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
        if (scene->selection->transformed) updateScene(scene, viewport);
    }

    if (!controls->is_pressed.alt)
        updateViewport(viewport, mouse);

    drawSceneToViewport(scene, viewport);
    drawSelection(scene, viewport, controls);

    if (viewport->settings.show_hud) {
        printNumberIntoString(app->time.timers.update.average_frames_per_second, &viewport->hud.lines[0].value);
        printNumberIntoString((i32)viewport->trace.depth, &viewport->hud.lines[4].value);
        printNumberIntoString(scene->selection->primitive ? scene->selection->primitive->material_id : 0, &viewport->hud.lines[5].value);
        drawHUD(viewport->frame_buffer, &viewport->hud);
    }
    f64 now = (f64)app->time.getTicks();
    f64 tps = (f64)app->time.ticks.per_second;
    if ((now - (f64)scene_io_time) / tps <= 2.0) {
        PixelGrid *canvas = viewport->frame_buffer;
        char *message;
        RGBA color;
        if (last_scene_io_is_save) {
            message = (char*)"Scene saved to: this.scene";
            color = Color(Yellow);
        } else {
            message = (char*)"Scene loaded from: this.scene";
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
                updateScene(scene, viewport);
            }
            scene_io_time = app->time.getTicks();
        }

        if (key == 'G') settings->use_GPU = USE_GPU_BY_DEFAULT ? !settings->use_GPU : false;
        if (key == '9') settings->show_BVH = !settings->show_BVH;
        if (key == '0') settings->show_SSB = !settings->show_SSB;
        if (key == '1') settings->render_mode = RenderMode_Beauty;
        if (key == '2') settings->render_mode = RenderMode_Depth;
        if (key == '3') settings->render_mode = RenderMode_Normals;
        if (key == '4') settings->render_mode = RenderMode_UVs;
        if (key >= '1' &&
            key <= '4')
            setRenderModeString(settings->render_mode,
                                &viewport->hud.lines[HUD_LINE__Mode].value.string);
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

    Material *walls_material                 = scene->materials + wall_material_id,
            *diffuse_material               = scene->materials + diffuse_material_id,
            *phong_material                 = scene->materials + phong_material_id,
            *blinn_material                 = scene->materials + blinn_material_id,
            *reflective_material            = scene->materials + reflective_material_id,
            *refractive_material            = scene->materials + refractive_material_id;

    walls_material->uses = LAMBERT;
    diffuse_material->uses = LAMBERT;
    phong_material->uses = LAMBERT | PHONG | TRANSPARENCY;
    blinn_material->uses = LAMBERT | BLINN;
    reflective_material->uses = BLINN | REFLECTION;
    refractive_material->uses = BLINN | REFRACTION;

    quat identity_orientation = getIdentityQuaternion();

    Material* material = scene->materials;
    for (u32 i = 0; i < scene->settings.materials; i++, material++) {
        material->diffuse = getVec3Of(1);
        material->specular = getVec3Of(1);
        material->roughness = 1;
        material->shininess = 1;
        material->n1_over_n2 = IOR_AIR / IOR_GLASS;
        material->n2_over_n1 = IOR_GLASS / IOR_AIR;
    }

    phong_material->shininess = 2;
    blinn_material->shininess = 3;
    reflective_material->specular = getVec3Of(0.9f);
    phong_material->diffuse.z = 0.4f;
    diffuse_material->diffuse.x = 0.3f;
    diffuse_material->diffuse.z = 0.2f;
    diffuse_material->diffuse.z = 0.5f;

    // Back-right cube position:
    Primitive *primitive;
    vec3 scale = getVec3Of(1);
    for (u8 i = 0; i < 2; i++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Quad;
        primitive->scale = scale;
        primitive->material_id = 0;
        primitive->position = getVec3Of(0);
        primitive->rotation = identity_orientation;
    }

    // Bottom quad:
    primitive = scene->primitives;
    primitive->scale.x = 40;
    primitive->scale.z = 40;

    // Left quad:
    primitive++;
    primitive->scale.x = 20;
    primitive->scale.z = 40;
    primitive->position.x = -40;
    primitive->position.y = 20;
    primitive->rotation.axis.z = -HALF_SQRT2;
    primitive->rotation.amount = +HALF_SQRT2;

    Light *key_light = scene->lights;
    Light *fill_light = scene->lights + 1;
    Light *rim_light = scene->lights + 2;

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
    key_light->is_directional = fill_light->is_directional = rim_light->is_directional = false;

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
    setDimensionsInHUD(&app->viewport.hud, width, height);
    updateSceneSSB(&app->scene, &app->viewport);
}

String file_paths[2];
char string_buffers[3][100];
void initApp(Defaults *defaults) {
    char* this_file   = (char*)__FILE__;
    char* monkey_file = (char*)"suzanne.mesh";
    char* dragon_file = (char*)"dragon.mesh";
    char* scene_file  = (char*)"this.scene";

    String *scene  = &defaults->settings.scene.file;
    String *monkey = &file_paths[0];
    String *dragon = &file_paths[1];

    monkey->char_ptr = string_buffers[0];
    dragon->char_ptr = string_buffers[1];
    scene->char_ptr  = string_buffers[2];

    u32 dir_len = getDirectoryLength(this_file);
    mergeString(monkey, this_file, monkey_file, dir_len);
    mergeString(dragon, this_file, dragon_file, dir_len);
    mergeString(scene,  this_file, scene_file,  dir_len);

    defaults->settings.scene.meshes = 2;
    defaults->settings.scene.mesh_files = file_paths;
    defaults->settings.scene.lights = 3;
    defaults->settings.scene.materials    = 6;
    defaults->settings.scene.primitives   = 5;
    defaults->settings.viewport.hud_line_count = 9;
    defaults->settings.viewport.hud_default_color = Green;

    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onButtonDown;
    app->on.mouseButtonDoubleClicked = onDoubleClick;
    app->on.windowResize  = onResize;
    app->on.windowRedraw  = updateAndRender;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;

    scene_io_time = 0;
}