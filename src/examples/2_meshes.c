#include "../SlimRayTracer/app.h"
#include "../SlimRayTracer/viewport/hud.h"
#include "./_common.h"

void setupScene(Scene *scene) {
    setupLights(scene);
    setupMaterials(scene);

    Primitive *primitive;
    vec3 scale = getVec3Of(1);
    for (u8 i = 0; i < 2; i++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Quad;
        primitive->scale = scale;
        primitive->material_id = 0;
        primitive->position = getVec3Of(0);
        primitive->rotation = getIdentityQuaternion();
    }

    // Bottom quad:
    primitive = scene->primitives;
    primitive->scale.x = 40;
    primitive->scale.z = 40;

    // Left quad:
    primitive++;
    primitive->scale.x = 20;
    primitive->scale.z = 40;
    primitive->position.x = -40;
    primitive->position.y = 20;
    primitive->rotation.axis.z = -HALF_SQRT2;
    primitive->rotation.amount = +HALF_SQRT2;

    // Suzanne 1:
    primitive++;
    primitive->id = 0;
    primitive->position.x = 10;
    primitive->position.z = 5;
    primitive->position.y = 4;
    primitive->type = PrimitiveType_Mesh;
    primitive->material_id = MaterialID_Refractive;
    primitive->color = Magenta;

    primitive++;
    *primitive = *(primitive - 1);
    primitive->position.x = -10;
    primitive->color = Cyan;

    primitive++;
    *primitive = *(primitive - 1);
    primitive->id = 1;
    primitive->position.z = 10;
    primitive->position.y = 8;
    primitive->color = Blue;
}

String file_paths[2];
char string_buffers[3][100];
void initApp(Defaults *defaults) {
    char* this_file   = (char*)__FILE__;
    char* monkey_file = (char*)"suzanne.mesh";
    char* dragon_file = (char*)"dragon.mesh";
    char* scene_file  = (char*)"meshes.scene";

    String *scene  = &defaults->settings.scene.file;
    String *monkey = &file_paths[0];
    String *dragon = &file_paths[1];

    monkey->char_ptr = string_buffers[0];
    dragon->char_ptr = string_buffers[1];
    scene->char_ptr  = string_buffers[2];

    u32 dir_len = getDirectoryLength(this_file);
    mergeString(monkey, this_file, monkey_file, dir_len);
    mergeString(dragon, this_file, dragon_file, dir_len);
    mergeString(scene,  this_file, scene_file,  dir_len);

    defaults->settings.scene.meshes = 2;
    defaults->settings.scene.mesh_files = file_paths;
    defaults->settings.scene.lights = 3;
    defaults->settings.scene.materials    = 6;
    defaults->settings.scene.primitives   = 5;
    defaults->settings.viewport.hud_line_count = 9;
    defaults->settings.viewport.hud_default_color = Green;

    app->on.keyChanged               = onKeyChanged;
    app->on.mouseButtonDown          = onButtonDown;
    app->on.mouseButtonDoubleClicked = onDoubleClick;
    app->on.windowResize  = onResize;
    app->on.windowRedraw  = updateAndRender;
    app->on.sceneReady    = setupScene;
    app->on.viewportReady = setupViewport;
}
void onUpdate(Scene *scene, f32 delta_time) {}