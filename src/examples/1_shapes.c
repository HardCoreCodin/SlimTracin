#include "../SlimRayTracer/app.h"
#include "../SlimRayTracer/viewport/hud.h"
#include "./_common.h"

quat rotation, x_rotation, y_rotation, z_rotation;

void onUpdate(Scene *scene, f32 delta_time) {
    quat adj_rotation = rotation;
    adj_rotation.amount /= delta_time;

    Primitive *primitive = scene->primitives;
    primitive->rotation = normQuat(mulQuat(primitive->rotation, adj_rotation));
    primitive++;
    primitive->rotation = normQuat(mulQuat(primitive->rotation, adj_rotation));
}

void setupScene(Scene *scene) {
    setupLights(scene);
    setupMaterials(scene);

    vec3 axis;
    axis.x = 1;
    axis.y = 0;
    axis.z = 0;
    x_rotation = getRotationAroundAxis(axis, 0.02f);
    axis.x = 0;
    axis.y = 1;
    y_rotation = getRotationAroundAxis(axis, 0.04f);
    axis.y = 0;
    axis.z = 1;
    z_rotation = getRotationAroundAxis(axis, 0.06f);

    rotation = normQuat(mulQuat(x_rotation, y_rotation));

    Primitive *primitive = scene->primitives;
    primitive->type = PrimitiveType_Box;
    primitive->position.x = -9;
    primitive->position.y = 3;
    primitive->position.z = 3;
    primitive->rotation = rotation;
    primitive->scale = getVec3Of(2.5f);
    primitive->material_id = MaterialID_Refractive;

    rotation = normQuat(mulQuat(rotation, z_rotation));

    primitive++;
    primitive->type = PrimitiveType_Tetrahedron;
    primitive->position.x = -3;
    primitive->position.y = 4;
    primitive->position.z = 12;
    primitive->rotation = rotation;
    primitive->scale = getVec3Of(2.5f);
    primitive->material_id = MaterialID_Refractive;

    primitive++;
    primitive->type = PrimitiveType_Sphere;
    primitive->scale = getVec3Of(2.5f);
    primitive->position.x = 3;
    primitive->position.y = 3;
    primitive->material_id = MaterialID_Refractive;

    primitive++;
    primitive->type = PrimitiveType_Quad;
    primitive->scale.x = 40;
    primitive->scale.z = 40;
}

char string_buffer[100];
void initApp(Defaults *defaults) {
    char* this_file   = (char*)__FILE__;
    char* scene_file  = (char*)"shapes.scene";
    String *scene  = &defaults->settings.scene.file;
    scene->char_ptr  = string_buffer;

    u32 dir_len = getDirectoryLength(this_file);
    mergeString(scene,  this_file, scene_file,  dir_len);

    defaults->settings.scene.lights = 3;
    defaults->settings.scene.materials    = 6;
    defaults->settings.scene.primitives   = 4;
    defaults->settings.viewport.hud_line_count = 9;
    defaults->settings.viewport.hud_default_color = Green;

    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onButtonDown;
    app->on.mouseButtonDoubleClicked = onDoubleClick;
    app->on.windowResize  = onResize;
    app->on.windowRedraw  = updateAndRender;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;

    scene_io_time = 0;
}