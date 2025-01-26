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

void clay_text(const char* text, int size){
    Clay_String formatted_text = CLAY_STRING("text");
    Clay_TextElementConfig config = {
        .fontId = 0,
        .fontSize = 12,
        .textColor = { 255, 255, 255, 255 },
    };
    CLAY_TEXT(formatted_text, &config);
}

void RenderHeaderButton(char* text) {
    CLAY(
        CLAY_LAYOUT({ .padding = { 16, 16, 8, 8 }}),
        CLAY_RECTANGLE({
            .color = { 140, 140, 140, 255 },
            .cornerRadius = 5
        })
    ) {
        clay_text(text, 16);
    }
}

void RenderDropdownMenuItem(char* text) {
    CLAY(CLAY_LAYOUT({ .padding = CLAY_PADDING_ALL(16)})) {
        clay_text(text, 16);
    }
}

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

void HandleSidebarInteraction(
    Clay_ElementId elementId,
    Clay_PointerData pointerData,
    intptr_t userData
) {
    // If this button was clicked
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (userData >= 0 && userData < documents.length) {
            // Select the corresponding document
            selectedDocumentIndex = userData;
        }
    }
}

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
            // Child elements go inside braces
            CLAY(
                CLAY_ID("HeaderBar"),
                CLAY_RECTANGLE(contentBackgroundConfig),
                CLAY_LAYOUT({
                    .sizing = {
                        .height = CLAY_SIZING_FIXED(60),
                        .width = CLAY_SIZING_GROW(0)
                    },
                    .padding = { 16, 16, 0, 0 },
                    .childGap = 16,
                    .childAlignment = {
                        .y = CLAY_ALIGN_Y_CENTER
                    }
                })
                
            ) {
                // Header buttons go here
                
                CLAY(
                    CLAY_ID("FileButton"),
                    CLAY_LAYOUT({ .padding = { 16, 16, 8, 8 }}),
                    CLAY_RECTANGLE({
                        .color = { 140, 140, 140, 255 },
                        .cornerRadius = CLAY_CORNER_RADIUS(5)
                    })
                ) {
                    clay_text("File", 16);

                    bool fileMenuVisible =
                        Clay_PointerOver(Clay_GetElementId(CLAY_STRING("FileButton")))
                        ||
                        Clay_PointerOver(Clay_GetElementId(CLAY_STRING("FileMenu")));
                    
                    if (fileMenuVisible) { // Below has been changed slightly to fix the small bug where the menu would dismiss when mousing over the top gap
                        CLAY(
                            CLAY_ID("FileMenu"),
                            CLAY_FLOATING({
                                .attachment = {
                                    .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM
                                },
                            }),
                            CLAY_LAYOUT({
                                .padding = {0, 0, 8, 8 }
                            })
                        ) {
                            CLAY(
                                CLAY_LAYOUT({
                                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                    .sizing = {
                                            .width = CLAY_SIZING_FIXED(200)
                                    },
                                }),
                                CLAY_RECTANGLE({
                                    .color = { 40, 40, 40, 255 },
                                    .cornerRadius = CLAY_CORNER_RADIUS(8)
                                })
                            ) {
                                // Render dropdown items here
                                RenderDropdownMenuItem("New");
                                RenderDropdownMenuItem("Open");
                                RenderDropdownMenuItem("Close");
                            }
                        }
                    }
                }
                
                RenderHeaderButton("Edit");
                CLAY(CLAY_LAYOUT({ .sizing = { CLAY_SIZING_GROW(0) }})) {}
                RenderHeaderButton("Upload");
                RenderHeaderButton("Media");
                RenderHeaderButton("Support");
            }
            
            CLAY(
                CLAY_ID("LowerContent"),
                CLAY_LAYOUT({ .sizing = layoutExpand, .childGap = 16 })
            ) {
                CLAY(
                    CLAY_ID("Sidebar"),
                    CLAY_RECTANGLE(contentBackgroundConfig),
                    CLAY_LAYOUT({
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .padding = CLAY_PADDING_ALL(16),
                        .childGap = 8,
                        .sizing = {
                            .width = CLAY_SIZING_FIXED(250),
                            .height = CLAY_SIZING_GROW(0)
                        }
                    })
                ) {
                    for (int i = 0; i < documents.length; i++) {
                        Document document = documents.documents[i];
                        Clay_LayoutConfig sidebarButtonLayout = {
                            .sizing = { .width = CLAY_SIZING_GROW(0) },
                            .padding = CLAY_PADDING_ALL(16)
                        };

                        if (i == selectedDocumentIndex) {
                            CLAY(
                                CLAY_LAYOUT(sidebarButtonLayout),
                                CLAY_RECTANGLE({
                                    .color = { 120, 120, 120, 255 },
                                    .cornerRadius = CLAY_CORNER_RADIUS(8),
                                })
                            ) {
                                clay_text(document.title.chars, 20);
                            }
                        } else {
                            CLAY(
                                CLAY_LAYOUT(sidebarButtonLayout),
                                Clay_OnHover(HandleSidebarInteraction, i),
                                Clay_Hovered()
                                    ? CLAY_RECTANGLE({
                                        .color = { 120, 120, 120, 120 },
                                        .cornerRadius = CLAY_CORNER_RADIUS(8)
                                    })
                                    : 0
                            ) {
                                clay_text(document.title.chars, 20);
                            }
                        }
                    }
                }

                CLAY(
                    CLAY_ID("MainContent"),
                    CLAY_RECTANGLE(contentBackgroundConfig),
                    CLAY_SCROLL({ .vertical = true }),
                    CLAY_LAYOUT({
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .childGap = 16,
                        .padding = CLAY_PADDING_ALL(16),
                        .sizing = layoutExpand
                    })
                ) {
                    Document selectedDocument = documents.documents[selectedDocumentIndex];
                    clay_text(selectedDocument.title.chars, 24);
                    clay_text(selectedDocument.contents.chars, 24);
                }
            }
        }
    Clay_RenderCommandArray render_commands = Clay_EndLayout();
    ui_render_clay(render_commands);

    ui_draw_text("whut",(TextDescription){
        .position_x = 100,
        .position_y = 100
    });
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

    documents.documents = (Document[]) {
            { .title = CLAY_STRING("Squirrels"), .contents = CLAY_STRING("The Secret Life of Squirrels: Nature's Clever Acrobats\n""Squirrels are often overlooked creatures, dismissed as mere park inhabitants or backyard nuisances. Yet, beneath their fluffy tails and twitching noses lies an intricate world of cunning, agility, and survival tactics that are nothing short of fascinating. As one of the most common mammals in North America, squirrels have adapted to a wide range of environments from bustling urban centers to tranquil forests and have developed a variety of unique behaviors that continue to intrigue scientists and nature enthusiasts alike.\n""\n""Master Tree Climbers\n""At the heart of a squirrel's skill set is its impressive ability to navigate trees with ease. Whether they're darting from branch to branch or leaping across wide gaps, squirrels possess an innate talent for acrobatics. Their powerful hind legs, which are longer than their front legs, give them remarkable jumping power. With a tail that acts as a counterbalance, squirrels can leap distances of up to ten times the length of their body, making them some of the best aerial acrobats in the animal kingdom.\n""But it's not just their agility that makes them exceptional climbers. Squirrels' sharp, curved claws allow them to grip tree bark with precision, while the soft pads on their feet provide traction on slippery surfaces. Their ability to run at high speeds and scale vertical trunks with ease is a testament to the evolutionary adaptations that have made them so successful in their arboreal habitats.\n""\n""Food Hoarders Extraordinaire\n""Squirrels are often seen frantically gathering nuts, seeds, and even fungi in preparation for winter. While this behavior may seem like instinctual hoarding, it is actually a survival strategy that has been honed over millions of years. Known as \"scatter hoarding,\" squirrels store their food in a variety of hidden locations, often burying it deep in the soil or stashing it in hollowed-out tree trunks.\n""Interestingly, squirrels have an incredible memory for the locations of their caches. Research has shown that they can remember thousands of hiding spots, often returning to them months later when food is scarce. However, they don't always recover every stash some forgotten caches eventually sprout into new trees, contributing to forest regeneration. This unintentional role as forest gardeners highlights the ecological importance of squirrels in their ecosystems.\n""\n""The Great Squirrel Debate: Urban vs. Wild\n""While squirrels are most commonly associated with rural or wooded areas, their adaptability has allowed them to thrive in urban environments as well. In cities, squirrels have become adept at finding food sources in places like parks, streets, and even garbage cans. However, their urban counterparts face unique challenges, including traffic, predators, and the lack of natural shelters. Despite these obstacles, squirrels in urban areas are often observed using human infrastructure such as buildings, bridges, and power lines as highways for their acrobatic escapades.\n""There is, however, a growing concern regarding the impact of urban life on squirrel populations. Pollution, deforestation, and the loss of natural habitats are making it more difficult for squirrels to find adequate food and shelter. As a result, conservationists are focusing on creating squirrel-friendly spaces within cities, with the goal of ensuring these resourceful creatures continue to thrive in both rural and urban landscapes.\n""\n""A Symbol of Resilience\n""In many cultures, squirrels are symbols of resourcefulness, adaptability, and preparation. Their ability to thrive in a variety of environments while navigating challenges with agility and grace serves as a reminder of the resilience inherent in nature. Whether you encounter them in a quiet forest, a city park, or your own backyard, squirrels are creatures that never fail to amaze with their endless energy and ingenuity.\n""In the end, squirrels may be small, but they are mighty in their ability to survive and thrive in a world that is constantly changing. So next time you spot one hopping across a branch or darting across your lawn, take a moment to appreciate the remarkable acrobat at work a true marvel of the natural world.\n") },
            { .title = CLAY_STRING("Lorem Ipsum"), .contents = CLAY_STRING("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.") },
            { .title = CLAY_STRING("Vacuum Instructions"), .contents = CLAY_STRING("Chapter 3: Getting Started - Unpacking and Setup\n""\n""Congratulations on your new SuperClean Pro 5000 vacuum cleaner! In this section, we will guide you through the simple steps to get your vacuum up and running. Before you begin, please ensure that you have all the components listed in the \"Package Contents\" section on page 2.\n""\n""1. Unboxing Your Vacuum\n""Carefully remove the vacuum cleaner from the box. Avoid using sharp objects that could damage the product. Once removed, place the unit on a flat, stable surface to proceed with the setup. Inside the box, you should find:\n""\n""    The main vacuum unit\n""    A telescoping extension wand\n""    A set of specialized cleaning tools (crevice tool, upholstery brush, etc.)\n""    A reusable dust bag (if applicable)\n""    A power cord with a 3-prong plug\n""    A set of quick-start instructions\n""\n""2. Assembling Your Vacuum\n""Begin by attaching the extension wand to the main body of the vacuum cleaner. Line up the connectors and twist the wand into place until you hear a click. Next, select the desired cleaning tool and firmly attach it to the wand's end, ensuring it is securely locked in.\n""\n""For models that require a dust bag, slide the bag into the compartment at the back of the vacuum, making sure it is properly aligned with the internal mechanism. If your vacuum uses a bagless system, ensure the dust container is correctly seated and locked in place before use.\n""\n""3. Powering On\n""To start the vacuum, plug the power cord into a grounded electrical outlet. Once plugged in, locate the power switch, usually positioned on the side of the handle or body of the unit, depending on your model. Press the switch to the \"On\" position, and you should hear the motor begin to hum. If the vacuum does not power on, check that the power cord is securely plugged in, and ensure there are no blockages in the power switch.\n""\n""Note: Before first use, ensure that the vacuum filter (if your model has one) is properly installed. If unsure, refer to \"Section 5: Maintenance\" for filter installation instructions.") },
            { .title = CLAY_STRING("Article 4"), .contents = CLAY_STRING("Article 4") },
            { .title = CLAY_STRING("Article 5"), .contents = CLAY_STRING("Article 5") },
    };

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