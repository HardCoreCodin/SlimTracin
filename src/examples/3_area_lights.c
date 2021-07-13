#include "../SlimRayTracer/app.h"
#include "../SlimRayTracer/viewport/hud.h"
#include "./_common.h"

void onUpdate(Scene *scene, f32 delta_time) {}
void setupScene(Scene *scene) {
//    setupLights(scene);
    setupMaterials(scene);

    Primitive *primitive = scene->primitives;
    primitive->type = PrimitiveType_Sphere;
    primitive->position.x = -9;
    primitive->position.y = 3;
    primitive->position.z = 3;
    primitive->scale = getVec3Of(2.5f);
    primitive->material_id = MaterialID_Blinn;

    primitive++;
    primitive->type = PrimitiveType_Sphere;
    primitive->position.x = -3;
    primitive->position.y = 4;
    primitive->position.z = 12;
    primitive->scale = getVec3Of(3.5f);
    primitive->material_id = MaterialID_Reflective;

    primitive++;
    primitive->type = PrimitiveType_Sphere;
    primitive->scale = getVec3Of(4.5f);
    primitive->position.x = 3;
    primitive->position.y = 5;
    primitive->material_id = MaterialID_Refractive;

    primitive++;
    primitive->type = PrimitiveType_Quad;
    primitive->scale.x = 40;
    primitive->scale.z = 40;

    // Left quad:
    primitive++;
    primitive->type = PrimitiveType_Quad;
    primitive->scale.x = 4;
    primitive->scale.z = 8;
    primitive->position.x = -20;
    primitive->position.y = 5;
    primitive->rotation.axis.z = -HALF_SQRT2;
    primitive->rotation.amount = +HALF_SQRT2;
    primitive->material_id = MaterialID_Emissive;

    // Left quad:
    primitive++;
    primitive->type = PrimitiveType_Quad;
    primitive->scale.x = 2;
    primitive->scale.z = 4;
    primitive->position.x = 10;
    primitive->position.y = 5;
    primitive->rotation.axis.z = -HALF_SQRT2;
    primitive->rotation.amount = +HALF_SQRT2;
//    primitive->material_id = MaterialID_Emissive;
}

char string_buffer[100];
void initApp(Defaults *defaults) {
    char* this_file   = (char*)__FILE__;
    char* scene_file  = (char*)"area_lights.scene";
    String *scene  = &defaults->settings.scene.file;
    scene->char_ptr  = string_buffer;

    u32 dir_len = getDirectoryLength(this_file);
    mergeString(scene,  this_file, scene_file,  dir_len);

    defaults->settings.scene.lights = 0;
    defaults->settings.scene.materials    = 7;
    defaults->settings.scene.primitives   = 6;
    defaults->settings.viewport.hud_line_count = 9;
    defaults->settings.viewport.hud_default_color = Green;

    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = resetMouseRawMovement;
    app->on.mouseButtonDoubleClicked = toggleMouseCapturing;
    app->on.windowResize  = onResize;
    app->on.windowRedraw  = updateAndRender;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;

    scene_io_time = 0;
}