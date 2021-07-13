#include "../SlimRayTracer/app.h"
#include "../SlimRayTracer/core/time.h"
#include "../SlimRayTracer/viewport/hud.h"
#include "../SlimRayTracer/viewport/navigation.h"
#include "../SlimRayTracer/viewport/manipulation.h"
#include "../SlimRayTracer/render/raytracer.h"

enum HUDLineID {
    HUD_LINE_FPS,
    HUD_LINE_GPU,
    HUD_LINE_Width,
    HUD_LINE_Height,
    HUD_LINE_Bounces,
    HUD_LINE_MatID,
    HUD_LINE_BVH,
    HUD_LINE_SSB,
    HUD_LINE_Mode
};



enum MaterialIDs {
    MaterialID_Wall,
    MaterialID_Diffuse,
    MaterialID_Phong,
    MaterialID_Blinn,
    MaterialID_Reflective,
    MaterialID_Refractive,
    MaterialID_Emissive
};

void setupLights(Scene *scene) {
    scene->ambient_light.color.x = 0.008f;
    scene->ambient_light.color.y = 0.008f;
    scene->ambient_light.color.z = 0.014f;

    Light *key_light  = scene->lights + 0;
    Light *fill_light = scene->lights + 1;
    Light *rim_light  = scene->lights + 2;

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

    key_light->intensity  = 1.3f * 4;
    rim_light->intensity  = 1.5f * 4;
    fill_light->intensity = 1.1f * 4;
    key_light->is_directional = fill_light->is_directional = rim_light->is_directional = false;
}

void setupMaterials(Scene *scene) {
    Material *walls_material     = scene->materials + MaterialID_Wall,
            *diffuse_material    = scene->materials + MaterialID_Diffuse,
            *phong_material      = scene->materials + MaterialID_Phong,
            *blinn_material      = scene->materials + MaterialID_Blinn,
            *reflective_material = scene->materials + MaterialID_Reflective,
            *refractive_material = scene->materials + MaterialID_Refractive,
            *emissive_material   = scene->materials + MaterialID_Emissive;

    walls_material->flags = LAMBERT;
    diffuse_material->flags = LAMBERT;
    phong_material->flags = LAMBERT | PHONG | TRANSPARENCY;
    blinn_material->flags = LAMBERT | BLINN;
    reflective_material->flags = BLINN | REFLECTION;
    refractive_material->flags = BLINN | REFRACTION;
    emissive_material->flags = EMISSION;

    Material* material = scene->materials;
    for (u32 i = 0; i < scene->settings.materials; i++, material++) {
        material->n1_over_n2 = IOR_AIR / IOR_GLASS;
        material->n2_over_n1 = IOR_GLASS / IOR_AIR;
    }

    phong_material->shininess = 2;
    blinn_material->shininess = 3;
    reflective_material->specular = refractive_material->specular = getVec3Of(0.9f);
//    refractive_material->specular.x = 0.6f;
//    refractive_material->specular.y = 0.7f;
//    refractive_material->specular.z = 0.3f;
    phong_material->diffuse.z = 0.4f;
    diffuse_material->diffuse.x = 0.3f;
    diffuse_material->diffuse.z = 0.2f;
    diffuse_material->diffuse.z = 0.5f;
}

void setDimensionsInHUD(HUD *hud, i32 width, i32 height) {
    printNumberIntoString(width,  &hud->lines[HUD_LINE_Width ].value);
    printNumberIntoString(height, &hud->lines[HUD_LINE_Height].value);
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
void drawSceneMessage(Scene *scene, Viewport *viewport) {
    f64 now = (f64)app->time.getTicks();
    f64 tps = (f64)app->time.ticks.per_second;
    if ((now - (f64)scene->last_io_ticks) / tps <= 2.0) {
        PixelGrid *canvas = viewport->frame_buffer;
        char *message;
        RGBA color;
        if (scene->last_io_is_save) {
            message = (char*)"   Scene saved to: ";
            color = Color(Yellow);
        } else {
            message = (char*)"Scene loaded from: ";
            color = Color(Cyan);
        }
        i32 x = 10;
        i32 y = 20;
        drawText(canvas, color, message, x, y);
        drawText(canvas, color, scene->settings.file.char_ptr, x + 100, y);
    }
}
void drawSceneToViewport(Scene *scene, Viewport *viewport) {
    fillPixelGrid(viewport->frame_buffer, Color(Black));
    renderScene(scene, viewport);
    if (viewport->settings.show_BVH) drawBVH(scene, viewport);
    if (viewport->settings.show_SSB) drawSSB(scene, viewport);
}

void setupHUD(Viewport *viewport) {
    Dimensions *dim = &viewport->frame_buffer->dimensions;
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
            case HUD_LINE_GPU:
            case HUD_LINE_SSB:
            case HUD_LINE_BVH:
                setString(str, (char*)("On"));
                setString(alt, (char*)("Off"));
                line->alternate_value_color = DarkGreen;
                line->invert_alternate_use = true;
                line->use_alternate = line_id == HUD_LINE_BVH ? &viewport->settings.show_BVH :
                                      line_id == HUD_LINE_SSB ? &viewport->settings.show_SSB :
                                      &viewport->settings.use_GPU;
                break;
            case HUD_LINE_FPS:     printNumberIntoString(0, num); break;
            case HUD_LINE_Width:   printNumberIntoString(dim->width, num); break;
            case HUD_LINE_Height:  printNumberIntoString(dim->height, num); break;
            case HUD_LINE_Bounces: printNumberIntoString(bounces, num); break;
            case HUD_LINE_MatID:   printNumberIntoString(0, num); break;
            case HUD_LINE_Mode:
                setRenderModeString(viewport->settings.render_mode, str);
                break;
            default:
                break;
        }

        str = &line->title;
        switch (line_id) {
            case HUD_LINE_FPS:     setString(str, (char*)"Fps    : "); break;
            case HUD_LINE_GPU:     setString(str, (char*)"Use GPU: "); break;
            case HUD_LINE_SSB:     setString(str, (char*)"ShowSSB: "); break;
            case HUD_LINE_BVH:     setString(str, (char*)"ShowBVH: "); break;
            case HUD_LINE_Width:   setString(str, (char*)"Width  : "); break;
            case HUD_LINE_Height:  setString(str, (char*)"Height : "); break;
            case HUD_LINE_Bounces: setString(str, (char*)"Bounces: "); break;
            case HUD_LINE_MatID:   setString(str, (char*)"Mat. id: "); break;
            case HUD_LINE_Mode:    setString(str, (char*)"Mode   : "); break;
            default:
                break;
        }
    }
}

//void setupViewport(Viewport *viewport) {
//    viewport->trace.depth = 4;
//    viewport->camera->transform.position.y = 7;
//    viewport->camera->transform.position.z = -11;
//    rotateXform3(&viewport->camera->transform, 0, -0.2f, 0);
//
//    setupHUD(viewport);
//}

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

void onUpdate(Scene *scene, f32 delta_time);

void manipulateMaterial(Controls *controls, Viewport *viewport, Primitive *selected_primitive, u32 last_material_id) {
    Mouse *mouse = &controls->mouse;
    bool down = mouse->wheel_scroll_amount < 0;
    u32 amount = (u32)(down ? -mouse->wheel_scroll_amount : mouse->wheel_scroll_amount);
    if (amount / 50 > 0) {
        mouse->wheel_scroll_handled = true;

        if (controls->is_pressed.ctrl) {
            if (down) {
                if (viewport->trace.depth > 1)
                    viewport->trace.depth--;
            } else {
                if (viewport->trace.depth < 5)
                    viewport->trace.depth++;
            }
            printNumberIntoString((i32)viewport->trace.depth, &viewport->hud.lines[HUD_LINE_Bounces].value);
        } else if (selected_primitive) {
            if (down) {
                if (selected_primitive->material_id)
                    selected_primitive->material_id--;
                else
                    selected_primitive->material_id = last_material_id;
            } else {
                if (selected_primitive->material_id == last_material_id)
                    selected_primitive->material_id = 0;
                else
                    selected_primitive->material_id++;
            }
        }
    }
}

//void updateAndRender() {
//    Timer *timer = &app->time.timers.update;
//    Controls *controls = &app->controls;
//    Viewport *viewport = &app->viewport;
//    Mouse *mouse = &controls->mouse;
//    Scene *scene = &app->scene;
//
//    startFrameTimer(timer);
//
//    onUpdate(scene, timer->delta_time);
//
//    if (mouse->is_captured)
//        navigateViewport(viewport, timer->delta_time);
//    else
//        manipulateSelection(scene, viewport, controls);
//
//    if (scene->selection->changed) {
//        scene->selection->changed = false;
//        printNumberIntoString(scene->selection->primitive ?
//        scene->selection->primitive->material_id : 0,
//        &viewport->hud.lines[HUD_LINE_MatID].value);
//    }
//
//    if (!controls->is_pressed.alt)
//        updateViewport(viewport, mouse);
//
//    drawSceneToViewport(scene, viewport);
//    drawSelection(scene, viewport, controls);
//
//    if (viewport->settings.show_hud)
//        drawHUD(viewport->frame_buffer, &viewport->hud);
//
//    drawSceneMessage(scene, viewport);
//
//    if (controls->is_pressed.shift && mouse->wheel_scrolled)
//        manipulateMaterial(controls, viewport, scene->selection->primitive,
//                           scene->settings.materials - 1);
//
//    resetMouseChanges(mouse);
//
//    endFrameTimer(timer);
//    printNumberIntoString(app->time.timers.update.average_frames_per_second, &viewport->hud.lines[HUD_LINE_FPS].value);
//}

void updateViewportRenderMode(Viewport *viewport, u8 key) {
    ViewportSettings *settings = &viewport->settings;
    if (key == '1') settings->render_mode = RenderMode_Beauty;
    if (key == '2') settings->render_mode = RenderMode_Depth;
    if (key == '3') settings->render_mode = RenderMode_Normals;
    if (key == '4') settings->render_mode = RenderMode_UVs;
    if (key >= '1' &&
        key <= '4')
        setRenderModeString(settings->render_mode,
                            &viewport->hud.lines[HUD_LINE_Mode].value.string);
}

void saveOrLoadScene(Scene *scene, Viewport *viewport, Platform *platform, bool save) {
    char *file = scene->settings.file.char_ptr;
    scene->last_io_is_save = save;
    if (scene->last_io_is_save)
        saveSceneToFile(  scene, file, platform);
    else {
        loadSceneFromFile(scene, file, platform);
        updateScene(scene, viewport);
    }
    scene->last_io_ticks = platform->getTicks();
}

//void onKeyChanged(u8 key, bool is_pressed) {
//    Scene *scene = &app->scene;
//    Platform *platform = &app->platform;
//    NavigationMove *move = &app->viewport.navigation.move;
//    if (key == 'R') move->up       = is_pressed;
//    if (key == 'F') move->down     = is_pressed;
//    if (key == 'W') move->forward  = is_pressed;
//    if (key == 'A') move->left     = is_pressed;
//    if (key == 'S') move->backward = is_pressed;
//    if (key == 'D') move->right    = is_pressed;
//
//    if (!is_pressed) {
//        Viewport *viewport = &app->viewport;
//        ViewportSettings *settings = &viewport->settings;
//        if (key == app->controls.key_map.tab)
//            settings->show_hud = !settings->show_hud;
//
//        if (app->controls.is_pressed.ctrl && key == 'S' || key == 'Z')
//            saveOrLoadScene(scene, viewport, platform, key == 'S');
//
//        if (key == 'T' && scene->selection->primitive) {
//            if (scene->selection->primitive->flags & IS_TRANSPARENT)
//                scene->selection->primitive->flags &= ~IS_TRANSPARENT;
//            else
//                scene->selection->primitive->flags |= IS_TRANSPARENT;
//        }
//        if (key == 'G') settings->use_GPU = USE_GPU_BY_DEFAULT ? !settings->use_GPU : false;
//        if (key == '9') settings->show_BVH = !settings->show_BVH;
//        if (key == '0') settings->show_SSB = !settings->show_SSB;
//        updateViewportRenderMode(viewport, key);
//    }
//}


void onResize(u16 width, u16 height) {
    setDimensionsInHUD(&app->viewport.hud, width, height);
}