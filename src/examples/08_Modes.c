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
    PRIM_BOX,
    PRIM_TET,
    PRIM_SPHERE,
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
enum HUD_LINE {
    HUD_LINE_MODE,
    HUD_LINE_BVH,
    HUD_LINE_SSB,
    HUD_LINE_COUNT
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
            if (viewport->settings.show_BVH) drawBVH(scene, viewport);
            if (viewport->settings.show_SSB) drawSSB(scene, viewport);
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
    Viewport *viewport = &app->viewport;
    NavigationMove* move = &app->viewport.navigation.move;
    if (key == 'R') move->up       = is_pressed;
    if (key == 'F') move->down     = is_pressed;
    if (key == 'W') move->forward  = is_pressed;
    if (key == 'A') move->left     = is_pressed;
    if (key == 'S') move->backward = is_pressed;
    if (key == 'D') move->right    = is_pressed;
    if (!is_pressed) {
        ViewportSettings *settings = &viewport->settings;
        u8 tab = app->controls.key_map.tab;
        if (key == tab) settings->show_hud = !settings->show_hud;
        if (key == 'X') settings->show_BVH = !settings->show_BVH;
        if (key == 'Z') settings->show_SSB = !settings->show_SSB;
        if (key == '1') settings->render_mode = RenderMode_Beauty;
        if (key == '2') settings->render_mode = RenderMode_Depth;
        if (key == '3') settings->render_mode = RenderMode_Normals;
        if (key == '4') settings->render_mode = RenderMode_UVs;
        if (key >= '1' && key <= '4') {
            char *str;
            switch (settings->render_mode) {
                case RenderMode_Beauty : str = (char*)"Beauty";  break;
                case RenderMode_Normals: str = (char*)"Normals"; break;
                case RenderMode_Depth  : str = (char*)"Depth";   break;
                case RenderMode_UVs    : str = (char*)"UVs";     break;
                default: break;
            }
            setString(&viewport->hud.lines[HUD_LINE_MODE].value.string, str);
        }
    }
}
void setupViewport(Viewport *viewport) {
    HUD *hud = &viewport->hud;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    HUDLine *line = hud->lines;
    for (u8 i = 0; i < (u8)hud->line_count; i++, line++) {
        String *alt = &line->alternate_value;
        String *str = &line->value.string;
        setString(str, (char*)"On");
        setString(alt, (char*)"Off");
        if (i) {
            line->alternate_value_color = Grey;
            line->invert_alternate_use = true;
            line->use_alternate = i == 1 ?
                                  &viewport->settings.show_BVH :
                                  &viewport->settings.show_SSB;
        } else setString(str, (char*)"Beauty");
        str = &line->title;
        switch (i) {
            case HUD_LINE_MODE: setString(str, (char*)"Mode: "); break;
            case HUD_LINE_BVH : setString(str, (char*)"BVH : "); break;
            case HUD_LINE_SSB : setString(str, (char*)"SSB : "); break;
            default: break;
        }
    }
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
        Primitive *box = scene->primitives + PRIM_BOX;
        Primitive *tet = scene->primitives + PRIM_TET;
        Primitive *sphere = scene->primitives + PRIM_SPHERE;
        floor->type  = PrimitiveType_Quad;
        dog->type    = PrimitiveType_Mesh;
        dragon->type = PrimitiveType_Mesh;
        monkey->type = PrimitiveType_Mesh;
        dog->id    = MESH_DOG;
        dragon->id = MESH_DRAGON;
        monkey->id = MESH_MONKEY;
        dog->scale    = getVec3Of(0.4f);
        dragon->scale = getVec3Of(0.4f);
        monkey->scale = getVec3Of(0.3f);
        floor->scale = Vec3(40, 1, 40);
        dog->position    = Vec3( 4, 2, 8);
        monkey->position = Vec3(-4, 1.2f, 8);
        dragon->position = Vec3( 0, 2, -1);
        box->type = PrimitiveType_Box;
        tet->type = PrimitiveType_Tetrahedron;
        sphere->type = PrimitiveType_Sphere;
        sphere->position = Vec3( 3, 3, 0);
        box->position = Vec3(-9, 3, 3);
        tet->position = Vec3(-3, 4, 12);
        { // Rotate Primitives:
            vec3 X = Vec3(1, 0, 0);
            vec3 Y = Vec3(0, 1, 0);
            vec3 Z = Vec3(0, 0, 1);
            quat rot_x = getRotationAroundAxis(X, 0.02f);
            quat rot_y = getRotationAroundAxis(Y, 0.04f);
            quat rot_z = getRotationAroundAxis(Z, 0.06f);
            box->rotation = normQuat(mulQuat(rot_x, rot_y));
            tet->rotation = normQuat(mulQuat(box->rotation, rot_z));
        }
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
    defaults->settings.viewport.hud_line_count = HUD_LINE_COUNT;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}