#include "../SlimRayTracer/app.h"
#include "./_common.h"

quat rotation;
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

    if (!controls->is_pressed.alt)
        updateViewport(viewport, mouse);

    quat rot = Quat(rotation.axis, rotation.amount / dt);
    Primitive *prim = scene->primitives;
    for (u8 i = 0; i < 3; i++, prim++)
        prim->rotation = normQuat(mulQuat(prim->rotation, rot));

    fillPixelGrid(viewport->frame_buffer, Color(Black));
    renderScene(scene, viewport);
    drawSelection(scene, viewport, controls);

    resetMouseChanges(mouse);
    endFrameTimer(timer);
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
    Primitive *qud = &scene->primitives[3];
    box->type = PrimitiveType_Box;
    tet->type = PrimitiveType_Tetrahedron;
    spr->type = PrimitiveType_Sphere;
    qud->type = PrimitiveType_Quad;
    spr->position = Vec3( 3, 3, 0);
    box->position = Vec3(-9, 3, 3);
    tet->position = Vec3(-3, 4, 12);
    qud->scale    = Vec3(40, 1, 40);
    box->scale = tet->scale = spr->scale = getVec3Of(2.5f);
    box->rotation = normQuat(mulQuat(rot_x, rot_y));
    tet->rotation = normQuat(mulQuat(box->rotation, rot_z));
    rotation = tet->rotation;
}
void initApp(Defaults *defaults) {
    defaults->settings.scene.lights     = 3;
    defaults->settings.scene.primitives = 4;
    defaults->settings.scene.materials  = 1;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupCamera;
    app->on.windowRedraw  = updateAndRender;
    app->on.keyChanged               = updateNavigation;
    app->on.mouseButtonDown          = resetMouseRawMovement;
    app->on.mouseButtonDoubleClicked = toggleMouseCapturing;
}

//setupMaterials(scene);
//    box->material_id    = MaterialID_Refractive;
//    tet->material_id    = MaterialID_Refractive;
//    sphere->material_id = MaterialID_Refractive;