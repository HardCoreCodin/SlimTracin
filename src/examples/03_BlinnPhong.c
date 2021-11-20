#include "../SlimTracin/app.h"
#include "../SlimTracin/core/time.h"
#include "../SlimTracin/viewport/hud.h"
#include "../SlimTracin/viewport/navigation.h"
#include "../SlimTracin/viewport/manipulation.h"
#include "../SlimTracin/render/raytracer.h"

quat rotation;
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
    PRIM_COUNT
};
enum MATERIAL {
    MATERIAL_LAMBERT,
    MATERIAL_PHONG,
    MATERIAL_BLINN,
    MATERIAL_COUNT
};
enum HUD_LINE {
    HUD_LINE_SHADING,
    HUD_LINE_ROUGHNESS,
    HUD_LINE_COUNT
};
void updateSceneSelectionInHUD(Scene *scene, Viewport *viewport) {
    Primitive *prim = scene->selection->primitive;
    char* shader = (char*)"";
    if (prim) {
        switch (prim->material_id) {
            case MATERIAL_BLINN: shader = (char*)"Blinn"; break;
            case MATERIAL_PHONG: shader = (char*)"Phong"; break;
            default: shader = (char*)"Lambert"; break;
        }
        printFloatIntoString(scene->materials[prim->material_id].roughness,
                             &viewport->hud.lines[HUD_LINE_ROUGHNESS].value, 2);
    } else
        setString(&viewport->hud.lines[HUD_LINE_ROUGHNESS].value.string, (char*)"");
    setString(&viewport->hud.lines[HUD_LINE_SHADING].value.string, shader);
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
    bool prim_is_manipulated = controls->is_pressed.alt;

    beginFrame(timer);
        if (mouse->is_captured)
            navigateViewport(viewport, timer->delta_time);
        else
            manipulateSelection(scene, viewport, controls);

        if (scene->selection->changed) {
            scene->selection->changed = false;
            updateSceneSelectionInHUD(scene, viewport);
        }
        quat rot = Quat(rotation.axis, rotation.amount / timer->delta_time);
        scene->primitives[PRIM_BOX].rotation = normQuat(mulQuat(scene->primitives[PRIM_BOX].rotation, rot));
        scene->primitives[PRIM_TET].rotation = normQuat(mulQuat(scene->primitives[PRIM_TET].rotation, rot));
        uploadPrimitives(scene);

        if (!prim_is_manipulated)
            updateViewport(viewport, mouse);

        beginDrawing(viewport);
            renderScene(scene, viewport);
            drawSelection(scene, viewport, controls);
        endDrawing(viewport);
    endFrame(timer, mouse);
}
void onKeyChanged(u8 key, bool is_pressed) {
    Viewport *viewport = &app->viewport;
    ViewportSettings *settings = &viewport->settings;
    NavigationMove* move = &app->viewport.navigation.move;
    if (key == 'R') move->up       = is_pressed;
    if (key == 'F') move->down     = is_pressed;
    if (key == 'W') move->forward  = is_pressed;
    if (key == 'A') move->left     = is_pressed;
    if (key == 'S') move->backward = is_pressed;
    if (key == 'D') move->right    = is_pressed;

    Scene *scene = &app->scene;
    Primitive *prim = scene->selection->primitive;
    IsPressed pressed = app->controls.is_pressed;
    if (is_pressed) {
        if ((key == 'Z' || key == 'X') && prim) {
            char inc = key == 'X' ? 1 : -1;
            if (pressed.ctrl) {
                Material *M = scene->materials + prim->material_id;
                M->roughness = clampValueToBetween(M->roughness + (f32)inc * 0.05f, 0.05f, 1.0f);
                uploadMaterials(scene);
            } else {
                prim->material_id = (prim->material_id + inc + MATERIAL_COUNT) % MATERIAL_COUNT;
                uploadPrimitives(scene);
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
    setString(&lines[HUD_LINE_SHADING].title,  (char*)"Shading: ");
    setString(&lines[HUD_LINE_ROUGHNESS].title,(char*)"Roughness: ");
    setString(&lines[HUD_LINE_SHADING].value.string,  (char*)"Lambert");
    setString(&lines[HUD_LINE_ROUGHNESS].value.string,(char*)"");
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
        Primitive *floor  = scene->primitives + PRIM_FLOOR;
        Primitive *sphere = scene->primitives + PRIM_SPHERE;
        Primitive *box    = scene->primitives + PRIM_BOX;
        Primitive *tet    = scene->primitives + PRIM_TET;
        floor->type  = PrimitiveType_Quad;
        sphere->type = PrimitiveType_Sphere;
        box->type    = PrimitiveType_Box;
        tet->type    = PrimitiveType_Tetrahedron;
        box->material_id = MATERIAL_PHONG;
        tet->material_id = MATERIAL_PHONG;
        sphere->material_id = MATERIAL_BLINN;
        sphere->position = Vec3(3, 4, 0);
        box->position    = Vec3(-9, 5, 3);
        tet->position    = Vec3(-3, 4, 12);
        floor->scale     = Vec3(40, 1, 40);
        box->scale = tet->scale = sphere->scale = getVec3Of(2.5f);
        { // Rotate Primitives:
            vec3 X = Vec3(1, 0, 0);
            vec3 Y = Vec3(0, 1, 0);
            vec3 Z = Vec3(0, 0, 1);
            quat rot_x = getRotationAroundAxis(X, 0.02f);
            quat rot_y = getRotationAroundAxis(Y, 0.04f);
            quat rot_z = getRotationAroundAxis(Z, 0.06f);
            box->rotation = normQuat(mulQuat(rot_x, rot_y));
            tet->rotation = normQuat(mulQuat(box->rotation, rot_z));
            rotation = tet->rotation;
        }
    }
    { // Setup Materials:
        Material *lambert = scene->materials + MATERIAL_LAMBERT;
        Material *blinn   = scene->materials + MATERIAL_BLINN;
        Material *phong   = scene->materials + MATERIAL_PHONG;
        blinn->brdf = BRDF_Blinn;
        phong->brdf = BRDF_Phong;
        blinn->roughness = 0.5f;
        phong->roughness = 0.5f;
        lambert->albedo = Vec3(0.8f, 1.0f, 0.8f);
        blinn->albedo   = Vec3(1.0f, 0.3f, 1.0f);
        phong->albedo   = Vec3(0.2f, 0.2f, 0.3f);
        blinn->reflectivity = Vec3(1.0f, 0.4f, 1.0f);
        phong->reflectivity = Vec3(1.0f, 1.0f, 0.4f);
    }
}
void initApp(Defaults *defaults) {
    defaults->settings.scene.lights     = LIGHT_COUNT;
    defaults->settings.scene.materials  = MATERIAL_COUNT;
    defaults->settings.scene.primitives = PRIM_COUNT;
    defaults->settings.viewport.hud_line_count = HUD_LINE_COUNT;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}