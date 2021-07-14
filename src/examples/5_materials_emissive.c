#include "../SlimRayTracer/app.h"
#include "../SlimRayTracer/core/time.h"
#include "../SlimRayTracer/viewport/hud.h"
#include "../SlimRayTracer/viewport/navigation.h"
#include "../SlimRayTracer/viewport/manipulation.h"

void setupCamera(Camera *camera) {
    camera->transform.position.y = 7;
    camera->transform.position.z = -11;
    rotateXform3(&camera->transform, 0, -0.2f, 0);
}
void setupLights(Scene *scene) {
    scene->ambient_light.color.x = 0.008f;
    scene->ambient_light.color.y = 0.008f;
    scene->ambient_light.color.z = 0.014f;

    Light *key_light  = &scene->lights[0];
    Light *fill_light = &scene->lights[1];
    Light *rim_light  = &scene->lights[2];
    key_light->intensity  = 1.3f * 8;
    rim_light->intensity  = 1.5f * 8;
    fill_light->intensity = 1.1f * 8;
    key_light->color  = Vec3(1.0f,  1.0f,  0.65f);
    rim_light->color  = Vec3(1.0f,  0.25f, 0.25f);
    fill_light->color = Vec3(0.65f, 0.65f, 1.0f );
    key_light->position_or_direction  = Vec3( 10, 10, -5);
    rim_light->position_or_direction  = Vec3(  2,  5, 12);
    fill_light->position_or_direction = Vec3(-10, 10, -5);
}
void resetMouseRawMovement(Mouse *mouse) {
    mouse->pos_raw_diff.x = 0;
    mouse->pos_raw_diff.y = 0;
}
void toggleMouseCapturing(Mouse *mouse, Platform *platform) {
    mouse->is_captured = !mouse->is_captured;
    platform->setCursorVisibility(!mouse->is_captured);
    platform->setWindowCapture(    mouse->is_captured);
    resetMouseRawMovement(mouse);
}
void updateNavigation(NavigationMove *move, u8 key, bool is_pressed) {
    if (key == 'R') move->up       = is_pressed;
    if (key == 'F') move->down     = is_pressed;
    if (key == 'W') move->forward  = is_pressed;
    if (key == 'A') move->left     = is_pressed;
    if (key == 'S') move->backward = is_pressed;
    if (key == 'D') move->right    = is_pressed;
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
void setupShapes(Scene *scene) {
    Primitive *floor = &scene->primitives[0];
    Primitive *wall1 = &scene->primitives[1];
    Primitive *wall2 = &scene->primitives[2];
    Primitive *shpr1 = &scene->primitives[3];
    Primitive *shpr2 = &scene->primitives[4];
    shpr1->type = PrimitiveType_Sphere;
    shpr2->type = PrimitiveType_Sphere;
    floor->type = PrimitiveType_Quad;
    wall1->type = PrimitiveType_Quad;
    wall2->type = PrimitiveType_Quad;
    wall1->rotation.axis.z = -HALF_SQRT2;
    wall1->rotation.amount = +HALF_SQRT2;
    wall2->rotation.axis.z = +HALF_SQRT2;
    wall2->rotation.amount = -HALF_SQRT2;
    shpr1->position = Vec3( +3, 3, 0);
    shpr2->position = Vec3( -3, 3, 0);
    wall1->position = Vec3(-15, 5, 5);
    wall2->position = Vec3(+15, 5, 5);
    floor->scale    = Vec3(40, 1, 40);
    wall1->scale    = Vec3( 4, 1, 8 );
    wall2->scale    = Vec3( 4, 1, 8 );
}
void updateSelection(Scene *scene, Viewport *viewport,
                     Controls *controls) {
    Primitive *selected = scene->selection->primitive;
    if (!selected) return;
    Mouse *mouse = &controls->mouse;
    HUDLine *lines = viewport->hud.lines;
    u8 last_material = scene->settings.materials - 1;
    if ((u32)(fabsf(mouse->wheel_scroll_amount)) / 100) {
        mouse->wheel_scroll_handled = true;
        if (controls->is_pressed.shift) {
            if (mouse->wheel_scroll_amount < 0) {
                if (selected->material_id)
                    selected->material_id--;
                else
                    selected->material_id = last_material;
            } else {
                if (selected->material_id == last_material)
                    selected->material_id = 0;
                else
                    selected->material_id++;
            }
            setString(&lines[0].value.string,
                      selected->material_id ?
                      "Emissive" : "Lambert");
        }
    }
}
void updateAndRender() {
    Timer *timer = &app->time.timers.update;
    startFrameTimer(timer);
    f32 dt = timer->delta_time;
    Scene *scene       = &app->scene;
    Viewport *viewport = &app->viewport;
    Controls *controls = &app->controls;
    Mouse *mouse = &controls->mouse;
    if (mouse->is_captured)
        navigateViewport(viewport, dt);
    else
        manipulateSelection(scene, viewport, controls);

    if (!(controls->is_pressed.alt ||
          controls->is_pressed.ctrl ||
          controls->is_pressed.shift))
        updateViewport(viewport, mouse);

    if (mouse->wheel_scrolled)
        updateSelection(scene, viewport, controls);

    fillPixelGrid(viewport->frame_buffer, Color(Black));
    renderScene(scene, viewport);

    drawSelection(scene, viewport, controls);
    if (viewport->settings.show_hud)
        drawHUD(viewport->frame_buffer, &viewport->hud);

    resetMouseChanges(mouse);
    endFrameTimer(timer);
}
void onMouseButtonDown(MouseButton *mouse_button) {
    resetMouseRawMovement(&app->controls.mouse);
}
void onMouseDoubleClicked(MouseButton *mouse_button) {
    Mouse *mouse = &app->controls.mouse;
    if (mouse_button == &mouse->left_button)
        toggleMouseCapturing(mouse, &app->platform);
}

void onKeyChanged(u8 key, bool is_pressed) {
    Viewport *viewport = &app->viewport;
    updateNavigation(&viewport->navigation.move, key, is_pressed);

    if (!is_pressed) {
        ViewportSettings *settings = &viewport->settings;
        if (key == app->controls.key_map.tab)
            settings->show_hud = !settings->show_hud;

        Primitive *selected = app->scene.selection->primitive;
        String *s = &viewport->hud.lines->value.string;
        if (key == 'T' && selected) {
            if (selected->flags & IS_TRANSPARENT) {
                selected->flags &= ~IS_TRANSPARENT;
                setString(s, (char*)("Off"));
            } else {
                setString(s, (char*) ("On"));
                selected->flags |= IS_TRANSPARENT;
            }
        }
    }
}
void setupViewport(Viewport *viewport) {
    setupCamera(viewport->camera);
    HUD *hud = &viewport->hud;
    HUDLine *lines = hud->lines;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    setString(&lines[0].title, "Shader     : ");
    setString(&lines[1].title, "Transparent: ");
    setString(&lines[1].value.string, "Off");
    setString(&lines[0].value.string,"Lambert");
}
void setupScene(Scene *scene) {
    setupShapes(scene);
    setupLights(scene);
    Material *mat = scene->materials;
    mat[0].flags = LAMBERT;
    mat[1].flags = EMISSION;
    mat[2].flags = EMISSION;
    mat[0].diffuse  = Vec3(0.8f, 1.0f, 0.8f);
    mat[1].emission = Vec3(0.9f, 0.7f, 0.7f);
    mat[2].emission = Vec3(0.7f, 0.7f, 0.9f);
}
void initApp(Defaults *defaults) {
    defaults->settings.scene.lights     = 3;
    defaults->settings.scene.materials  = 3;
    defaults->settings.scene.primitives = 5;
    defaults->settings.viewport.hud_line_count = 2;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}