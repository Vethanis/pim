#include "render_system.h"

#include <sokol/sokol_app.h>

#include "common/types.h"
#include "common/array.h"

Array(PipelineArray, sg_pipeline);
Array(ShaderArray, sg_shader);
Array(ImageArray, sg_image);
Array(BufferArray, sg_buffer);

static sg_pass_action ms_passAction;
static PipelineArray ms_pipelines;
static ShaderArray ms_shaders;
static ImageArray ms_images;
static BufferArray ms_buffers;

void rendersys_init()
{
    sg_setup(&(sg_desc)
    {
        .mtl_device = sapp_metal_get_device(),
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view,
    });

    ArrGrow(ms_pipelines);
    // ArrBack(ms_pipelines) = sg_make_pipeline(&(sg_pipeline_desc)
    // {
    //     .label = "idk",
    // });
}

void rendersys_update(float dt)
{
    sg_begin_default_pass(&(sg_pass_action)
    {
        .colors[0] = 
        {
            .action = SG_ACTION_CLEAR,
            .val[0] = 0.0f,
            .val[1] = 0.0f,
            .val[2] = 0.25f,
            .val[3] = 0.0f,
        },
    }, sapp_width(), sapp_height());

    sg_end_pass();
    sg_commit();
}

void rendersys_shutdown()
{
    sg_shutdown();
    ArrReset(ms_pipelines);
}
