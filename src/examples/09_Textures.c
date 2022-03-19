#include "../SlimTracin/app.h"
#include "../SlimTracin/core/time.h"
#include "../SlimTracin/core/string.h"
#include "../SlimTracin/viewport/navigation.h"
#include "../SlimTracin/viewport/manipulation.h"
#include "../SlimTracin/render/raytracer.h"

enum LIGHT {
    LIGHT_KEY,
    LIGHT_RIM,
    LIGHT_FILL,
    LIGHT_COUNT
};
enum HUD_LINE {
    HUD_LINE_FPS,
    HUD_LINE_XPU,
    HUD_LINE_MODE,
    HUD_LINE_BVH,
    HUD_LINE_PRIM,
    HUD_LINE_MESH,
    HUD_LINE_MATERIAL,
    HUD_LINE_ALBEDO,
    HUD_LINE_NORMAL,
    HUD_LINE_UV_REPEAT,
    HUD_LINE_ROUGHNESS,
    HUD_LINE_BOUNCES,
    HUD_LINE_COUNT
};
enum MATERIAL {
    MATERIAL_PBR,
    MATERIAL_FLOOR,
    MATERIAL_DOG,
    MATERIAL_MIRROR,
    MATERIAL_GLASS,
    MATERIAL_COUNT
};
enum MESH {
    MESH_MONKEY,
    MESH_DRAGON,
    MESH_DOG,
    MESH_COUNT
};
enum PRIM {
    PRIM_FLOOR,
    PRIM_BOX,
    PRIM_TET,
    PRIM_SPHERE,
    PRIM_MESH,
    PRIM_COUNT
};
enum TEXTURE {
    TEXTURE_FLOOR_ALBEDO,
    TEXTURE_FLOOR_NORMAL,
    TEXTURE_DOG_ALBEDO,
    TEXTURE_DOG_NORMAL,
    TEXTURE_COUNT
};
#define nextMat(selected, inc) ((selected)->material_id = ((selected)->material_id + (inc) + MATERIAL_COUNT) % MATERIAL_COUNT)

void updateSelectionInHUD(Selection *selection, HUDLine *lines) {
    Primitive *prim = selection->primitive;
    if (!prim) {
        setString(&lines[HUD_LINE_MESH].value.string, (char*)"");
        setString(&lines[HUD_LINE_MATERIAL].value.string, (char*)"");
        setString(&lines[HUD_LINE_UV_REPEAT].value.string, (char*)"");
        setString(&lines[HUD_LINE_PRIM].value.string, selection->object_type == PrimitiveType_Light ? (char*)"Light" : (char*)"");
        return;
    }

    setString(&lines[HUD_LINE_ROUGHNESS].value.string, (char*)"");
    char *str;
    Material *M = app->scene.materials + prim->material_id;
    printNumberIntoString((i32)M->uv_repeat.u, &lines[HUD_LINE_UV_REPEAT].value);
    switch (prim->material_id) {
        case MATERIAL_MIRROR : str = (char*)"Mirror"; break;
        case MATERIAL_GLASS : str = (char*)"Glass"; break;
        default:
            str = (char*)"PBR";
            printFloatIntoString(M->roughness, &lines[HUD_LINE_ROUGHNESS].value, 2);
    }
    setString(&lines[HUD_LINE_MATERIAL].value.string, str);
    setString(&lines[HUD_LINE_ALBEDO].value.string, M->use & ALBEDO_MAP ? (char*)"On" : (char*)"Off");
    setString(&lines[HUD_LINE_NORMAL].value.string, M->use & NORMAL_MAP ? (char*)"On" : (char*)"Off");
    lines[HUD_LINE_ALBEDO].value_color = M->use & ALBEDO_MAP ? DarkGreen : DarkRed;
    lines[HUD_LINE_NORMAL].value_color = M->use & NORMAL_MAP ? DarkGreen : DarkRed;

    if (prim->type == PrimitiveType_Mesh) {
        switch(prim->id) {
            case MESH_DOG:    str = (char*)"Dog"; break;
            case MESH_DRAGON: str = (char*)"Dragon"; break;
            case MESH_MONKEY: str = (char*)"Monkey"; break;
        }
        setString(&lines[HUD_LINE_MESH].value.string, str);
        setString(&lines[HUD_LINE_PRIM].value.string, (char*)"Mesh");
    } else {
        switch(prim->type) {
            case PrimitiveType_Box        : str = (char*)"Box"; break;
            case PrimitiveType_Tetrahedron: str = (char*)"Tet"; break;
            case PrimitiveType_Quad       : str = (char*)"Quad"; break;
            case PrimitiveType_Sphere     : str = (char*)"Sphere"; break;
            default: break;
        }
        setString(&lines[HUD_LINE_MESH].value.string, (char*)"");
        setString(&lines[HUD_LINE_PRIM].value.string, str);
    }
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
    Viewport *viewport = &app->viewport;
    Controls *controls = &app->controls;
    Mouse *mouse = &controls->mouse;
    Scene *scene = &app->scene;

    beginFrame(timer);
        if (mouse->is_captured)
            navigateViewport(viewport, timer->delta_time);
        else
            manipulateSelection(scene, viewport, controls);

        if (!(controls->is_pressed.alt ||
              controls->is_pressed.ctrl ||
              controls->is_pressed.shift))
            updateViewport(viewport, mouse);

        if (scene->selection->changed) {
            scene->selection->changed = false;
            updateSelectionInHUD(scene->selection, viewport->hud.lines);
        }

        if (viewport->settings.show_hud)
            printNumberIntoString((i16)timer->average_frames_per_second, &viewport->hud.lines[HUD_LINE_FPS].value);

        beginDrawing(viewport);
            renderScene(scene, viewport);
            if (viewport->settings.show_BVH) drawBVH(scene, viewport);
            if (viewport->settings.show_selection) drawSelection(scene, viewport, controls);
        endDrawing(viewport);
    endFrame(timer, mouse);
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
void onKeyChanged(u8 key, bool is_pressed) {
    Scene *scene = &app->scene;
    Viewport *viewport = &app->viewport;
    NavigationMove* move = &app->viewport.navigation.move;
    if (key == 'R') move->up       = is_pressed;
    if (key == 'F') move->down     = is_pressed;
    if (key == 'W') move->forward  = is_pressed;
    if (key == 'A') move->left     = is_pressed;
    if (key == 'S') move->backward = is_pressed;
    if (key == 'D') move->right    = is_pressed;

    if (!is_pressed) {
        Primitive *prim = scene->selection->primitive;
        IsPressed pressed = app->controls.is_pressed;
        ViewportSettings *settings = &viewport->settings;
        u8 tab = app->controls.key_map.tab;
        if (key == tab) settings->show_hud = !settings->show_hud;
        else if (key == '1') settings->render_mode = RenderMode_Beauty;
        else if (key == '2') settings->render_mode = RenderMode_Depth;
        else if (key == '3') settings->render_mode = RenderMode_Normals;
        else if (key == '4') settings->render_mode = RenderMode_UVs;
        else if (key == '5' && scene->selection->primitive) {
            Material *M = scene->materials + scene->selection->primitive->material_id;
            bool use_albedo_maps = M->use & ALBEDO_MAP;
            use_albedo_maps = !use_albedo_maps;
            if (use_albedo_maps)
                M->use |= (u8)ALBEDO_MAP;
            else
                M->use &= ~((u8)ALBEDO_MAP);
            setString(&viewport->hud.lines[HUD_LINE_ALBEDO].value.string, use_albedo_maps ? (char*)"On" : (char*)"Off");
            viewport->hud.lines[HUD_LINE_ALBEDO].value_color = use_albedo_maps ? DarkGreen : DarkRed;
            uploadMaterials(scene);
        }
        else if (key == '6' && scene->selection->primitive) {
            Material *M = scene->materials + scene->selection->primitive->material_id;
            bool use_normal_maps = M->use & NORMAL_MAP;
            use_normal_maps = !use_normal_maps;
            if (use_normal_maps)
                M->use |= (u8)NORMAL_MAP;
            else
                M->use &= ~((u8)NORMAL_MAP);
            setString(&viewport->hud.lines[HUD_LINE_NORMAL].value.string, use_normal_maps ? (char*)"On" : (char*)"Off");
            viewport->hud.lines[HUD_LINE_NORMAL].value_color = use_normal_maps ? BrightGreen : DarkRed;
            uploadMaterials(scene);
        }
        else if (key == 'G') settings->use_GPU  = USE_GPU_BY_DEFAULT ? !settings->use_GPU : false;
        else if (key == 'C') viewport->settings.show_BVH = !viewport->settings.show_BVH;
        else if (key == app->controls.key_map.ctrl ||
                 key == app->controls.key_map.shift) {
            app->controls.mouse.wheel_scroll_amount = 0;
            app->controls.mouse.wheel_scrolled = false;
        }
        else if ((key == 'Z' || key == 'X') && (pressed.ctrl || pressed.shift || pressed.space)) {
            char inc = key == 'X' ? 1 : -1;
            if (pressed.ctrl && pressed.shift)  {
                viewport->trace.depth += inc;
                if (viewport->trace.depth == 0) viewport->trace.depth = 5;
                if (viewport->trace.depth == 6) viewport->trace.depth = 1;
                printNumberIntoString((i32)(viewport->trace.depth), &viewport->hud.lines[HUD_LINE_BOUNCES].value);
            } else if (prim) {
                if (pressed.shift) {
                    if (prim->type == PrimitiveType_Mesh) {
                        prim->id = (prim->id + inc + MESH_COUNT) % MESH_COUNT;
                        if (prim->id == MESH_DOG) prim->material_id = MATERIAL_DOG;
                        else if (prim->material_id == MATERIAL_DOG) prim->material_id = MATERIAL_PBR;
                        updateSceneSSB(scene, viewport);
                    } else {
                        switch(prim->type) {
                            case PrimitiveType_Box        : prim->type = inc == 1 ? PrimitiveType_Tetrahedron : PrimitiveType_Sphere;      break;
                            case PrimitiveType_Tetrahedron: prim->type = inc == 1 ? PrimitiveType_Quad        : PrimitiveType_Box;         break;
                            case PrimitiveType_Quad       : prim->type = inc == 1 ? PrimitiveType_Sphere      : PrimitiveType_Tetrahedron; break;
                            case PrimitiveType_Sphere     : prim->type = inc == 1 ? PrimitiveType_Box         : PrimitiveType_Quad;        break;
                            default: break;
                        }
                        scene->selection->object_type = prim->type;
                        if (prim->material_id == MATERIAL_FLOOR && prim->type != PrimitiveType_Quad) prim->material_id = MATERIAL_PBR;
                        uploadPrimitives(scene);
                    }
                } else if (pressed.ctrl && !pressed.space)  {
                    nextMat(prim, inc);
                    for (u8 i = 0; i < 3; i++) {
                        if (prim->material_id == MATERIAL_FLOOR && prim->type != PrimitiveType_Quad) nextMat(prim, inc);
                        if (prim->material_id == MATERIAL_DOG && !(prim->type == PrimitiveType_Mesh && prim->id == MESH_DOG)) nextMat(prim, inc);
                    }
                    uploadPrimitives(scene);
                } else if (pressed.space) {
                    Material *M = scene->materials + prim->material_id;
                    if (pressed.ctrl)  {
                        i32 rep = (i32)log2f(M->uv_repeat.u);
                        rep += (i32)inc;
                        if (rep <= 0) rep = 1;
                        if (rep >= 4) rep = 4;
                        M->uv_repeat.u = M->uv_repeat.v = powf(2.0f, (f32)rep);
                        uploadMaterials(scene);
                    } else {
                        if (M->brdf == BRDF_CookTorrance) {
                            M->roughness = clampValueToBetween(M->roughness + (f32)inc * 0.05f, 0.05f, 1.0f);
                            printFloatIntoString(M->roughness, &viewport->hud.lines[HUD_LINE_ROUGHNESS].value, 2);
                            uploadMaterials(scene);
                        }
                    }
                }
                updateSelectionInHUD(scene->selection, viewport->hud.lines);
            }
        }
        if (key >= '1' && key <= '4') {
            char *str;
            switch (settings->render_mode) {
                case RenderMode_Beauty : str = (char*)"Beauty";  break;
                case RenderMode_Normals: str = (char*)"Normals"; break;
                case RenderMode_Depth  : str = (char*)"Depth";   break;
                case RenderMode_UVs    : str = (char*)"UVs";     break;
                default: break;
            }
            setString(&viewport->hud.lines[HUD_LINE_MODE].value.string, str);
        }
    }
}
void setupViewport(Viewport *viewport) {
    HUD *hud = &viewport->hud;
    hud->line_height = 1.2f;
    hud->position.x = hud->position.y = 10;
    HUDLine *line = hud->lines;
    for (u8 i = 0; i < (u8)hud->line_count; i++, line++) {
        switch (i) {
            case HUD_LINE_XPU:
                setString(&line->value.string,    (char*)("GPU"));
                setString(&line->alternate_value, (char*)("CPU"));
                line->value_color = Green;
                line->alternate_value_color = Red;
                line->invert_alternate_use = true;
                line->use_alternate = &viewport->settings.use_GPU;
                break;
            case HUD_LINE_BVH:
                setString(&line->value.string,    (char*)("On"));
                setString(&line->alternate_value, (char*)("Off"));
                line->value_color = BrightGrey;
                line->alternate_value_color = DarkGrey;
                line->use_alternate = &viewport->settings.show_BVH;
                line->invert_alternate_use = true;
                break;
            case HUD_LINE_FPS:     setString(&line->value.string, (char*)("0")); break;
            case HUD_LINE_BOUNCES: setString(&line->value.string, (char*)("1")); break;
            case HUD_LINE_MODE:    setString(&line->value.string, (char*)("Beauty")); break;
            default:               setString(&line->value.string, (char*)(""));
        }

        switch (i) {
            case HUD_LINE_FPS:       setString(&line->title, (char*)"Fps : "); break;
            case HUD_LINE_XPU:       setString(&line->title, (char*)"Uses: "); break;
            case HUD_LINE_MODE:      setString(&line->title, (char*)"Mode: "); break;
            case HUD_LINE_BVH:       setString(&line->title, (char*)"BVH : "); break;
            case HUD_LINE_PRIM:      setString(&line->title, (char*)"Prim: "); break;
            case HUD_LINE_MESH:      setString(&line->title, (char*)"Mesh: "); break;
            case HUD_LINE_MATERIAL:  setString(&line->title, (char*)"Mat : "); break;
            case HUD_LINE_ALBEDO:    setString(&line->title, (char*)"Albd: "); break;
            case HUD_LINE_NORMAL:    setString(&line->title, (char*)"Norm: "); break;
            case HUD_LINE_UV_REPEAT: setString(&line->title, (char*)"Rep.: "); break;
            case HUD_LINE_ROUGHNESS: setString(&line->title, (char*)"Rogh: "); break;
            case HUD_LINE_BOUNCES:   setString(&line->title, (char*)"Bnc : "); break;
            default: break;
        }
    }
    viewport->frame_buffer->QCAA = true;
    viewport->settings.antialias = false;
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
        Primitive *box    = scene->primitives + PRIM_BOX;
        Primitive *tet    = scene->primitives + PRIM_TET;
        Primitive *sphere = scene->primitives + PRIM_SPHERE;
        Primitive *mesh   = scene->primitives + PRIM_MESH;

        mesh->id = MESH_DOG;
        mesh->type   = PrimitiveType_Mesh;
        box->type    = PrimitiveType_Box;
        floor->type  = PrimitiveType_Quad;
        sphere->type = PrimitiveType_Sphere;
        tet->type    = PrimitiveType_Tetrahedron;

        floor->material_id  = MATERIAL_FLOOR;
        box->material_id    = MATERIAL_PBR;
        mesh->material_id   = MATERIAL_DOG;
        tet->material_id    = MATERIAL_GLASS;
        sphere->material_id = MATERIAL_MIRROR;

        mesh->scale      = getVec3Of(0.6f);
        mesh->position   = Vec3(0, 4, -1);
        floor->scale     = Vec3(40, 1, 40);
        sphere->position = Vec3(8, 5, 0);
        sphere->scale    = Vec3(3, 3, 3);
        box->position    = Vec3(-9, 3, 3);
        tet->position    = Vec3(-3, 4, 12);

        { // Primitive rotation:
            vec3 X = Vec3(1, 0, 0);
            vec3 Y = Vec3(0, 1, 0);
            vec3 Z = Vec3(0, 0, 1);
            quat rot_x = getRotationAroundAxis(X, 0.02f);
            quat rot_y = getRotationAroundAxis(Y, 0.04f);
            quat rot_z = getRotationAroundAxis(Z, 0.06f);
            box->rotation = normQuat(mulQuat(rot_x, rot_y));
            tet->rotation = normQuat(mulQuat(box->rotation, rot_z));
        }
    }
    { // Setup Materials:
        Material *pbr    = scene->materials + MATERIAL_PBR;
        Material *mirror = scene->materials + MATERIAL_MIRROR;
        Material *glass  = scene->materials + MATERIAL_GLASS;
        Material *floor  = scene->materials + MATERIAL_FLOOR;
        Material *dog    = scene->materials + MATERIAL_DOG;

        mirror->brdf = BRDF_Blinn;
        mirror->is = REFLECTIVE;
        mirror->reflectivity = getVec3Of(0.3f);
        mirror->metallic = 1.0f;
        mirror->roughness = 0.0f;
        mirror->use = NORMAL_MAP;
        mirror->texture_count = 2;
        mirror->texture_ids[0] = TEXTURE_FLOOR_ALBEDO;
        mirror->texture_ids[1] = TEXTURE_FLOOR_NORMAL;

        glass->brdf = BRDF_Blinn;
        glass->is = REFRACTIVE;
        glass->reflectivity = getVec3Of(0.3f);
        glass->metallic = 1.0f;
        glass->roughness = 0.0f;
        glass->n1_over_n2 = IOR_AIR / IOR_GLASS;
        glass->n2_over_n1 = IOR_GLASS / IOR_AIR;
        glass->reflectivity = Vec3(0.7f, 0.9f, 0.7f);

        pbr->albedo  = getVec3Of(0.7f);
        pbr->roughness = 0.5f;
        pbr->brdf = BRDF_CookTorrance;

        floor->brdf = BRDF_CookTorrance;
        floor->texture_count = 2;
        floor->texture_ids[0] = TEXTURE_FLOOR_ALBEDO;
        floor->texture_ids[1] = TEXTURE_FLOOR_NORMAL;
        floor->uv_repeat.u = floor->uv_repeat.v = 4;

        dog->brdf = BRDF_CookTorrance;
        dog->texture_count = 2;
        dog->texture_ids[0] = TEXTURE_DOG_ALBEDO;
        dog->texture_ids[1] = TEXTURE_DOG_NORMAL;

        vec3 plastic = getVec3Of(0.04f);
        Material *material = scene->materials;
        for (u8 i = 0; i < MATERIAL_COUNT; i++, material++) {
            if (!(material->is & REFLECTIVE || material->is & REFRACTIVE))
                material->reflectivity = lerpVec3(plastic, material->albedo, material->metallic);
        }
    }
}
void initApp(Defaults *defaults) {
    static String mesh_files[MESH_COUNT];
    static String texture_files[TEXTURE_COUNT];
    static char string_buffers[MESH_COUNT + TEXTURE_COUNT][100];
    char* this_file         = (char*)__FILE__;
    char* dog_mesh          = (char*)"dog.mesh";
    char* dragon_mesh       = (char*)"dragon.mesh";
    char* monkey_mesh       = (char*)"monkey.mesh";
    char* dog_diffuse_map   = (char*)"dog_albedo.texture";
    char* dog_normal_map    = (char*)"dog_normal.texture";
    char* floor_diffuse_map = (char*)"floor_albedo.texture";
    char* floor_normal_map  = (char*)"floor_normal.texture";
    String *dragon_mesh_file_path         = mesh_files + MESH_DRAGON;
    String *dog_mesh_file_path            = mesh_files + MESH_DOG;
    String *monkey_mesh_file_path         = mesh_files + MESH_MONKEY;
    String *floor_diffuse_map_file_path   = texture_files + TEXTURE_FLOOR_ALBEDO;
    String *floor_normal_map_file_path    = texture_files + TEXTURE_FLOOR_NORMAL;
    String *dog_diffuse_map_file_path     = texture_files + TEXTURE_DOG_ALBEDO;
    String *dog_normal_map_file_path      = texture_files + TEXTURE_DOG_NORMAL;
    dragon_mesh_file_path->char_ptr       = string_buffers[MESH_DRAGON];
    dog_mesh_file_path->char_ptr          = string_buffers[MESH_DOG];
    monkey_mesh_file_path->char_ptr       = string_buffers[MESH_MONKEY];
    floor_diffuse_map_file_path->char_ptr = string_buffers[MESH_COUNT + TEXTURE_FLOOR_ALBEDO];
    floor_normal_map_file_path->char_ptr  = string_buffers[MESH_COUNT + TEXTURE_FLOOR_NORMAL];
    dog_diffuse_map_file_path->char_ptr   = string_buffers[MESH_COUNT + TEXTURE_DOG_ALBEDO];
    dog_normal_map_file_path->char_ptr    = string_buffers[MESH_COUNT + TEXTURE_DOG_NORMAL];
    u32 dir_len = getDirectoryLength(this_file);
    mergeString(dog_mesh_file_path,          this_file, dog_mesh, dir_len);
    mergeString(monkey_mesh_file_path,       this_file, monkey_mesh, dir_len);
    mergeString(dragon_mesh_file_path,       this_file, dragon_mesh, dir_len);
    mergeString(floor_diffuse_map_file_path, this_file, floor_diffuse_map, dir_len);
    mergeString(floor_normal_map_file_path,  this_file, floor_normal_map, dir_len);
    mergeString(dog_diffuse_map_file_path,   this_file, dog_diffuse_map, dir_len);
    mergeString(dog_normal_map_file_path,    this_file, dog_normal_map, dir_len);
    defaults->settings.scene.texture_files = texture_files;
    defaults->settings.scene.mesh_files = mesh_files;
    defaults->settings.scene.meshes     = MESH_COUNT;
    defaults->settings.scene.textures   = TEXTURE_COUNT;
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