#ifndef SOKOL_CLAY_RENDERER

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
#include "../libs/HandmadeMath.h"
#include "../libs/sokol_gfx.h"
#include "../libs/sokol_app.h"
#include "../libs/sokol_log.h"
#include "../libs/sokol_glue.h"
#include "../libs/sokol_gl.h"
#include <stdio.h>  // needed by fontstash's IO functions even though they are not used
#include "../libs/stb_truetype.h"
#include "../libs/fontstash.h"
#include "../libs/sokol_fontstash.h"
#include "../libs/sokol_fetch.h"

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
} state_t;
static state_t state;

#endif


const char* get_file_path(const char* filename, char* buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", filename);
    return buf;
}

static int rounded_power_of_2(float v) {
    uint32_t vi = ((uint32_t) v) - 1;
    for (uint32_t i = 0; i < 5; i++) {
        vi |= (vi >> (1<<i));
    }
    return (int) (vi + 1);
}

static void load_font_callback(const sfetch_response_t* response) {
    if (response->fetched) {
        state.font_normal = fonsAddFontMem(state.fons, "sans", (unsigned char*)response->data.ptr, (int)response->data.size,  false);
    }
}

void load_font(const float sapp_dpi_scale){
    state.dpi_scale = sapp_dpi_scale;
    const int atlas_dim = rounded_power_of_2(512.0f * state.dpi_scale);

    FONScontext* fons_context = sfons_create(&(sfons_desc_t){
        .width = atlas_dim,
        .height = atlas_dim, 
    });
    state.fons = fons_context;
    state.font_normal = FONS_INVALID;

    sfetch_setup(&(sfetch_desc_t){
        .num_channels = 1,
        .num_lanes = 4,
        .logger.func = slog_func,
    });
    char path_buf[4096];
    sfetch_send(&(sfetch_request_t){
        .path = get_file_path("../../renderers/sokol/resources/DroidSerif-Regular.ttf", path_buf, sizeof(path_buf)),
        .callback = load_font_callback,
        .buffer = SFETCH_RANGE(state.font_normal_data),
    });
}


typedef struct {
    float position_x;
    float position_y;
    uint32_t color;
    float size;
    float spacing;
    bool justify_left;
    bool justify_right;
    bool justify_center;
    bool align_top;
    bool align_baseline;
    bool align_middle;
    bool align_bottom;
} TextDescription;

typedef struct {
    float x;
    float y;
} gui_vec2;

typedef struct {
    float start_x;
    float start_y;
    float end_x;
    float end_y;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
    float angle;
    float length;
    float width;
} LineDescription;

typedef struct {
    gui_vec2 top_left;
    gui_vec2 top_right;
    gui_vec2 bottom_left;
    gui_vec2 bottom_right;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} QuadDescription;

typedef struct {
    float start_x;
    float start_y;
    float width;
    float height;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
    float top_left_radius;
    float top_right_radius;
    float bottom_left_radius;
    float bottom_right_radius;
    float border_width;
} RectangleDescription;

typedef struct {
    float x;
    float y;
    float radius;
    float thickness;
    float degree_begin;
    float degree_end;
    uint8_t segments;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} ArcDescription;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} ScissorDescription;

static gui_vec2 gui_arc_points[51];

gui_vec2 rotate_vec2(float degrees, gui_vec2 vector){
    degrees = -degrees;
    float cs = (float)cos((double)sgl_rad(degrees));
    float sn = (float)sin((double)sgl_rad(degrees));

    return (gui_vec2) {
        .x = vector.x*cs - vector.y*sn,
        .y = vector.x*sn + vector.y*cs,
    };
}

// done
void ui_draw_text(const char* text, TextDescription text_description){
    fonsSetFont(state.fons, state.font_normal);
    if (text_description.size == 0){text_description.size = 12.0f;}
    fonsSetSize(state.fons, text_description.size);
    if (text_description.color == 0){text_description.color = sfons_rgba(255, 255, 255, 255);}
    fonsSetColor(state.fons, text_description.color);
    // fonsSetSize(state.fons, 12.0f);
    // fonsSetColor(state.fons,sfons_rgba(255, 255, 255, 255));
    fonsSetSpacing(state.fons, text_description.spacing);
    fonsDrawText(state.fons, text_description.position_x, text_description.position_y, text, NULL);
}
// done
void ui_draw_line(LineDescription line_description){
    if (line_description.width == 0){
        sgl_begin_lines();
        sgl_c4b(line_description.red, line_description.green, line_description.blue, line_description.alpha);
        sgl_v2f(line_description.start_x, line_description.start_y);
        sgl_v2f(line_description.end_x, line_description.end_y);
        sgl_end();
        return;
    }

    gui_vec2 points[4];

    points[0].x = 0.0f;
    points[0].y = 0.0f + (line_description.width/2.0f);

    points[1].x = 0.0f;
    points[1].y = 0.0f - (line_description.width/2.0f);

    points[2].x = 0.0f + line_description.length;
    points[2].y = 0.0f - (line_description.width/2.0f);

    points[3].x = 0.0f + line_description.length;
    points[3].y = 0.0f + (line_description.width/2.0f);

    sgl_begin_quads();
    for (int i = 0; i < 4; i++){
        points[i] = rotate_vec2(line_description.angle, points[i]);

        points[i].x += line_description.start_x;
        points[i].y += line_description.start_y;

        sgl_v2f_c4b( 
            points[i].x, 
            points[i].y, 
            line_description.red, 
            line_description.green, 
            line_description.blue, 
            line_description.alpha
        );
    };
    sgl_end();
}
// done
void ui_draw_arc(ArcDescription arc_description){
    arc_description.thickness = arc_description.thickness == 0 ? 1 : arc_description.thickness;
    arc_description.segments = arc_description.segments == 0 ? 10 : arc_description.segments;
    arc_description.segments = arc_description.segments > 50 ? 50 : arc_description.segments;
    arc_description.degree_begin = arc_description.degree_begin < 0 ? arc_description.degree_begin += 360 : arc_description.degree_begin;
    arc_description.degree_end = arc_description.degree_end < arc_description.degree_begin ? arc_description.degree_end += 360 : arc_description.degree_end;

    float arc_length = fabs(arc_description.degree_end-arc_description.degree_begin);
    float arc_segment_length = arc_length / (float)arc_description.segments;
    float arc_segment_distance = (2.0f * M_PI * arc_description.radius) * (arc_segment_length/360);

    for (int i = 0; i<arc_description.segments; i++){
        gui_arc_points[i].x = arc_description.radius;
        gui_arc_points[i].y = 0;

        gui_arc_points[i] = rotate_vec2(arc_description.degree_begin+(arc_segment_length*i), gui_arc_points[i]);

        ui_draw_line(
            (LineDescription){
                .start_x = arc_description.x + gui_arc_points[i].x,
                .start_y = arc_description.y + gui_arc_points[i].y,
                .length = arc_segment_distance,
                .width = arc_description.thickness,
                .angle = arc_description.degree_begin + 90 + (arc_segment_length*i) + (arc_segment_length/2),
                .red = arc_description.red,
                .green = arc_description.green,
                .blue = arc_description.blue,
                .alpha = arc_description.alpha
            }
        );
    }
}
// done
void ui_draw_filled_arc(ArcDescription arc_description){
    arc_description.thickness = arc_description.thickness == 0 ? 1 : arc_description.thickness;
    arc_description.segments = arc_description.segments == 0 ? 10 : arc_description.segments;
    arc_description.segments = arc_description.segments > 50 ? 50 : arc_description.segments;
    arc_description.degree_begin = arc_description.degree_begin < 0 ? arc_description.degree_begin += 360 : arc_description.degree_begin;
    arc_description.degree_end = arc_description.degree_end < arc_description.degree_begin ? arc_description.degree_end += 360 : arc_description.degree_end;

    float arc_length = fabs(arc_description.degree_end-arc_description.degree_begin);
    float arc_segment_length = arc_length / (float)arc_description.segments;
    float arc_segment_distance = (2.0f * M_PI * arc_description.radius) * (arc_segment_length/360);

    sgl_begin_triangles();

    for (int i = 0; i<arc_description.segments; i++){
        gui_arc_points[i].x = arc_description.radius;
        gui_arc_points[i].y = 0;
        gui_arc_points[i] = rotate_vec2(arc_description.degree_begin+(arc_segment_length*i), gui_arc_points[i]);

        gui_arc_points[i+1].x = arc_description.radius;
        gui_arc_points[i+1].y = 0;
        gui_arc_points[i+1] = rotate_vec2(arc_description.degree_begin+(arc_segment_length*(i+1)), gui_arc_points[i+1]);

        sgl_v2f_c4b(
            gui_arc_points[i].x + arc_description.x,
            gui_arc_points[i].y + arc_description.y,
            arc_description.red,
            arc_description.green,
            arc_description.blue,
            arc_description.alpha
        );
        sgl_v2f_c4b(
            arc_description.x,
            arc_description.y,
            arc_description.red,
            arc_description.green,
            arc_description.blue,
            arc_description.alpha
        );
        sgl_v2f_c4b(
            gui_arc_points[i+1].x + arc_description.x,
            gui_arc_points[i+1].y + arc_description.y,
            arc_description.red,
            arc_description.green,
            arc_description.blue,
            arc_description.alpha
        );
    }

    sgl_end();
}
// done
void ui_draw_quad(QuadDescription quad_description){
    sgl_begin_quads();
    sgl_v2f_c4b(  
        quad_description.top_left.x,
        quad_description.top_left.y,
        quad_description.red,
        quad_description.green,
        quad_description.blue,
        quad_description.alpha
    );
    sgl_v2f_c4b(  
        quad_description.bottom_left.x,
        quad_description.bottom_left.y,
        quad_description.red,
        quad_description.green,
        quad_description.blue,
        quad_description.alpha
    );
    sgl_v2f_c4b(  
        quad_description.bottom_right.x,
        quad_description.bottom_right.y,
        quad_description.red,
        quad_description.green,
        quad_description.blue,
        quad_description.alpha
    );
    sgl_v2f_c4b(  
        quad_description.top_right.x,
        quad_description.top_right.y,
        quad_description.red,
        quad_description.green,
        quad_description.blue,
        quad_description.alpha
    );
    sgl_end();
}
//done
void ui_draw_filled_rectangle(RectangleDescription rectangle_description){
    gui_vec2 anchor_topleft = (gui_vec2){
        .x = rectangle_description.start_x + rectangle_description.top_left_radius,
        .y = rectangle_description.start_y + rectangle_description.top_left_radius
    };
    gui_vec2 anchor_topright = (gui_vec2){
        .x = rectangle_description.start_x + rectangle_description.width - rectangle_description.top_right_radius,
        .y = rectangle_description.start_y + rectangle_description.top_right_radius
    };
    gui_vec2 anchor_bottomleft = (gui_vec2){
        .x = rectangle_description.start_x + rectangle_description.bottom_left_radius,
        .y = rectangle_description.start_y + rectangle_description.height - rectangle_description.bottom_left_radius
    };
    gui_vec2 anchor_bottomright = (gui_vec2){
        .x = rectangle_description.start_x + rectangle_description.width - rectangle_description.bottom_right_radius,
        .y = rectangle_description.start_y + rectangle_description.height - rectangle_description.bottom_right_radius
    };

    ui_draw_filled_arc((ArcDescription){
        .x = anchor_topleft.x,
        .y = anchor_topleft.y,
        .radius = rectangle_description.top_left_radius,
        .degree_begin = 90,
        .degree_end = 180,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    ui_draw_filled_arc((ArcDescription){
        .x = anchor_topright.x,
        .y = anchor_topright.y,
        .radius = rectangle_description.top_right_radius,
        .degree_begin = 0,
        .degree_end = 90,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    ui_draw_filled_arc((ArcDescription){
        .x = anchor_bottomleft.x,
        .y = anchor_bottomleft.y,
        .radius = rectangle_description.bottom_left_radius,
        .degree_begin = 180,
        .degree_end = 270,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    ui_draw_filled_arc((ArcDescription){
        .x = anchor_bottomright.x,
        .y = anchor_bottomright.y,
        .radius = rectangle_description.bottom_right_radius,
        .degree_begin = 270,
        .degree_end = 360,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    // center
    ui_draw_quad((QuadDescription){
        .top_left = anchor_topleft,
        .top_right = anchor_topright,
        .bottom_left = anchor_bottomleft,
        .bottom_right = anchor_bottomright,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    // left side
    ui_draw_quad((QuadDescription){
        .top_left = (gui_vec2){
            .x = rectangle_description.start_x,
            .y = anchor_topleft.y
        },
        .top_right = anchor_topleft,
        .bottom_left = (gui_vec2){
            .x = rectangle_description.start_x,
            .y = anchor_bottomleft.y
        },
        .bottom_right = anchor_bottomleft,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    // bottom
    ui_draw_quad((QuadDescription){
        .top_left = anchor_bottomleft,
        .top_right = anchor_bottomright,
        .bottom_left = (gui_vec2){
            .x = anchor_bottomleft.x,
            .y = rectangle_description.start_y + rectangle_description.height
        },
        .bottom_right = (gui_vec2){
            .x = anchor_bottomright.x,
            .y = rectangle_description.start_y + rectangle_description.height
        },
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    // right
    ui_draw_quad((QuadDescription){
        .top_left = anchor_topright,
        .top_right = (gui_vec2){
            .x = rectangle_description.start_x + rectangle_description.width,
            .y = anchor_topright.y
        },
        .bottom_left = anchor_bottomright,
        .bottom_right = (gui_vec2){
            .x = rectangle_description.start_x + rectangle_description.width,
            .y = anchor_bottomright.y
        },
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    // top
    ui_draw_quad((QuadDescription){
        .top_left = (gui_vec2){
            .x = anchor_topleft.x,
            .y = rectangle_description.start_y
        },
        .top_right = (gui_vec2){
            .x = anchor_topright.x,
            .y = rectangle_description.start_y
        },
        .bottom_left = anchor_topleft,
        .bottom_right = anchor_topright,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
}
// done
void ui_draw_unfilled_rectangle(RectangleDescription rectangle_description){
    gui_vec2 anchor_topleft = (gui_vec2){
        .x = rectangle_description.start_x + rectangle_description.top_left_radius,
        .y = rectangle_description.start_y + rectangle_description.top_left_radius
    };
    gui_vec2 anchor_topright = (gui_vec2){
        .x = rectangle_description.start_x + rectangle_description.width - rectangle_description.top_right_radius,
        .y = rectangle_description.start_y + rectangle_description.top_right_radius
    };
    gui_vec2 anchor_bottomleft = (gui_vec2){
        .x = rectangle_description.start_x + rectangle_description.bottom_left_radius,
        .y = rectangle_description.start_y + rectangle_description.height - rectangle_description.bottom_left_radius
    };
    gui_vec2 anchor_bottomright = (gui_vec2){
        .x = rectangle_description.start_x + rectangle_description.width - rectangle_description.bottom_right_radius,
        .y = rectangle_description.start_y + rectangle_description.height - rectangle_description.bottom_right_radius
    };

    ui_draw_arc((ArcDescription){
        .x = anchor_topleft.x,
        .y = anchor_topleft.y,
        .radius = rectangle_description.top_left_radius,
        .degree_begin = 90,
        .degree_end = 180,
        .thickness = rectangle_description.border_width,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    ui_draw_arc((ArcDescription){
        .x = anchor_topright.x,
        .y = anchor_topright.y,
        .radius = rectangle_description.top_right_radius,
        .degree_begin = 0,
        .degree_end = 90,
        .thickness = rectangle_description.border_width,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    ui_draw_arc((ArcDescription){
        .x = anchor_bottomleft.x,
        .y = anchor_bottomleft.y,
        .radius = rectangle_description.bottom_left_radius,
        .degree_begin = 180,
        .degree_end = 270,
        .thickness = rectangle_description.border_width,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    ui_draw_arc((ArcDescription){
        .x = anchor_bottomright.x,
        .y = anchor_bottomright.y,
        .radius = rectangle_description.bottom_right_radius,
        .degree_begin = 270,
        .degree_end = 360,
        .thickness = rectangle_description.border_width,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha
    });
    // right
    ui_draw_line((LineDescription){
        .start_x = rectangle_description.start_x,
        .start_y = anchor_topleft.y,
        .angle = 270,
        .length = anchor_bottomleft.y - anchor_topleft.y,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha,
        .width = rectangle_description.border_width
    });
    // bottom
    ui_draw_line((LineDescription){
        .start_x = anchor_bottomleft.x,
        .start_y = rectangle_description.start_y + rectangle_description.height,
        .angle = 0,
        .length = anchor_bottomright.x - anchor_bottomleft.x,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha,
        .width = rectangle_description.border_width
    });
    // right
    ui_draw_line((LineDescription){
        .start_x = rectangle_description.start_x + rectangle_description.width,
        .start_y = anchor_bottomright.y,
        .angle = 90,
        .length = anchor_bottomright.y - anchor_topright.y,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha,
        .width = rectangle_description.border_width
    });
    // top
    ui_draw_line((LineDescription){
        .start_x = anchor_topright.x,
        .start_y = rectangle_description.start_y,
        .angle = 180,
        .length = anchor_topright.x - anchor_topleft.x,
        .red = rectangle_description.red,
        .green = rectangle_description.green,
        .blue = rectangle_description.blue,
        .alpha = rectangle_description.alpha,
        .width = rectangle_description.border_width
    });
}
// not tested
void ui_start_scissor(ScissorDescription scissor_description){
    sgl_scissor_rect(
        scissor_description.x,
        scissor_description.y,
        scissor_description.width,
        scissor_description.height,
        true
    );
}
// not tested
void ui_end_scissor(){
    sgl_scissor_rect(
        0,
        0,
        state.width,
        state.height,
        true
    );
}
// not started
void ui_draw_image(){

}
// not finished
void ui_render_clay(Clay_RenderCommandArray renderCommands){
    for (int j = 0; j < renderCommands.length; j++)
    {
        Clay_RenderCommand *renderCommand = Clay_RenderCommandArray_Get(&renderCommands, j);
        Clay_BoundingBox boundingBox = renderCommand->boundingBox;
        switch (renderCommand->commandType)
        {
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_String text = renderCommand->text;
                char *cloned = (char *)malloc(text.length + 1);
                memcpy(cloned, text.chars, text.length);
                cloned[text.length] = '\0';

                int sz = renderCommand->config.textElementConfig->fontSize;

                ui_draw_text(cloned, (TextDescription){
                    .position_x = boundingBox.x,
                    .position_y = boundingBox.y,
                    .size = (float)renderCommand->config.textElementConfig->fontSize,
                    .spacing = (float)renderCommand->config.textElementConfig->letterSpacing,
                    .color = sfons_rgba(
                        renderCommand->config.textElementConfig->textColor.r,
                        renderCommand->config.textElementConfig->textColor.g,
                        renderCommand->config.textElementConfig->textColor.b,
                        renderCommand->config.textElementConfig->textColor.a
                    )
                });
                free(cloned);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                //Texture2D imageTexture = *(Texture2D *)renderCommand->config.imageElementConfig->imageData;
                // DrawTextureEx(
                // imageTexture,
                // (Vector2){boundingBox.x, boundingBox.y},
                // 0,
                // boundingBox.width / (float)imageTexture.width,
                // WHITE);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                ui_start_scissor((ScissorDescription){
                    .x = (int)roundf(boundingBox.x),
                    .y = (int)roundf(boundingBox.y),
                    .width = (int)roundf(boundingBox.width),
                    .height = (int)roundf(boundingBox.height)
                });
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                ui_end_scissor();
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleElementConfig *config = renderCommand->config.rectangleElementConfig;
                ui_draw_filled_rectangle((RectangleDescription){
                    .start_x = boundingBox.x,
                    .start_y = boundingBox.y,
                    .width = boundingBox.width,
                    .height = boundingBox.height,
                    .red = config->color.r,
                    .green = config->color.g,
                    .blue = config->color.b,
                    .alpha = config->color.a,
                    .top_left_radius = config->cornerRadius.topLeft,
                    .top_right_radius = config->cornerRadius.topRight,
                    .bottom_left_radius = config->cornerRadius.bottomLeft,
                    .bottom_right_radius = config->cornerRadius.bottomRight
                });
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Clay_BorderElementConfig *config = renderCommand->config.borderElementConfig;
                // Left border
                if (config->left.width > 0) {
                    ui_draw_line((LineDescription){
                        .start_x = boundingBox.x,
                        .start_y = boundingBox.y + config->cornerRadius.topLeft,
                        .width = (int)config->left.width,
                        .length = (int)(boundingBox.height - config->cornerRadius.topLeft - config->cornerRadius.bottomLeft),
                        .angle = 270,
                        .red = config->left.color.r,
                        .green = config->left.color.g,
                        .blue = config->left.color.b,
                        .alpha = config->left.color.a
                    });
                }
                // Right border
                if (config->right.width > 0) {
                    ui_draw_line((LineDescription){
                        .start_x = boundingBox.x + boundingBox.width,
                        .start_y = boundingBox.y + config->cornerRadius.topRight,
                        .width = (int)config->right.width,
                        .length = (int)(boundingBox.height - config->cornerRadius.topRight - config->cornerRadius.bottomRight),
                        .angle = 270,
                        .red = config->right.color.r,
                        .green = config->right.color.g,
                        .blue = config->right.color.b,
                        .alpha = config->right.color.a
                    });
                }
                // Top border
                if (config->top.width > 0) {
                    ui_draw_line((LineDescription){
                        .start_x = boundingBox.x + config->cornerRadius.topLeft,
                        .start_y = boundingBox.y,
                        .width = (int)config->top.width,
                        .length = (int)(boundingBox.width - config->cornerRadius.topLeft - config->cornerRadius.topRight),
                        .angle = 0,
                        .red = config->top.color.r,
                        .green = config->top.color.g,
                        .blue = config->top.color.b,
                        .alpha = config->top.color.a
                    });
                    ui_draw_arc((ArcDescription){
                        .x = boundingBox.x + config->cornerRadius.topLeft,
                        .y = boundingBox.y + config->cornerRadius.topLeft,
                        .radius = config->cornerRadius.topLeft,
                        .degree_begin = 90,
                        .degree_end = 180,
                        .thickness = config->top.width,
                        .red = config->top.color.r,
                        .green = config->top.color.g,
                        .blue = config->top.color.b,
                        .alpha = config->top.color.a
                    });
                    ui_draw_arc((ArcDescription){
                        .x = boundingBox.x + boundingBox.width - config->cornerRadius.topRight,
                        .y = boundingBox.y + config->cornerRadius.topRight,
                        .radius = config->cornerRadius.topRight,
                        .degree_begin = 0,
                        .degree_end = 90,
                        .thickness = config->top.width,
                        .red = config->top.color.r,
                        .green = config->top.color.g,
                        .blue = config->top.color.b,
                        .alpha = config->top.color.a
                    });
                }
                // Bottom border
                if (config->bottom.width > 0) {
                    ui_draw_line((LineDescription){
                        .start_x = boundingBox.x + config->cornerRadius.bottomLeft,
                        .start_y = boundingBox.y + boundingBox.height - config->bottom.width,
                        .width = (int)config->top.width,
                        .length = (int)(boundingBox.width - config->cornerRadius.bottomLeft - config->cornerRadius.bottomRight),
                        .angle = 0,
                        .red = config->bottom.color.r,
                        .green = config->bottom.color.g,
                        .blue = config->bottom.color.b,
                        .alpha = config->bottom.color.a
                    });
                    ui_draw_arc((ArcDescription){
                        .x = boundingBox.x + config->cornerRadius.bottomLeft,
                        .y = boundingBox.y + boundingBox.height - config->cornerRadius.bottomLeft,
                        .radius = config->cornerRadius.bottomLeft,
                        .degree_begin = 180,
                        .degree_end = 270,
                        .thickness = config->bottom.width,
                        .red = config->bottom.color.r,
                        .green = config->bottom.color.g,
                        .blue = config->bottom.color.b,
                        .alpha = config->bottom.color.a
                    });
                    ui_draw_arc((ArcDescription){
                        .x = boundingBox.x + boundingBox.width - config->cornerRadius.bottomRight,
                        .y = boundingBox.y + boundingBox.height - config->cornerRadius.bottomRight,
                        .radius = config->cornerRadius.bottomRight,
                        .degree_begin = 270,
                        .degree_end = 360,
                        .thickness = config->bottom.width,
                        .red = config->bottom.color.r,
                        .green = config->bottom.color.g,
                        .blue = config->bottom.color.b,
                        .alpha = config->bottom.color.a
                    });
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
                // 3d stuff apparently
                printf("Error: unhandled render command.");
                break;
            }
            default: {
                printf("Error: unhandled render command.");
                exit(1);
            }
        }
    }
}

static inline Clay_Dimensions ui_measure_text(Clay_StringSlice text, Clay_TextElementConfig *config, uintptr_t userData)
{
    float out_bounds[4];
    // char *chars = (char *)malloc(text.length + 1);
    // memcpy(chars, text.chars, text.length);
    // chars[text.length] = '\0';
    
    char *end = "\0";

    fonsSetSize(state.fons, config->fontSize);
    fonsSetSpacing(state.fons, config->letterSpacing);

	fonsTextBounds(state.fons, 10, 10, text.chars, end, out_bounds);

    //printf("%f,%f,%f,%f -- %s --\n", out_bounds[0], out_bounds[1], out_bounds[2], out_bounds[3], text.chars);

    //free(chars);

    return (Clay_Dimensions) { .height = config->fontSize, .width = out_bounds[0]*text.length };
}