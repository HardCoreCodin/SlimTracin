#include "../SlimRayTracer/app.h"
#include "../SlimRayTracer/viewport/hud.h"
#include "./_common.h"

void setupScene(Scene *scene) {
    setupLights(scene);
    setupMaterials(scene);

    // Back-right cube position:
    Primitive *primitive = scene->primitives;
    primitive->material_id = MaterialID_Reflective;
    primitive->position.x = 9;
    primitive->position.y = 4;
    primitive->position.z = 3;

    // Back-left cube position:
    primitive++;
    primitive->material_id = MaterialID_Phong;
    primitive->position.x = 10;
    primitive->position.z = 1;

    // Front-left cube position:
    primitive++;
    primitive->material_id = MaterialID_Refractive;
    primitive->position.x = -1;
    primitive->position.z = -5;

    // Front-right cube position:
    primitive++;
    primitive->material_id = MaterialID_Blinn;
    primitive->position.x = 10;
    primitive->position.z = -8;

    vec3 y_axis;
    y_axis.x = 0;
    y_axis.y = 1;
    y_axis.z = 0;
    quat rotation = getRotationAroundAxis(y_axis, 0.3f);
//    rotation = rotateAroundAxis(rotation, y_axis, 0.4f);
//    rotation = rotateAroundAxis(rotation, y_axis, 0.5f);

    u8 radius = 1;
    for (u8 i = 0; i < 4; i++, radius++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Box;
        primitive->scale = getVec3Of((f32)radius * 0.5f);
        primitive->position.x -= 4;
        primitive->position.z += 2;
        primitive->position.y = radius;
        primitive->rotation = rotation;
        rotation = mulQuat(rotation, rotation);
        primitive->rotation = getIdentityQuaternion();
    }

    // Back-right tetrahedron position:
    primitive++;
    primitive->material_id = MaterialID_Reflective;
    primitive->position.x = 3;
    primitive->position.y = 4;
    primitive->position.z = 8;

    // Back-left tetrahedron position:
    primitive++;
    primitive->material_id = MaterialID_Phong;
    primitive->position.x = 4;
    primitive->position.z = 6;

    // Front-left tetrahedron position:
    primitive++;
    primitive->material_id = MaterialID_Refractive;
    primitive->position.x = -3;
    primitive->position.z = 0;

    // Front-right tetrahedron position:
    primitive++;
    primitive->material_id = MaterialID_Blinn;
    primitive->position.x = 4;
    primitive->position.z = -3;

    radius = 1;

    rotation = getRotationAroundAxis(y_axis, 0.3f);
    for (u8 i = 4; i < 8; i++, radius++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Tetrahedron;
        primitive->scale = getVec3Of((f32)radius * 0.5f);
        primitive->position.x -= 4;
        primitive->position.z += 2;
        if (i > 4) primitive->position.y = radius;
        primitive->rotation = rotation;
        rotation = mulQuat(rotation, rotation);
    }

    // Back-left sphere:
    primitive++;
    primitive->material_id = 1;
    primitive->position.x = -1;
    primitive->position.z = 5;

    // Back-right sphere:
    primitive++;
    primitive->material_id = 2;
    primitive->position.x = 4;
    primitive->position.z = 6;

    // Front-left sphere:
    primitive++;
    primitive->material_id = 4;
    primitive->position.x = -3;
    primitive->position.z = 0;

    // Front-right sphere:
    primitive++;
    primitive->material_id = 5;
    primitive->position.x = 4;
    primitive->position.z = -8;

    radius = 1;
    for (u8 i = 8; i < 12; i++, radius++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Sphere;
        primitive->scale = getVec3Of(radius);
        primitive->position.y = radius;
        primitive->rotation = getIdentityQuaternion();
    }

    vec3 scale = getVec3Of(1);
    for (u8 i = 12; i < 18; i++) {
        primitive = scene->primitives + i;
        primitive->id = i;
        primitive->type = PrimitiveType_Quad;
        primitive->scale = scale;
        primitive->material_id = 0;
        primitive->position = getVec3Of(0);
        primitive->rotation = getIdentityQuaternion();
    }

    // Bottom quad:
    primitive = scene->primitives + 12;
    primitive->scale.x = 40;
    primitive->scale.z = 40;

    // Top quad:
    primitive++;
    primitive->scale.x = 40;
    primitive->scale.z = 40;
    primitive->position.y = 40;
    primitive->rotation.axis.x = 1;
    primitive->rotation.amount = 0;

    // Left quad:
    primitive++;
    primitive->scale.x = 20;
    primitive->scale.z = 40;
    primitive->position.x = -40;
    primitive->position.y = 20;
    primitive->rotation.axis.z = -HALF_SQRT2;
    primitive->rotation.amount = +HALF_SQRT2;

    // Right quad:
    primitive++;
    primitive->scale.x = 20;
    primitive->scale.z = 40;
    primitive->position.x = 40;
    primitive->position.y = 20;
    primitive->rotation.axis.z = HALF_SQRT2;
    primitive->rotation.amount = HALF_SQRT2;

    // Back quad:
    primitive++;
    primitive->scale.x = 40;
    primitive->scale.z = 20;
    primitive->position.z = -40;
    primitive->position.y = +20;
    primitive->rotation.axis.x = HALF_SQRT2;
    primitive->rotation.amount = HALF_SQRT2;

    // Front quad:
    primitive++;
    primitive->scale.x = 40;
    primitive->scale.z = 20;
    primitive->position.z = 40;
    primitive->position.y = 20;
    primitive->rotation.axis.x = -HALF_SQRT2;
    primitive->rotation.amount = +HALF_SQRT2;
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
    defaults->settings.scene.primitives   = 18;
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