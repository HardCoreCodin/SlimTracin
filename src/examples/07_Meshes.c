#include "../SlimTracin/app.h"
#include "../SlimTracin/core/time.h"
#include "../SlimTracin/core/string.h"
#include "../SlimTracin/viewport/navigation.h"
#include "../SlimTracin/viewport/manipulation.h"
#include "../SlimTracin/render/raytracer.h"

enum LIGHT {
    LIGHT_KEY,
    LIGHT_RIM,
    LIGHT_FILL,
    LIGHT_COUNT
};
enum PRIM {
    PRIM_FLOOR,
    PRIM_DOG,
    PRIM_DRAGON,
    PRIM_MONKEY,
    PRIM_COUNT
};
enum MESH {
    MESH_DOG,
    MESH_DRAGON,
    MESH_MONKEY,
    MESH_COUNT
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
    bool prim_is_manipulated = controls->is_pressed.alt;

    beginFrame(timer);
        static float elapsed = 0;
        elapsed += timer->delta_time;

        if (mouse->is_captured)
            navigateViewport(viewport, timer->delta_time);
        else
            manipulateSelection(scene, viewport, controls);

        if (!prim_is_manipulated)
            updateViewport(viewport, mouse);

        f32 dog_scale    = sinf(elapsed * 1.5f) * 0.02f;
        f32 dragon_scale = sinf(elapsed * 2.5f) * 0.04f;
        f32 monkey_scale = sinf(elapsed * 2.5f + 1) * 0.05f;
        Primitive *dog    = scene->primitives + PRIM_DOG;
        Primitive *dragon = scene->primitives + PRIM_DRAGON;
        Primitive *monkey = scene->primitives + PRIM_MONKEY;

        dog->scale    = getVec3Of(0.4f + dog_scale);
        dragon->scale = getVec3Of(0.4f - dragon_scale);
        monkey->scale = getVec3Of(0.3f + monkey_scale);
        dog->scale.y    += 2 * dog_scale;
        dragon->scale.y += 2 * dragon_scale;
        monkey->scale.y -= 2 * monkey_scale;
        quat rot = getRotationAroundAxis(Vec3(0, 1, 0),
                                         timer->delta_time * 0.015f);
        if (!(prim_is_manipulated && dragon == scene->selection->primitive))
            dragon->rotation = normQuat(mulQuat(dragon->rotation, rot));
        rot.amount *= -0.5f;

        if (!(prim_is_manipulated && monkey == scene->selection->primitive))
            monkey->rotation = normQuat(mulQuat(monkey->rotation, rot));
        rot.amount = -rot.amount;
        if (!(prim_is_manipulated && dog == scene->selection->primitive))
            dog->rotation = normQuat(mulQuat(dog->rotation, rot));

        uploadPrimitives(scene);

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
    { // Setup Camera:
        Camera *camera = scene->cameras;
        camera->transform.position.y = 7;
        camera->transform.position.z = -11;
        rotateXform3(&camera->transform, 0, -0.2f, 0);
    }
    { // Setup Lights:
        scene->ambient_light.color = Vec3(0.004f, 0.004f, 0.007f);
        Light *key = scene->lights + LIGHT_KEY;
        Light *rim = scene->lights + LIGHT_RIM;
        Light *fill = scene->lights + LIGHT_FILL;
        key->intensity  = 1.1f * 40.0f;
        rim->intensity  = 1.2f * 40.0f;
        fill->intensity = 0.9f * 40.0f;
        key->color  = Vec3(1.0f, 1.0f, 0.65f);
        rim->color  = Vec3(1.0f, 0.25f, 0.25f);
        fill->color = Vec3(0.65f, 0.65f, 1.0f );
        key->position_or_direction  = Vec3(10, 10, -5);
        rim->position_or_direction  = Vec3(2, 5, 12);
        fill->position_or_direction = Vec3(-10, 10, -5);
    }
    { // Setup Materials:
        scene->materials->roughness = 0.5f;
        scene->materials->brdf = BRDF_CookTorrance;
    }
    { // Setup Primitives:
        Primitive *floor  = scene->primitives + PRIM_FLOOR;
        Primitive *dog    = scene->primitives + PRIM_DOG;
        Primitive *dragon = scene->primitives + PRIM_DRAGON;
        Primitive *monkey = scene->primitives + PRIM_MONKEY;
        floor->type  = PrimitiveType_Quad;
        floor->scale = Vec3(40, 1, 40);
        dog->type    = PrimitiveType_Mesh;
        dragon->type = PrimitiveType_Mesh;
        monkey->type = PrimitiveType_Mesh;
        dog->id    = MESH_DOG;
        dragon->id = MESH_DRAGON;
        monkey->id = MESH_MONKEY;
        dog->position    = Vec3( 4, 2, 8);
        monkey->position = Vec3(-4, 1.2f, 8);
        dragon->position = Vec3( 0, 2, -1);
    }
}
void initApp(Defaults *defaults) {
    static String mesh_files[MESH_COUNT];
    static char string_buffers[MESH_COUNT][100];
    char* this_file   = (char*)__FILE__;
    char* dog_file    = (char*)"dog.mesh";
    char* dragon_file = (char*)"dragon.mesh";
    char* monkey_file = (char*)"monkey.mesh";
    String *dog    = &mesh_files[MESH_DOG];
    String *dragon = &mesh_files[MESH_DRAGON];
    String *monkey = &mesh_files[MESH_MONKEY];
    dog->char_ptr    = string_buffers[MESH_DOG];
    dragon->char_ptr = string_buffers[MESH_DRAGON];
    monkey->char_ptr = string_buffers[MESH_MONKEY];
    u32 dir_len = getDirectoryLength(this_file);
    mergeString(dog,    this_file, dog_file,    dir_len);
    mergeString(dragon, this_file, dragon_file, dir_len);
    mergeString(monkey, this_file, monkey_file, dir_len);
    defaults->settings.scene.mesh_files = mesh_files;
    defaults->settings.scene.meshes     = MESH_COUNT;
    defaults->settings.scene.lights     = LIGHT_COUNT;
    defaults->settings.scene.primitives = PRIM_COUNT;
    defaults->settings.scene.materials  = 1;
    app->on.sceneReady    = setupScene;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}