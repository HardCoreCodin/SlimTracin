//#include "../SlimRayTracer/app.h"
//#include "../SlimRayTracer/core/time.h"
//#include "../SlimRayTracer/viewport/hud.h"
//#include "../SlimRayTracer/viewport/navigation.h"
//#include "../SlimRayTracer/viewport/manipulation.h"
//#include "../SlimRayTracer/render/raytracer.h"

//enum HUDLineID {
//    HUD_LINE_FPS,
//    HUD_LINE_GPU,
//    HUD_LINE_Width,
//    HUD_LINE_Height,
//    HUD_LINE_Bounces,
//    HUD_LINE_MatID,
//    HUD_LINE_BVH,
//    HUD_LINE_SSB,
//    HUD_LINE_Mode
//};

//void setupShapes(Scene *scene, quat *rotation) {
//    vec3 X = Vec3(1, 0, 0);
//    vec3 Y = Vec3(0, 1, 0);
//    vec3 Z = Vec3(0, 0, 1);
//    quat rot_x = getRotationAroundAxis(X, 0.02f);
//    quat rot_y = getRotationAroundAxis(Y, 0.04f);
//    quat rot_z = getRotationAroundAxis(Z, 0.06f);
//    Primitive *box = &scene->primitives[0];
//    Primitive *tet = &scene->primitives[1];
//    Primitive *spr = &scene->primitives[2];
//    Primitive *qud = &scene->primitives[3];
//    box->type = PrimitiveType_Box;
//    tet->type = PrimitiveType_Tetrahedron;
//    spr->type = PrimitiveType_Sphere;
//    qud->type = PrimitiveType_Quad;
//    spr->position = Vec3( 3, 3, 0);
//    box->position = Vec3(-9, 3, 3);
//    tet->position = Vec3(-3, 4, 12);
//    qud->scale    = Vec3(40, 1, 40);
//    box->scale = tet->scale = spr->scale = getVec3Of(2.5f);
//    box->rotation = normQuat(mulQuat(rot_x, rot_y));
//    tet->rotation = normQuat(mulQuat(box->rotation, rot_z));
//    *rotation = tet->rotation;
//}
//void setDimensionsInHUD(HUD *hud, i32 width, i32 height) {
//    printNumberIntoString(width,  &hud->lines[HUD_LINE_Width ].value);
//    printNumberIntoString(height, &hud->lines[HUD_LINE_Height].value);
//}
//void resetMouseRawMovement(MouseButton *mouse_button) {
//    app->controls.mouse.pos_raw_diff.x = 0;
//    app->controls.mouse.pos_raw_diff.y = 0;
//}
//void toggleMouseCapturing(MouseButton *mouse_button) {
//    if (mouse_button == &app->controls.mouse.left_button) {
//        app->controls.mouse.is_captured = !app->controls.mouse.is_captured;
//        app->platform.setCursorVisibility(!app->controls.mouse.is_captured);
//        app->platform.setWindowCapture(    app->controls.mouse.is_captured);
//        resetMouseRawMovement(mouse_button);
//    }
//}
//void drawSceneMessage(Scene *scene, Viewport *viewport) {
//    f64 now = (f64)app->time.getTicks();
//    f64 tps = (f64)app->time.ticks.per_second;
//    if ((now - (f64)scene->last_io_ticks) / tps <= 2.0) {
//        PixelGrid *canvas = viewport->frame_buffer;
//        char *message;
//        RGBA color;
//        if (scene->last_io_is_save) {
//            message = (char*)"   Scene saved to: ";
//            color = Color(Yellow);
//        } else {
//            message = (char*)"Scene loaded from: ";
//            color = Color(Cyan);
//        }
//        i32 x = 10;
//        i32 y = 20;
//        drawText(canvas, color, message, x, y);
//        drawText(canvas, color, scene->settings.file.char_ptr, x + 100, y);
//    }
//}
//void drawSceneToViewport(Scene *scene, Viewport *viewport) {
//    fillPixelGrid(viewport->frame_buffer, Color(Black));
//    renderScene(scene, viewport);
//    if (viewport->settings.show_BVH) drawBVH(scene, viewport);
//    if (viewport->settings.show_SSB) drawSSB(scene, viewport);
//}

//void setupHUD(Viewport *viewport) {
//    Dimensions *dim = &viewport->frame_buffer->dimensions;
//    i32 bounces = (i32)viewport->trace.depth;
//    HUD *hud = &viewport->hud;
//    hud->line_height = 1.2f;
//    hud->position.x = hud->position.y = 10;
//    setDimensionsInHUD(hud, dim->width, dim->height);
//
//    HUDLine *line = hud->lines;
//    enum HUDLineID line_id;
//    for (u8 i = 0; i < (u8)hud->line_count; i++, line++) {
//        NumberString *num = &line->value;
//        String *str = &line->value.string;
//        String *alt = &line->alternate_value;
//
//        line_id = (enum HUDLineID)(i);
//        switch (line_id) {
//            case HUD_LINE_GPU:
//            case HUD_LINE_SSB:
//            case HUD_LINE_BVH:
//                setString(str, (char*)("On"));
//                setString(alt, (char*)("Off"));
//                line->alternate_value_color = DarkGreen;
//                line->invert_alternate_use = true;
//                line->use_alternate = line_id == HUD_LINE_BVH ? &viewport->settings.show_BVH :
//                                      line_id == HUD_LINE_SSB ? &viewport->settings.show_SSB :
//                                      &viewport->settings.use_GPU;
//                break;
//            case HUD_LINE_FPS:     printNumberIntoString(0, num); break;
//            case HUD_LINE_Width:   printNumberIntoString(dim->width, num); break;
//            case HUD_LINE_Height:  printNumberIntoString(dim->height, num); break;
//            case HUD_LINE_Bounces: printNumberIntoString(bounces, num); break;
//            case HUD_LINE_MatID:   printNumberIntoString(0, num); break;
//            case HUD_LINE_Mode:
//                setRenderModeString(viewport->settings.render_mode, str);
//                break;
//            default:
//                break;
//        }
//
//        str = &line->title;
//        switch (line_id) {
//            case HUD_LINE_FPS:     setString(str, (char*)"Fps    : "); break;
//            case HUD_LINE_GPU:     setString(str, (char*)"Use GPU: "); break;
//            case HUD_LINE_SSB:     setString(str, (char*)"ShowSSB: "); break;
//            case HUD_LINE_BVH:     setString(str, (char*)"ShowBVH: "); break;
//            case HUD_LINE_Width:   setString(str, (char*)"Width  : "); break;
//            case HUD_LINE_Height:  setString(str, (char*)"Height : "); break;
//            case HUD_LINE_Bounces: setString(str, (char*)"Bounces: "); break;
//            case HUD_LINE_MatID:   setString(str, (char*)"Mat. id: "); break;
//            case HUD_LINE_Mode:    setString(str, (char*)"Mode   : "); break;
//            default:
//                break;
//        }
//    }
//}

//void updateViewport(Viewport *viewport, Mouse *mouse) {
//    if (mouse->is_captured) {
//        if (mouse->moved)         orientViewport(viewport, mouse);
//        if (mouse->wheel_scrolled)  zoomViewport(viewport, mouse);
//    } else if (!(mouse->wheel_scrolled && app->controls.is_pressed.shift)) {
//        if (mouse->wheel_scrolled) dollyViewport(viewport, mouse);
//        if (mouse->moved) {
//            if (mouse->middle_button.is_pressed)
//                panViewport(viewport, mouse);
//
//            if (mouse->right_button.is_pressed &&
//                !app->controls.is_pressed.alt)
//                orbitViewport(viewport, mouse);
//        }
//    }
//
//    if (viewport->navigation.turned ||
//        viewport->navigation.moved ||
//        viewport->navigation.zoomed)
//        updateSceneSSB(&app->scene, viewport);
//}



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

//void updateViewportRenderMode(Viewport *viewport, u8 key) {
//    ViewportSettings *settings = &viewport->settings;
//    if (key == '1') settings->render_mode = RenderMode_Beauty;
//    if (key == '2') settings->render_mode = RenderMode_Depth;
//    if (key == '3') settings->render_mode = RenderMode_Normals;
//    if (key == '4') settings->render_mode = RenderMode_UVs;
//    if (key >= '1' &&
//        key <= '4')
//        setRenderModeString(settings->render_mode,
//                            &viewport->hud.lines[HUD_LINE_Mode].value.string);
//}
//
//void saveOrLoadScene(Scene *scene, Viewport *viewport, Platform *platform, bool save) {
//    char *file = scene->settings.file.char_ptr;
//    scene->last_io_is_save = save;
//    if (scene->last_io_is_save)
//        saveSceneToFile(  scene, file, platform);
//    else {
//        loadSceneFromFile(scene, file, platform);
//        updateScene(scene, viewport);
//    }
//    scene->last_io_ticks = platform->getTicks();
//}

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



//void onResize(u16 width, u16 height) {
//    setDimensionsInHUD(&app->viewport.hud, width, height);
//}