#include "../SlimTracin/app.h"
#include "../SlimTracin/core/time.h"
#include "../SlimTracin/viewport/navigation.h"
#include "../SlimTracin/viewport/manipulation.h"
#include "../SlimTracin/render/raytracer.h"

enum LIGHT {
    LIGHT_KEY,
    LIGHT_RIM,
    LIGHT_FILL,
    LIGHT_COUNT
};
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
    Scene *scene       = &app->scene;
    Viewport *viewport = &app->viewport;
    Controls *controls = &app->controls;
    Mouse *mouse = &controls->mouse;

    beginFrame(timer);
        if (mouse->is_captured)
            navigateViewport(viewport, timer->delta_time);
        else
            manipulateSelection(scene, viewport, controls);

        if (!controls->is_pressed.alt)
            updateViewport(viewport, mouse);

        beginDrawing(viewport);
            renderScene(scene, viewport);
            drawSelection(scene, viewport, controls);
        endDrawing(viewport);
    endFrame(timer, mouse);
}
void onMouseButtonDown(MouseButton *mouse_button) {
    app->controls.mouse.pos_raw_diff = Vec2i(0, 0);
}
void onMouseDoubleClicked(MouseButton *mouse_button) {
    Mouse *mouse = &app->controls.mouse;
    if (mouse_button == &mouse->left_button) {
        mouse->is_captured = !mouse->is_captured;
        mouse->pos_raw_diff = Vec2i(0, 0);
        app->platform.setCursorVisibility(!mouse->is_captured);
        app->platform.setWindowCapture(    mouse->is_captured);
    }
}
void onKeyChanged(u8 key, bool is_pressed) {
    NavigationMove* move = &app->viewport.navigation.move;
    if (key == 'R') move->up       = is_pressed;
    if (key == 'F') move->down     = is_pressed;
    if (key == 'W') move->forward  = is_pressed;
    if (key == 'A') move->left     = is_pressed;
    if (key == 'S') move->backward = is_pressed;
    if (key == 'D') move->right    = is_pressed;
}
void setupScene(Scene *scene) {
    scene->ambient_light.color = Vec3(0.004f, 0.004f, 0.007f);
    { // Setup Camera:
        Camera *camera = scene->cameras;
        camera->transform.position.y = 7;
        camera->transform.position.z = -11;
        rotateXform3(&camera->transform, 0, -0.2f, 0);
    }
    { // Setup Lights:
        Light *key  = scene->lights + LIGHT_KEY;
        Light *rim  = scene->lights + LIGHT_RIM;
        Light *fill = scene->lights + LIGHT_FILL;
        key->intensity  = 3.3f * 40.0f;
        rim->intensity  = 3.5f * 40.0f;
        fill->intensity = 3.1f * 40.0f;
        key->color  = Vec3(1.0f, 1.0f, 0.65f);
        rim->color  = Vec3(1.0f, 0.25f, 0.25f);
        fill->color = Vec3(0.65f, 0.65f, 1.0f );
        key->position_or_direction  = Vec3(10, 10, -5);
        rim->position_or_direction  = Vec3(2, 5, 12);
        fill->position_or_direction = Vec3(-10, 10, -5);
    }
    { // Setup Primitives:
        Primitive *plane = scene->primitives;
        plane->type = PrimitiveType_Quad;
        plane->scale = Vec3(40, 1, 40);
    }
    { // Setup Materials:
        scene->materials->brdf = BRDF_CookTorrance;
        scene->materials->roughness = 0.5f;
    }
}
void initApp(Defaults *defaults) {
    defaults->settings.scene.lights     = LIGHT_COUNT;
    defaults->settings.scene.materials  = 1;
    defaults->settings.scene.primitives = 1;
    app->on.sceneReady    = setupScene;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}