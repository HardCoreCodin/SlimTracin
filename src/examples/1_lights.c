#include "../SlimRayTracer/app.h"
#include "./_common.h"

void updateAndRender() {
    Timer *timer = &app->time.timers.update;
    startFrameTimer(timer);

    Scene *scene       = &app->scene;
    Viewport *viewport = &app->viewport;
    Controls *controls = &app->controls;
    Mouse *mouse = &controls->mouse;
    if (mouse->is_captured)
        navigateViewport(viewport, timer->delta_time);
    else
        manipulateSelection(scene, viewport, controls);

    if (!controls->is_pressed.alt)
        updateViewport(viewport, mouse);

    fillPixelGrid(viewport->frame_buffer, Color(Black));
    renderScene(scene, viewport);
    drawSelection(scene, viewport, controls);

    resetMouseChanges(mouse);
    endFrameTimer(timer);
}

void onKeyChanged(u8 key, bool is_pressed) {
    NavigationMove *move = &app->viewport.navigation.move;
    if (key == 'R') move->up       = is_pressed;
    if (key == 'F') move->down     = is_pressed;
    if (key == 'W') move->forward  = is_pressed;
    if (key == 'A') move->left     = is_pressed;
    if (key == 'S') move->backward = is_pressed;
    if (key == 'D') move->right    = is_pressed;
}

void setupViewport(Viewport *viewport) {
    viewport->trace.depth = 4;
    viewport->camera->transform.position.y = 7;
    viewport->camera->transform.position.z = -11;
    rotateXform3(&viewport->camera->transform, 0, -0.2f, 0);
}

void setupScene(Scene *scene) {
    Primitive *plane = scene->primitives;
    plane->type = PrimitiveType_Quad;
    plane->scale = Vec3(40, 1, 40);

    Light *key_light  = &scene->lights[0];
    Light *fill_light = &scene->lights[1];
    Light *rim_light  = &scene->lights[2];
    key_light->intensity  = 1.3f * 4;
    rim_light->intensity  = 1.5f * 4;
    fill_light->intensity = 1.1f * 4;
    key_light->color  = Vec3(1.0f,  1.0f,  0.65f);
    rim_light->color  = Vec3(1.0f,  0.25f, 0.25f);
    fill_light->color = Vec3(0.65f, 0.65f, 1.0f );
    key_light->position_or_direction  = Vec3( 10, 10, -5);
    rim_light->position_or_direction  = Vec3(  2,  5, 12);
    fill_light->position_or_direction = Vec3(-10, 10, -5);
}
void initApp(Defaults *defaults) {
    defaults->settings.scene.lights     = 3;
    defaults->settings.scene.materials  = 1;
    defaults->settings.scene.primitives = 1;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onButtonDown;
    app->on.mouseButtonDoubleClicked = onDoubleClick;
}