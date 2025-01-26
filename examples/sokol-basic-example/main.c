#define _CRT_SECURE_NO_WARNINGS
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#define SOKOL_APP_IMPL
#define SOKOL_IMPL
#define SOKOL_WIN32_FORCE_MAIN
#define SOKOL_GLCORE
#define FONTSTASH_IMPLEMENTATION
#define SOKOL_FONTSTASH_IMPL
#define SOKOL_GL_IMPL
#include "../../renderers/sokol/libs/HandmadeMath.h"
#include "../../renderers/sokol/libs/sokol_gfx.h"
#include "../../renderers/sokol/libs/sokol_app.h"
#include "../../renderers/sokol/libs/sokol_log.h"
#include "../../renderers/sokol/libs/sokol_glue.h"
#include "../../renderers/sokol/libs/sokol_gl.h"
#include <stdio.h>  // needed by fontstash's IO functions even though they are not used
#include "../../renderers/sokol/libs/stb_truetype.h"
#include "../../renderers/sokol/libs/fontstash.h"
#include "../../renderers/sokol/libs/sokol_fontstash.h"
#include "../../renderers/sokol/libs/sokol_fetch.h"

#define CLAY_IMPLEMENTATION
#include "../../clay.h"

typedef struct {
    float rx, ry;
    sg_pipeline pip;
    sg_bindings bind;

    FONScontext* fons;
    float dpi_scale;
    int font_normal;
    uint8_t font_normal_data[256 * 1024];

    int width;
    int height;
    Clay_Vector2 mouse;
    Clay_Vector2 scroll;
    bool mouse_down;
} state_t;
static state_t state;

#define SOKOL_CLAY_RENDERER
#include "../../renderers/sokol/sokol_clay_renderer.c"

#include "../../renderers/sokol/resources/cube-sapp.glsl.h"

void em_init_cube(){
    
    /* cube vertex buffer */
    float vertices[] = {
        -1.0, -1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
         1.0, -1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
         1.0,  1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
        -1.0,  1.0, -1.0,   1.0, 0.0, 0.0, 1.0,

        -1.0, -1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
         1.0, -1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
         1.0,  1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
        -1.0,  1.0,  1.0,   0.0, 1.0, 0.0, 1.0,

        -1.0, -1.0, -1.0,   0.0, 0.0, 1.0, 1.0,
        -1.0,  1.0, -1.0,   0.0, 0.0, 1.0, 1.0,
        -1.0,  1.0,  1.0,   0.0, 0.0, 1.0, 1.0,
        -1.0, -1.0,  1.0,   0.0, 0.0, 1.0, 1.0,

        1.0, -1.0, -1.0,    1.0, 0.5, 0.0, 1.0,
        1.0,  1.0, -1.0,    1.0, 0.5, 0.0, 1.0,
        1.0,  1.0,  1.0,    1.0, 0.5, 0.0, 1.0,
        1.0, -1.0,  1.0,    1.0, 0.5, 0.0, 1.0,

        -1.0, -1.0, -1.0,   0.0, 0.5, 1.0, 1.0,
        -1.0, -1.0,  1.0,   0.0, 0.5, 1.0, 1.0,
         1.0, -1.0,  1.0,   0.0, 0.5, 1.0, 1.0,
         1.0, -1.0, -1.0,   0.0, 0.5, 1.0, 1.0,

        -1.0,  1.0, -1.0,   1.0, 0.0, 0.5, 1.0,
        -1.0,  1.0,  1.0,   1.0, 0.0, 0.5, 1.0,
         1.0,  1.0,  1.0,   1.0, 0.0, 0.5, 1.0,
         1.0,  1.0, -1.0,   1.0, 0.0, 0.5, 1.0
    };
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices),
        .label = "cube-vertices"
    });

    /* create an index buffer for the cube */
    uint16_t indices[] = {
        0, 1, 2,  0, 2, 3,
        6, 5, 4,  7, 6, 4,
        8, 9, 10,  8, 10, 11,
        14, 13, 12,  15, 14, 12,
        16, 17, 18,  16, 18, 19,
        22, 21, 20,  23, 22, 20
    };
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .data = SG_RANGE(indices),
        .label = "cube-indices"
    });

    /* create shader */
    sg_shader shd = sg_make_shader(cube_shader_desc(sg_query_backend()));

    /* create pipeline object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            /* test to provide buffer stride, but no attr offsets */
            .buffers[0].stride = 28,
            .attrs = {
                [ATTR_cube_position].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_cube_color0].format   = SG_VERTEXFORMAT_FLOAT4
            }
        },
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .cull_mode = SG_CULLMODE_BACK,
        .depth = {
            .write_enabled = true,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
        },
        .label = "cube-pipeline"
    });

    /* setup resource bindings */
    state.bind = (sg_bindings) {
        .vertex_buffers[0] = vbuf,
        .index_buffer = ibuf
    };
}

vs_params_t em_render_cube(){
    vs_params_t vs_params;
    const float w = sapp_widthf();
    const float h = sapp_heightf();
    const float t = (float)(sapp_frame_duration() * 60.0);
    hmm_mat4 proj = HMM_Perspective(60.0f, w/h, 0.01f, 10.0f);
    hmm_mat4 view = HMM_LookAt(HMM_Vec3(0.0f, 1.5f, 6.0f), HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f));
    hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);
    state.rx += 1.0f * t; state.ry += 2.0f * t;
    hmm_mat4 rxm = HMM_Rotate(state.rx, HMM_Vec3(1.0f, 0.0f, 0.0f));
    hmm_mat4 rym = HMM_Rotate(state.ry, HMM_Vec3(0.0f, 1.0f, 0.0f));
    hmm_mat4 model = HMM_MultiplyMat4(rxm, rym);
    vs_params.mvp = HMM_MultiplyMat4(view_proj, model);

    return vs_params;
}

const int FONT_ID_BODY_16 = 0;
Clay_Color COLOR_WHITE = { 255, 255, 255, 255};


typedef struct {
    Clay_String title;
    Clay_String contents;
} Document;

typedef struct {
    Document *documents;
    uint32_t length;
} DocumentArray;

DocumentArray documents = {
    .documents = NULL, // TODO figure out if it's possible to const init this list
    .length = 5
};

uint32_t selectedDocumentIndex = 0;


void UI_Layout(){
    Clay_SetLayoutDimensions((Clay_Dimensions) {
            .width = state.width,
            .height = state.height,
    });
    Clay_SetPointerState(
        state.mouse,
        state.mouse_down
    );
    Clay_UpdateScrollContainers(
        true,
        state.scroll,
        (float)sapp_frame_duration()
    );

    Clay_Sizing layoutExpand = {
        .width = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0)
    };

    Clay_RectangleElementConfig contentBackgroundConfig = {
        .color = { 90, 90, 90, 255 },
        .cornerRadius = CLAY_CORNER_RADIUS(8)
    };

    Clay_BeginLayout();
    CLAY(
            CLAY_ID("OuterContainer"),
            CLAY_RECTANGLE({ .color = { 43, 41, 51, 255 } }),
            CLAY_LAYOUT({
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = layoutExpand,
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16
            })
        ) {
        //    CLAY(
        //         CLAY_ID("MainContent"),
        //         CLAY_RECTANGLE(contentBackgroundConfig),
        //         CLAY_SCROLL({ .vertical = true }),
        //         CLAY_LAYOUT({
        //             .layoutDirection = CLAY_TOP_TO_BOTTOM,
        //             .childGap = 16,
        //             .padding = CLAY_PADDING_ALL(16),
        //             .sizing = layoutExpand
        //         })
        //     ) {
        //         Document selectedDocument = documents.documents[selectedDocumentIndex];
        //         CLAY_TEXT(selectedDocument.title, CLAY_TEXT_CONFIG({
        //             .fontId = FONT_ID_BODY_16,
        //             .fontSize = 24,
        //             .textColor = COLOR_WHITE
        //         }));
        //         CLAY_TEXT(selectedDocument.contents, CLAY_TEXT_CONFIG({
        //             .fontId = FONT_ID_BODY_16,
        //             .fontSize = 24,
        //             .textColor = COLOR_WHITE
        //         }));
        //     }
        }
    Clay_RenderCommandArray render_commands = Clay_EndLayout();
    ui_render_clay(render_commands);
}

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

static void initialize_application(void) {

    state.width = sapp_width();
    state.height = sapp_height();
    state.dpi_scale = sapp_dpi_scale();
    
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    em_init_cube();

    sgl_setup(&(sgl_desc_t){0});
    load_font(state.dpi_scale);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .memory = malloc(totalMemorySize),
        .capacity = totalMemorySize
    };
    Clay_Initialize(clayMemory, (Clay_Dimensions) { (float) state.width , (float) state.width  }, (Clay_ErrorHandler) { HandleClayErrors });
    Clay_SetMeasureTextFunction(ui_measure_text, 0);
}

static void frame(void) {
    sfetch_dowork();

    fonsClearState(state.fons);
    sgl_defaults();
    sgl_matrix_mode_projection();
    sgl_ortho(0.0f, state.width, state.height, 0.0f, -1.0f, +1.0f);

    if (state.font_normal != FONS_INVALID) {
        UI_Layout();
    }

    sfons_flush(state.fons);

    vs_params_t vs_params = em_render_cube();

    // render pass
    sg_begin_pass(&(sg_pass){
        .action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = { 0.3f, 0.3f, 0.32f, 1.0f }
            }
        },
        .swapchain = sglue_swapchain()
    });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    sgl_draw();
    sg_end_pass();
    sg_commit();
}

static void event(const sapp_event* event){
    switch (event->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            state.mouse_down = true;
            break;

        case SAPP_EVENTTYPE_MOUSE_UP:
            state.mouse_down = false;
            break;

        case SAPP_EVENTTYPE_MOUSE_MOVE:
            state.mouse.x = event->mouse_x;
            state.mouse.y = event->mouse_y;
            break;

        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            state.scroll.x = event->scroll_x;
            state.scroll.y = event->scroll_y;
            break;

        case SAPP_EVENTTYPE_RESIZED:
            state.dpi_scale = sapp_dpi_scale();
            state.width = sapp_width();
            state.height = sapp_height();
            break;

        default:
            break;
    }
}

static void cleanup(void) {
    sfetch_shutdown();
    sfons_destroy(state.fons);
    sgl_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc) {
        .init_cb = initialize_application,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .high_dpi = true,
        .window_title = "clay sokol demo",
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}