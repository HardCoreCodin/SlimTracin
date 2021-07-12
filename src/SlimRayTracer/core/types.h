#pragma once

#include "./base.h"

typedef struct KeyMap      { u8 ctrl, alt, shift, space, tab; } KeyMap;
typedef struct IsPressed { bool ctrl, alt, shift, space, tab; } IsPressed;
typedef struct Controls {
    IsPressed is_pressed;
    KeyMap key_map;
    Mouse mouse;
} Controls;

typedef u64 (*GetTicks)();

typedef struct PerTick {
    f64 seconds, milliseconds, microseconds, nanoseconds;
} PerTick;

typedef struct Ticks {
    PerTick per_tick;
    u64 per_second;
} Ticks;

typedef struct Timer {
    GetTicks getTicks;
    Ticks *ticks;
    f32 delta_time;
    u64 ticks_before,
        ticks_after,
        ticks_diff,
        accumulated_ticks,
        accumulated_frame_count,
        ticks_of_last_report,
        seconds,
        milliseconds,
        microseconds,
        nanoseconds;
    f64 average_frames_per_tick,
        average_ticks_per_frame;
    u16 average_frames_per_second,
        average_milliseconds_per_frame,
        average_microseconds_per_frame,
        average_nanoseconds_per_frame;
} Timer;

typedef struct Timers {
    Timer update, render, aux;
} Timers;

typedef struct Time {
    Timers timers;
    Ticks ticks;
    GetTicks getTicks;
} Time;

typedef enum BoxSide {
    NoSide = 0,
    Top    = 1,
    Bottom = 2,
    Left   = 4,
    Right  = 8,
    Front  = 16,
    Back   = 32
} BoxSide;

typedef struct BoxCorners {
    vec3 front_top_left,
         front_top_right,
         front_bottom_left,
         front_bottom_right,
         back_top_left,
         back_top_right,
         back_bottom_left,
         back_bottom_right;
} BoxCorners;

typedef union BoxVertices {
    BoxCorners corners;
    vec3 buffer[BOX__VERTEX_COUNT];
} BoxVertices;

typedef struct BoxEdgeSides {
    Edge front_top,
         front_bottom,
         front_left,
         front_right,
         back_top,
         back_bottom,
         back_left,
         back_right,
         left_bottom,
         left_top,
         right_bottom,
         right_top;
} BoxEdgeSides;

typedef union BoxEdges {
    BoxEdgeSides sides;
    Edge buffer[BOX__EDGE_COUNT];
} BoxEdges;

typedef struct Box {
    BoxVertices vertices;
    BoxEdges edges;
} Box;

enum PrimitiveType {
    PrimitiveType_None = 0,
    PrimitiveType_Mesh,
    PrimitiveType_Box,
    PrimitiveType_Tetrahedron,
    PrimitiveType_Quad,
    PrimitiveType_Sphere,
    PrimitiveType_Light
};

enum RenderMode {
    RenderMode_Normals,
    RenderMode_Beauty,
    RenderMode_Depth,
    RenderMode_UVs
};

typedef struct Primitive {
    Rect screen_bounds;
    quat rotation;
    vec3 position, scale;
    u32 id;
    enum PrimitiveType type;
    enum ColorID color;
    u8 flags, material_id;
} Primitive;

typedef struct xform3 {
    mat3 matrix,
         yaw_matrix,
         pitch_matrix,
         roll_matrix,
         rotation_matrix,
         rotation_matrix_inverted;
    quat rotation,
         rotation_inverted;
    vec3 position, scale,
         *up_direction,
         *right_direction,
         *forward_direction;
} xform3;

typedef struct Camera {
    f32 focal_length;
    xform3 transform;
    vec3 current_velocity;
    f32 zoom, dolly, target_distance;
} Camera;

typedef struct SphereHit {
    f32 b, c, t_near, t_far, furthest;
} SphereHit;

typedef struct Ray {
    vec3 origin, scaled_origin, direction, direction_reciprocal;
    u8_3 octant;
} Ray;

typedef struct RayHit {
    vec3 position, normal;
    vec2 uv;
    f32 distance, distance_squared;
    u32 material_id, object_id, object_type;
    bool from_behind;
} RayHit;

typedef struct Trace {
    SphereHit sphere_hit;
    RayHit closest_hit, closest_mesh_hit, current_hit, *quad_light_hits;
    Ray local_space_ray;
    u32 depth, *scene_stack, *mesh_stack;
} Trace;

// BVH:
// ====
typedef struct BVHNode {
    AABB aabb;
    u32 first_child_id, primitive_count;
} BVHNode;

typedef struct BVH {
    u32 node_count, depth, *leaf_ids;
    BVHNode *nodes;
} BVH;

typedef struct NavigationSpeedSettings {
    f32 turn, zoom, dolly, pan, orbit, orient;
} NavigationSpeedSettings;

typedef struct NavigationSettings {
    NavigationSpeedSettings speeds;
    f32 max_velocity, acceleration;
} NavigationSettings;

typedef struct NavigationTurn {
    bool right, left;
} NavigationTurn;

typedef struct NavigationMove {
    bool right, left, up, down, forward, backward;
} NavigationMove;

typedef struct Navigation {
    NavigationSettings settings;
    NavigationMove move;
    NavigationTurn turn;
    bool zoomed, moved, turned;
} Navigation;

typedef struct ProjectionPlane {
    vec3 start, right, down;
} ProjectionPlane;

typedef struct ViewportSettings {
    f32 near_clipping_plane_distance,
        far_clipping_plane_distance;
    u32 hud_line_count;
    HUDLine *hud_lines;
    enum RenderMode render_mode;
    enum ColorID hud_default_color;
    bool show_hud, show_BVH, show_SSB, use_GPU;
} ViewportSettings;

typedef struct Viewport {
    ViewportSettings settings;
    Navigation navigation;
    ProjectionPlane projection_plane;
    HUD hud;
    Camera *camera;
    PixelGrid *frame_buffer;
    Trace trace;
    Box default_box;
} Viewport;

// Materials:
// =========
enum BRDFType {
    phong,
    ggx
};

typedef struct Material {
    vec3 ambient, diffuse, specular, emission;
    f32 shininess, roughness, n1_over_n2, n2_over_n1;
    enum BRDFType brdf;
    u8 flags;
} Material;

typedef struct MaterialHas {
    bool diffuse,
         specular,
         emission,
         reflection,
         refraction;
} MaterialHas;

typedef struct MaterialUses {
    bool blinn, phong;
} MaterialUses;

INLINE void decodeMaterialSpec(u8 flags, MaterialHas *has, MaterialUses *uses) {
    uses->phong = flags & (u8)PHONG;
    uses->blinn = flags & (u8)BLINN;
    has->emission = flags & (u8)EMISSION;
    has->diffuse = flags & (u8)LAMBERT;
    has->specular = uses->phong || uses->blinn;
    has->reflection = flags & (u8)REFLECTION;
    has->refraction = flags & (u8)REFRACTION;
}

// Lights:
// ======
typedef struct AmbientLight{
    vec3 color;
} AmbientLight;

typedef struct Light {
    vec3 attenuation, position_or_direction, color;
    f32 intensity;
    bool is_directional;
} Light;

typedef struct AreaLight {
    vec3 position, normal, color, U, V, v1, v2, v3, v4;
    f32 A, u_length, v_length;
} AreaLight;

// Mesh:
// =====
typedef struct Triangle {
    mat3 world_to_tangent;
    vec3 position, normal;
    vec3 vertex_normals[3];
} Triangle;
typedef struct EdgeVertexIndices { u32 from, to; } EdgeVertexIndices;
typedef struct TriangleVertexIndices { u32 ids[3]; } TriangleVertexIndices;
typedef struct Mesh {
    AABB aabb;
    u32 triangle_count, vertex_count, edge_count, normals_count, uvs_count;
    vec3 *vertex_positions, *vertex_normals;
    vec2 *vertex_uvs;
    TriangleVertexIndices *vertex_position_indices;
    TriangleVertexIndices *vertex_normal_indices;
    TriangleVertexIndices *vertex_uvs_indices;
    EdgeVertexIndices *edge_vertex_indices;
    Triangle *triangles;
    BVH bvh;
} Mesh;

typedef struct Selection {
    quat object_rotation;
    vec3 transformation_plane_origin,
         transformation_plane_normal,
         transformation_plane_center,
         object_scale,
         world_offset,
         *world_position;
    Box box;
    Primitive *primitive;
    enum BoxSide box_side;
    f32 object_distance;
    u32 object_type, object_id;
    bool changed, transformed;
} Selection;

typedef struct SceneSettings {
    u32 cameras;
    u32 primitives;
    u32 meshes;
    u32 materials;
    u32 lights;
    u32 area_lights;
    String file, *mesh_files;
} SceneSettings;

typedef struct Scene {
    SceneSettings settings;
    Selection *selection;
    BVH bvh;

    Camera *cameras;
    AmbientLight ambient_light;
    Light *lights;
    AreaLight *area_lights;

    Material *materials;
    Primitive *primitives;
    Mesh *meshes;
    u32 *mesh_bvh_node_counts,
        *mesh_triangle_counts;
} Scene;

typedef struct AppCallbacks {
    void (*sceneReady)(Scene *scene);
    void (*viewportReady)(Viewport *viewport);
    void (*windowRedraw)();
    void (*windowResize)(u16 width, u16 height);
    void (*keyChanged)(  u8 key, bool pressed);
    void (*mouseButtonUp)(  MouseButton *mouse_button);
    void (*mouseButtonDown)(MouseButton *mouse_button);
    void (*mouseButtonDoubleClicked)(MouseButton *mouse_button);
    void (*mouseWheelScrolled)(f32 amount);
    void (*mousePositionSet)(i32 x, i32 y);
    void (*mouseMovementSet)(i32 x, i32 y);
    void (*mouseRawMovementSet)(i32 x, i32 y);
} AppCallbacks;

typedef void* (*CallbackForFileOpen)(const char* file_path);
typedef bool  (*CallbackForFileRW)(void *out, unsigned long, void *handle);
typedef void  (*CallbackForFileClose)(void *handle);

typedef struct Platform {
    GetTicks             getTicks;
    CallbackWithInt      getMemory;
    CallbackWithCharPtr  setWindowTitle;
    CallbackWithBool     setWindowCapture;
    CallbackWithBool     setCursorVisibility;
    CallbackForFileClose closeFile;
    CallbackForFileOpen  openFileForReading;
    CallbackForFileOpen  openFileForWriting;
    CallbackForFileRW    readFromFile;
    CallbackForFileRW    writeToFile;
    u64 ticks_per_second;
} Platform;

typedef struct Settings {
    SceneSettings scene;
    ViewportSettings viewport;
    NavigationSettings navigation;
} Settings;

typedef struct Defaults {
    char* title;
    u16 width, height;
    u64 additional_memory_size;
    Settings settings;
} Defaults;