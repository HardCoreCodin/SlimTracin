#include "../SlimTracin/app.h"
#include "../SlimTracin/core/time.h"
#include "../SlimTracin/viewport/hud.h"
#include "../SlimTracin/viewport/navigation.h"
#include "../SlimTracin/viewport/manipulation.h"
#include "../SlimTracin/render/raytracer.h"

quat rotation;

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
    key_light->intensity  = 1.3f * 8;
    rim_light->intensity  = 1.5f * 8;
    fill_light->intensity = 1.1f * 8;
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
void updateSelection(Scene *scene, Viewport *viewport, Controls *controls) {
    Primitive *selected = scene->selection->primitive;
    if (!selected) return;
    Mouse *mouse = &controls->mouse;
    HUDLine *lines = viewport->hud.lines;
    Material *material = &scene->materials[selected->material_id];
    u8 last_material = scene->settings.materials - 1;
    if ((u32)(fabsf(mouse->wheel_scroll_amount)) / 100) {
        mouse->wheel_scroll_handled = true;
        if (controls->is_pressed.shift) {
            if (mouse->wheel_scroll_amount < 0) {
                if (selected->material_id)
                    selected->material_id--;
                else
                    selected->material_id = last_material;
            } else {
                if (selected->material_id == last_material)
                    selected->material_id = 0;
                else
                    selected->material_id++;
            }
            char *shader;
            switch (selected->material_id) {
                case 1 : shader = "Blinn";   break;
                case 2 : shader = "Phong";   break;
                default: shader = "Lambert"; break;
            }
            setString(&lines[0].value.string, shader);
        } else if (controls->is_pressed.ctrl) {
            if (mouse->wheel_scroll_amount < 0) {
                if (material->shininess > 1)
                    material->shininess--;
            } else material->shininess++;
        }
        printNumberIntoString((u16)material->shininess, &lines[1].value);
    }
}
void updateAndRender() {
    Timer *timer = &app->time.timers.update;
    startFrameTimer(timer);
    f32 dt = timer->delta_time;
    Scene *scene       = &app->scene;
    Viewport *viewport = &app->viewport;
    Controls *controls = &app->controls;
    Mouse *mouse = &controls->mouse;
    if (mouse->is_captured)
        navigateViewport(viewport, dt);
    else
        manipulateSelection(scene, viewport, controls);

    if (!(controls->is_pressed.alt ||
          controls->is_pressed.ctrl ||
          controls->is_pressed.shift))
        updateViewport(viewport, mouse);

    if (mouse->wheel_scrolled)
        updateSelection(scene, viewport, controls);

    quat rot = Quat(rotation.axis, rotation.amount / dt);
    Primitive *prim = scene->primitives;
    for (u8 i = 0; i < 3; i++, prim++)
        prim->rotation = normQuat(mulQuat(prim->rotation, rot));

    renderScene(scene, viewport, controls);

    resetMouseChanges(mouse);
    endFrameTimer(timer);
}

void setupShapes(Scene *scene) {
    vec3 X = Vec3(1, 0, 0);
    vec3 Y = Vec3(0, 1, 0);
    vec3 Z = Vec3(0, 0, 1);
    quat rot_x = getRotationAroundAxis(X, 0.02f);
    quat rot_y = getRotationAroundAxis(Y, 0.04f);
    quat rot_z = getRotationAroundAxis(Z, 0.06f);
    Primitive *box = &scene->primitives[0];
    Primitive *tet = &scene->primitives[1];
    Primitive *spr = &scene->primitives[2];
    Primitive *qud = &scene->primitives[3];
    box->type = PrimitiveType_Box;
    tet->type = PrimitiveType_Tetrahedron;
    spr->type = PrimitiveType_Sphere;
    qud->type = PrimitiveType_Quad;
    spr->position = Vec3( 3, 3, 0 );
    box->position = Vec3(-9, 3, 3 );
    tet->position = Vec3(-3, 4, 12);
    qud->scale    = Vec3(40, 1, 40);
    box->scale = tet->scale = spr->scale = getVec3Of(2.5f);
    box->rotation = normQuat(mulQuat(rot_x, rot_y));
    tet->rotation = normQuat(mulQuat(box->rotation, rot_z));
    rotation = tet->rotation;
}
void onKeyChanged(u8 key, bool is_pressed) {
    Viewport *viewport = &app->viewport;
    updateNavigation(&viewport->navigation.move, key, is_pressed);

    if (!is_pressed) {
        ViewportSettings *settings = &viewport->settings;
        if (key == app->controls.key_map.tab)
            settings->show_hud = !settings->show_hud;
        else if (key == app->controls.key_map.ctrl ||
                 key == app->controls.key_map.shift) {
            app->controls.mouse.wheel_scroll_amount = 0;
            app->controls.mouse.wheel_scrolled = false;
        }
    }
}
void onMouseButtonDown(MouseButton *mouse_button) {
    resetMouseRawMovement(&app->controls.mouse);
}
void onMouseDoubleClicked(MouseButton *mouse_button) {
    Mouse *mouse = &app->controls.mouse;
    if (mouse_button == &mouse->left_button)
        toggleMouseCapturing(mouse, &app->platform);
}
void setupViewport(Viewport *viewport) {
    setupCamera(viewport->camera);
    HUD *hud = &viewport->hud;
    HUDLine *lines = hud->lines;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    setString(&lines[0].value.string,"Lambert");
    setString(&lines[0].title, "Shader: ");
    setString(&lines[1].title, "Shiny : ");
    printNumberIntoString(0, &hud->lines[1].value);
}
void setupScene(Scene *scene) {
    setupShapes(scene);
    setupLights(scene);
    Material *mat = scene->materials;
    mat[0].flags = LAMBERT;
    mat[0].diffuse = Vec3(0.8f, 1, 0.8f);
    mat[1].flags = BLINN | LAMBERT;
    mat[2].flags = PHONG | LAMBERT;
    mat[1].diffuse = getVec3Of(0.1f);
    mat[2].diffuse = getVec3Of(0.2f);
    mat[2].diffuse.z  = mat[1].diffuse.y  = 0.3f;
    mat[2].specular.z = mat[1].specular.y = 0.4f;
}
void initApp(Defaults *defaults) {
    defaults->settings.scene.lights     = 3;
    defaults->settings.scene.materials  = 3;
    defaults->settings.scene.primitives = 4;
    defaults->settings.viewport.hud_line_count = 2;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onMouseButtonDown;
    app->on.mouseButtonDoubleClicked = onMouseDoubleClicked;
}