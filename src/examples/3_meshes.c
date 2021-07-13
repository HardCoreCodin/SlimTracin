#include "../SlimRayTracer/app.h"
#include "./_common.h"

void updateAndRender() {
    startFrameTimer(&app->time.timers.update);
    f32 dt = app->time.timers.update.delta_time;
    static float elapsed = 0;
    elapsed += dt;
    f32 dragon_scale = sinf(elapsed * 2.5f) * 0.04f;
    f32 monkey_scale = sinf(elapsed * 3.5f + 1) * 0.15f;
    Primitive *dragon  = &app->scene.primitives[0];
    Primitive *monkey1 = &app->scene.primitives[1];
    Primitive *monkey2 = &app->scene.primitives[2];
    dragon->scale = getVec3Of(0.4f - dragon_scale);
    monkey1->scale = getVec3Of(1.0f + monkey_scale);
    dragon->scale.y  += 2 * dragon_scale;
    monkey1->scale.y -= 2 * monkey_scale;
    monkey2->scale = monkey1->scale;
    vec3 Y = Vec3(0, 1, 0);
    quat rot = getRotationAroundAxis(Y, 0.0015f / dt);
    dragon->rotation = normQuat(mulQuat(dragon->rotation, rot));
    rot.amount *= -0.5f;
    monkey1->rotation = normQuat(mulQuat(monkey1->rotation, rot));
    rot.amount = -rot.amount;
    monkey2->rotation = normQuat(mulQuat(monkey2->rotation, rot));

    if (app->controls.mouse.is_captured)
        navigateViewport(&app->viewport, dt);
    else
        manipulateSelection(&app->scene, &app->viewport, &app->controls);
    if (!app->controls.is_pressed.alt)
        updateViewport(&app->viewport, &app->controls.mouse);

    fillPixelGrid(app->viewport.frame_buffer, Color(Black));
    renderScene(&app->scene, &app->viewport);
    drawSelection(&app->scene, &app->viewport, &app->controls);
    resetMouseChanges(&app->controls.mouse);
    endFrameTimer(&app->time.timers.update);
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
    static String file_paths[2];
    static char string_buffers[3][100];
    char* this_file   = (char*)__FILE__;
    char* monkey_file = (char*)"suzanne.mesh";
    char* dragon_file = (char*)"dragon.mesh";
    String *dragon = &file_paths[0];
    String *monkey = &file_paths[1];
    dragon->char_ptr = string_buffers[0];
    monkey->char_ptr = string_buffers[1];
    u32 dir_len = getDirectoryLength(this_file);
    mergeString(monkey, this_file, monkey_file, dir_len);
    mergeString(dragon, this_file, dragon_file, dir_len);
    defaults->settings.scene.meshes = 2;
    defaults->settings.scene.mesh_files = file_paths;
    defaults->settings.scene.lights     = 3;
    defaults->settings.scene.primitives = 4;
    defaults->settings.scene.materials  = 1;
    app->on.windowRedraw  = updateAndRender;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupCamera;
    app->on.keyChanged               = updateNavigation;
    app->on.mouseButtonDown          = resetMouseRawMovement;
    app->on.mouseButtonDoubleClicked = toggleMouseCapturing;
}

//setupMaterials(scene);
//    box->material_id    = MaterialID_Refractive;
//    tet->material_id    = MaterialID_Refractive;
//    sphere->material_id = MaterialID_Refractive;