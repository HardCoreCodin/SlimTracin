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
    PRIM_WALL,
    PRIM_COUNT
};
enum MATERIAL {
    MATERIAL_ROUGH,
    MATERIAL_MIRROR,
    MATERIAL_GLASS,
    MATERIAL_COUNT
};
enum HUD_LINE {
    HUD_LINE_SHADING,
    HUD_LINE_BOUNCES,
    HUD_LINE_COUNT
};
void updateSceneSelectionInHUD(Scene *scene, Viewport *viewport) {
    Primitive *prim = scene->selection->primitive;
    HUDLine *lines = viewport->hud.lines;
    NumberString *shading = &lines[HUD_LINE_SHADING].value;
    NumberString *bounces = &lines[HUD_LINE_BOUNCES].value;
    char* shader = "";
    if (prim) {
        switch (prim->material_id) {
            case MATERIAL_MIRROR : shader = "Mirror";  break;
            case MATERIAL_GLASS  : shader = "Glass";   break;
            default: shader = "Lambert"; break;
        }
    }
    setString(&shading->string, shader);
    printNumberIntoString((i32)(viewport->trace.depth), bounces);
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
        if (key == 'Z' || key == 'X') {
            char inc = key == 'X' ? 1 : -1;
            if (pressed.ctrl) {
                viewport->trace.depth += inc;
                if (viewport->trace.depth == 0) viewport->trace.depth = 5;
                if (viewport->trace.depth == 6) viewport->trace.depth = 1;
            } else if (prim) {
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
    setString(&lines[HUD_LINE_SHADING].value.string,(char*)"");
    setString(&lines[HUD_LINE_SHADING].title, (char*)"Shading: ");
    setString(&lines[HUD_LINE_BOUNCES].title, (char*)"Bounces: ");
    setString(&lines[HUD_LINE_BOUNCES].value.string, (char*)"4");
    viewport->trace.depth = 4;
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
        key->intensity = 1.1f * 40.0f;
        rim->intensity = 1.2f * 40.0f;
        fill->intensity = 0.9f * 40.0f;
        key->color = Vec3(1.0f, 1.0f, 0.65f);
        rim->color = Vec3(1.0f, 0.25f, 0.25f);
        fill->color = Vec3(0.65f, 0.65f, 1.0f);
        key->position_or_direction = Vec3(10, 10, -5);
        rim->position_or_direction = Vec3(2, 5, 12);
        fill->position_or_direction = Vec3(-10, 10, -5);
    }
    { // Setup Primitives:
        Primitive *box    = scene->primitives + PRIM_BOX;
        Primitive *tet    = scene->primitives + PRIM_TET;
        Primitive *sphere = scene->primitives + PRIM_SPHERE;
        Primitive *floor  = scene->primitives + PRIM_FLOOR;
        Primitive *wall   = scene->primitives + PRIM_WALL;
        box->type    = PrimitiveType_Box;
        tet->type    = PrimitiveType_Tetrahedron;
        sphere->type = PrimitiveType_Sphere;
        floor->type  = PrimitiveType_Quad;
        wall->type   = PrimitiveType_Quad;
        wall->position   = Vec3(-15, 5, 5);
        sphere->position = Vec3(3, 3, 0 );
        box->position    = Vec3(-9, 3, 3 );
        tet->position    = Vec3(-3, 4, 12);
        floor->scale     = Vec3(40, 1, 40);
        wall->scale      = Vec3(4, 1, 8 );
        box->scale = tet->scale = sphere->scale = getVec3Of(2.5f);
        wall->rotation.axis.z = -HALF_SQRT2;
        wall->rotation.amount = +HALF_SQRT2;
        wall->material_id   = MATERIAL_MIRROR;
        sphere->material_id = MATERIAL_GLASS;
        box->material_id    = MATERIAL_GLASS;
        tet->material_id    = MATERIAL_GLASS;
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
        Material *rough  = scene->materials + MATERIAL_ROUGH;
        Material *mirror = scene->materials + MATERIAL_MIRROR;
        Material *glass  = scene->materials + MATERIAL_GLASS;
        glass->is = REFRACTIVE;
        glass->n1_over_n2 = IOR_AIR / IOR_GLASS;
        glass->n2_over_n1 = IOR_GLASS / IOR_AIR;
        glass->reflectivity = Vec3(0.7f, 0.9f, 0.7f);
        glass->roughness = 0;
        glass->brdf = BRDF_Blinn;

        mirror->is = REFLECTIVE;
        mirror->reflectivity = getVec3Of(0.9f);
        mirror->roughness = 0;
        mirror->brdf = BRDF_Blinn;

        rough->albedo = Vec3(0.8f, 1.0f, 0.8f);
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