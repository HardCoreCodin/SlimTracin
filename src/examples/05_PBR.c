#include "../SlimTracin/app.h"
#include "../SlimTracin/core/time.h"
#include "../SlimTracin/viewport/navigation.h"
#include "../SlimTracin/viewport/manipulation.h"
#include "../SlimTracin/render/raytracer.h"

#define GRID_SPACING 2.5f
#define GRID_SIZE 7
#define GRID_SIZE_F ((float)GRID_SIZE)
#define LIGHT_COUNT 4

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
    scene->cameras->transform.position.z = -11;
    { // Setup Lights:
        Light *light = scene->lights;
        for (u8 i = 0; i < 4; i++, light++) {
            light->intensity = 300;
            light->position_or_direction.x = i % 2 ? -10 : +10;
            light->position_or_direction.y = i < 2 ? +10 : -20;
            light->position_or_direction.z = -10;
        }
    }
    { // Setup Materials and Primitives:
        Material *mat = scene->materials;
        Primitive *prim = scene->primitives;
        vec3 plastic = getVec3Of(0.04f);
        for (u8 row = 0; row < GRID_SIZE; row++) {
            for (u8 col = 0; col < GRID_SIZE; col++, mat++, prim++) {
                prim->type = PrimitiveType_Sphere;
                prim->position.x = ((f32)col - (GRID_SIZE_F / 2.0f)) * GRID_SPACING;
                prim->position.y = ((f32)row - (GRID_SIZE_F / 2.0f)) * GRID_SPACING;
                prim->material_id = prim->id;
                mat->brdf = BRDF_CookTorrance;
                mat->metallic  = (f32)row / GRID_SIZE_F;
                mat->roughness = (f32)col / GRID_SIZE_F;
                mat->albedo = Vec3(0.5f, 0.0f, 0.0f);
                mat->reflectivity = lerpVec3(plastic, mat->albedo, mat->metallic);
                mat->roughness = clampValueToBetween(mat->roughness, 0.05f, 1.0f);
            }
        }
    }
}
void initApp(Defaults *defaults) {
    defaults->settings.scene.lights     = LIGHT_COUNT;
    defaults->settings.scene.materials  = GRID_SIZE * GRID_SIZE;
    defaults->settings.scene.primitives = GRID_SIZE * GRID_SIZE;
    app->on.sceneReady    = setupScene;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}