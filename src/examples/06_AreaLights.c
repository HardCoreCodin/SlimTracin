#include "../SlimTracin/app.h"
#include "../SlimTracin/core/time.h"
#include "../SlimTracin/viewport/hud.h"
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
    PRIM_LIGHT1,
    PRIM_LIGHT2,
    PRIM_SPHERE1,
    PRIM_SPHERE2,
    PRIM_COUNT
};
enum MATERIAL {
    MATERIAL_ROUGH,
    MATERIAL_LIGHT1,
    MATERIAL_LIGHT2,
    MATERIAL_COUNT
};
enum HUD_LINE {
    HUD_LINE_SHADING,
    HUD_LINE_TRANSPARENT,
    HUD_LINE_COUNT
};
void updateSceneSelectionInHUD(Scene *scene, Viewport *viewport) {
    Primitive *prim = scene->selection->primitive;
    char* shader = (char*)"";
    char* transparent = (char*)"";
    if (prim) {
        shader = prim->material_id ? (char*)"Lambert" : (char*)"Emissive";
        transparent = prim->flags & IS_TRANSPARENT ? (char*)("On") : (char*)("Off");
    }
    setString(&viewport->hud.lines[HUD_LINE_SHADING].value.string, shader);
    setString(&viewport->hud.lines[HUD_LINE_TRANSPARENT].value.string, transparent);
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
    Scene *scene       = &app->scene;
    Viewport *viewport = &app->viewport;
    Controls *controls = &app->controls;
    Mouse *mouse = &controls->mouse;

    beginFrame(timer);
        if (mouse->is_captured)
            navigateViewport(viewport, timer->delta_time);
        else
            manipulateSelection(scene, viewport, controls);

        if (scene->selection->changed) {
            scene->selection->changed = false;
            updateSceneSelectionInHUD(scene, viewport);
        }
        if (!(controls->is_pressed.alt))
            updateViewport(viewport, mouse);

        beginDrawing(viewport);
            renderScene(scene, viewport);
            drawSelection(scene, viewport, controls);
        endDrawing(viewport);
    endFrame(timer, mouse);
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
    Scene *scene = &app->scene;
    Primitive *prim = scene->selection->primitive;
    ViewportSettings *settings = &viewport->settings;
    if (is_pressed) {
        if (prim) {
            if (key == 'Z' || key == 'X') {
                char inc = key == 'X' ? 1 : -1;
                prim->material_id = (prim->material_id + inc + MATERIAL_COUNT) % MATERIAL_COUNT;
                uploadPrimitives(scene);
            } else if (key == 'T') {
                if (prim->flags & IS_TRANSPARENT)
                    prim->flags &= ~IS_TRANSPARENT;
                else
                    prim->flags |= IS_TRANSPARENT;
                uploadMaterials(scene);
            }
            updateSceneSelectionInHUD(scene, viewport);
        }
    } else if (key == app->controls.key_map.tab) settings->show_hud = !settings->show_hud;
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
void setupViewport(Viewport *viewport) {
    HUD *hud = &viewport->hud;
    HUDLine *lines = hud->lines;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    setString(&lines[HUD_LINE_SHADING].title,     (char*)"Shading    : ");
    setString(&lines[HUD_LINE_TRANSPARENT].title, (char*)"Transparent: ");
    setString(&lines[HUD_LINE_SHADING].value.string,(char*)"");
    setString(&lines[HUD_LINE_TRANSPARENT].value.string, (char*)"Off");
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
    { // Setup Primitives:
        Primitive *floor   = scene->primitives + PRIM_FLOOR;
        Primitive *light1  = scene->primitives + PRIM_LIGHT1;
        Primitive *light2  = scene->primitives + PRIM_LIGHT2;
        Primitive *sphere1 = scene->primitives + PRIM_SPHERE1;
        Primitive *sphere2 = scene->primitives + PRIM_SPHERE2;
        sphere1->type = PrimitiveType_Sphere;
        sphere2->type = PrimitiveType_Sphere;
        floor->type   = PrimitiveType_Quad;
        light1->type  = PrimitiveType_Quad;
        light2->type  = PrimitiveType_Quad;
        light1->material_id = MATERIAL_LIGHT1;
        light2->material_id = MATERIAL_LIGHT2;
        light1->rotation.axis.z = -HALF_SQRT2;
        light1->rotation.amount = +HALF_SQRT2;
        light2->rotation.axis.z = -HALF_SQRT2;
        light2->rotation.amount = -HALF_SQRT2;
        sphere1->position = Vec3(+3, 3, 0);
        sphere2->position = Vec3(-3, 3, 0);
        light1->position = Vec3(-15, 5, 5);
        light2->position = Vec3(+15, 5, 5);
        floor->scale     = Vec3(40, 1, 40);
        light1->scale    = Vec3(4, 1, 8 );
        light2->scale    = Vec3(4, 1, 8 );
    }
    { // Setup Materials:
        Material *rough  = scene->materials + MATERIAL_ROUGH;
        Material *light1 = scene->materials + MATERIAL_LIGHT1;
        Material *light2 = scene->materials + MATERIAL_LIGHT2;
        light1->is = EMISSIVE;
        light2->is = EMISSIVE;
        light1->emission = Vec3(0.9f, 0.7f, 0.7f);
        light2->emission = Vec3(0.7f, 0.7f, 0.9f);
        rough->albedo  = Vec3(0.8f, 1.0f, 0.8f);
    }
}
void initApp(Defaults *defaults) {
    defaults->settings.scene.materials  = MATERIAL_COUNT;
    defaults->settings.scene.lights     = LIGHT_COUNT;
    defaults->settings.scene.primitives = PRIM_COUNT;
    defaults->settings.viewport.hud_line_count = HUD_LINE_COUNT;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}