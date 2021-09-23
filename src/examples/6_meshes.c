#include "../SlimTracin/app.h"
#include "../SlimTracin/core/time.h"
#include "../SlimTracin/core/string.h"
#include "../SlimTracin/viewport/navigation.h"
#include "../SlimTracin/viewport/manipulation.h"
#include "../SlimTracin/render/raytracer.h"

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
void updateAndRender() {
    startFrameTimer(&app->time.timers.update);
    Controls *controls = &app->controls;
    Mouse *mouse       = &controls->mouse;
    Scene *scene       = &app->scene;
    Viewport *viewport = &app->viewport;
    Primitive *dragon  = &scene->primitives[0];
    Primitive *monkey1 = &scene->primitives[1];
    Primitive *monkey2 = &scene->primitives[2];

    f32 dt = app->time.timers.update.delta_time;
    static float elapsed = 0;
    elapsed += dt;
    f32 dragon_scale = sinf(elapsed * 2.5f) * 0.04f;
    f32 monkey_scale = sinf(elapsed * 3.5f + 1) * 0.15f;
    dragon->scale = getVec3Of(0.4f - dragon_scale);
    monkey1->scale = getVec3Of(1.0f + monkey_scale);
    dragon->scale.y  += 2 * dragon_scale;
    monkey1->scale.y -= 2 * monkey_scale;
    monkey2->scale = monkey1->scale;
    vec3 Y = Vec3(0, 1, 0);
    quat rot = getRotationAroundAxis(Y, 0.0015f * dt);
    dragon->rotation = normQuat(mulQuat(dragon->rotation, rot));
    rot.amount *= -0.5f;
    monkey1->rotation = normQuat(mulQuat(monkey1->rotation, rot));
    rot.amount = -rot.amount;
    monkey2->rotation = normQuat(mulQuat(monkey2->rotation, rot));

    if (mouse->is_captured) navigateViewport(viewport, dt);
    else manipulateSelection(scene, viewport, controls);
    if (!controls->is_pressed.alt) updateViewport(viewport, mouse);

    renderScene(scene, viewport, controls);

    resetMouseChanges(mouse);
    endFrameTimer(&app->time.timers.update);
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
    updateNavigation(&app->viewport.navigation.move, key, is_pressed);
}
void setupViewport(Viewport *viewport) {
    setupCamera(viewport->camera);
}
void setupScene(Scene *scene) {
    setupLights(scene);
    Primitive *dragon  = &scene->primitives[0];
    Primitive *monkey1 = &scene->primitives[1];
    Primitive *monkey2 = &scene->primitives[2];
    Primitive *floor   = &scene->primitives[3];
    floor->type   = PrimitiveType_Quad;
    dragon->type  = PrimitiveType_Mesh;
    monkey1->type = PrimitiveType_Mesh;
    monkey2->type = PrimitiveType_Mesh;
    monkey1->id   = monkey2->id = 1;
    monkey1->position = Vec3( 4, 1, 8);
    monkey2->position = Vec3(-4, 1, 8);
    dragon->position  = Vec3( 0, 2, -1);
    floor->scale      = Vec3(40, 1, 40);
}
void initApp(Defaults *defaults) {
    static String mesh_files[2];
    static char string_buffers[2][100];
    char* this_file   = __FILE__;
    char* monkey_file = "suzanne.mesh";
    char* dragon_file = "dragon.mesh";
    String *dragon = &mesh_files[0];
    String *monkey = &mesh_files[1];
    dragon->char_ptr = string_buffers[0];
    monkey->char_ptr = string_buffers[1];
    u32 dir_len = getDirectoryLength(this_file);
    mergeString(monkey, this_file, monkey_file, dir_len);
    mergeString(dragon, this_file, dragon_file, dir_len);
    defaults->settings.scene.meshes = 2;
    defaults->settings.scene.mesh_files = mesh_files;
    defaults->settings.scene.lights     = 3;
    defaults->settings.scene.primitives = 4;
    defaults->settings.scene.materials  = 1;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}