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
    app->on.viewportReady = setupCamera;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = updateNavigation;
    app->on.mouseButtonDown          = resetMouseRawMovement;
    app->on.mouseButtonDoubleClicked = toggleMouseCapturing;
}