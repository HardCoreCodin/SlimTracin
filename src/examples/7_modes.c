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
    Timer *timer = &app->time.timers.update;
    startFrameTimer(timer);
    Viewport *viewport = &app->viewport;
    Controls *controls = &app->controls;
    Mouse *mouse = &controls->mouse;
    Scene *scene = &app->scene;

    if (mouse->is_captured)
        navigateViewport(viewport, timer->delta_time);
    else
        manipulateSelection(scene, viewport, controls);

    if (!controls->is_pressed.alt)
        updateViewport(viewport, mouse);

    renderScene(scene, viewport, controls);

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
        u8 tab = app->controls.key_map.tab;
        if (key == tab) settings->show_hud = !settings->show_hud;
        if (key == '9') settings->show_BVH = !settings->show_BVH;
        if (key == '0') settings->show_SSB = !settings->show_SSB;
        if (key == '1') settings->render_mode = RenderMode_Beauty;
        if (key == '2') settings->render_mode = RenderMode_Depth;
        if (key == '3') settings->render_mode = RenderMode_Normals;
        if (key == '4') settings->render_mode = RenderMode_UVs;
        if (key >= '1' && key <= '4') {
            char *str;
            switch (settings->render_mode) {
                case RenderMode_Beauty : str = "Beauty";  break;
                case RenderMode_Normals: str = "Normals"; break;
                case RenderMode_Depth  : str = "Depth";   break;
                case RenderMode_UVs    : str = "UVs";     break;
                default: break;
            }
            setString(&viewport->hud.lines[0].value.string, str);
        }
    }
}
void setupViewport(Viewport *viewport) {
    setupCamera(viewport->camera);
    HUD *hud = &viewport->hud;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    HUDLine *line = hud->lines;
    for (u8 i = 0; i < (u8)hud->line_count; i++, line++) {
        String *alt = &line->alternate_value;
        String *str = &line->value.string;
        setString(str, "On");
        setString(alt, "Off");
        if (i) {
            line->alternate_value_color = Grey;
            line->invert_alternate_use = true;
            line->use_alternate = i == 1 ?
                                  &viewport->settings.show_BVH :
                                  &viewport->settings.show_SSB;
        } else setString(str, "Beauty");
        str = &line->title;
        switch (i) {
            case 0: setString(str, "Mode: "); break;
            case 1: setString(str, "BVH : "); break;
            case 2: setString(str, "SSB : "); break;
            default: break;
        }
    }
}
void setupScene(Scene *scene) {
    setupLights(scene);
    vec3 X = Vec3(1, 0, 0);
    vec3 Y = Vec3(0, 1, 0);
    vec3 Z = Vec3(0, 0, 1);
    quat rot_x = getRotationAroundAxis(X, 0.02f);
    quat rot_y = getRotationAroundAxis(Y, 0.04f);
    quat rot_z = getRotationAroundAxis(Z, 0.06f);
    Primitive *box = &scene->primitives[0];
    Primitive *tet = &scene->primitives[1];
    Primitive *spr = &scene->primitives[2];
    Primitive *dragon  = &scene->primitives[3];
    Primitive *monkey1 = &scene->primitives[4];
    Primitive *monkey2 = &scene->primitives[5];
    Primitive *floor   = &scene->primitives[6];
    floor->type   = PrimitiveType_Quad;
    dragon->type  = PrimitiveType_Mesh;
    monkey1->type = PrimitiveType_Mesh;
    monkey2->type = PrimitiveType_Mesh;
    dragon->id    = monkey2->id = 0;
    monkey1->id   = monkey2->id = 1;
    monkey2->id   = monkey2->id = 1;
    monkey1->position = Vec3( 4, 1, 8);
    monkey2->position = Vec3(-4, 1, 8);
    dragon->position  = Vec3( 0, 2, -1);
    floor->scale      = Vec3(40, 1, 40);
    dragon->scale     = getVec3Of(0.4f);
    box->type = PrimitiveType_Box;
    tet->type = PrimitiveType_Tetrahedron;
    spr->type = PrimitiveType_Sphere;
    spr->position = Vec3( 5, 3, 0);
    box->position = Vec3(-9, 3, 3);
    tet->position = Vec3(-3, 4, 12);
    box->rotation = normQuat(mulQuat(rot_x, rot_y));
    tet->rotation = normQuat(mulQuat(box->rotation, rot_z));
}
void initApp(Defaults *defaults) {
    static String mesh_files[2];
    static char string_buffers[2][100];
    char* this_file   = __FILE__;
    char* dragon_file = "dragon.mesh";
    char* monkey_file = "suzanne.mesh";
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
    defaults->settings.scene.primitives = 7;
    defaults->settings.scene.materials  = 1;
    defaults->settings.viewport.hud_line_count = 3;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}