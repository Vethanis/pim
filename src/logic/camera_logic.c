#include "logic/camera_logic.h"

#include "input/input_system.h"
#include "rendering/camera.h"
#include "rendering/r_window.h"
#include "math/quat_funcs.h"
#include "common/time.h"
#include "common/profiler.h"
#include "common/cvar.h"
#include "rendering/lights.h"
#include "rendering/cubemap.h"

static bool ms_placing = true;

static cvar_t cv_pitchscale = { .type = cvart_float,.name = "pitchscale",.value = "2",.minFloat = 0.0f,.maxFloat = 10.0f,.desc = "pitch input sensitivity" };
static cvar_t cv_yawscale = { .type = cvart_float,.name = "yawscale",.value = "1",.minFloat = 0.0f,.maxFloat = 10.0f,.desc = "yaw input sensitivity" };
static cvar_t cv_movescale = { .type = cvart_float,.name = "movescale",.value = "10",.minFloat = 0.0f,.maxFloat = 100.0f,.desc = "movement input sensitivity" };
static cvar_t cv_r_fov =
{
    .type = cvart_float,
    .name = "r_fov",
    .value = "90",
    .minFloat = 1.0f,
    .maxFloat = 170.0f,
    .desc = "Vertical field of fiew, in degrees",
};
static cvar_t cv_r_znear =
{
    .type = cvart_float,
    .name = "r_znear",
    .value = "0.1",
    .minFloat = 0.01f,
    .maxFloat = 1.0f,
    .desc = "Near clipping plane, in meters",
};
static cvar_t cv_r_zfar =
{
    .type = cvart_float,
    .name = "r_zfar",
    .value = "100",
    .minFloat = 1.0f,
    .maxFloat = 1000.0f,
    .desc = "Far clipping plane, in meters",
};

static void LightLogic(const camera_t* cam)
{
    if (lights_pt_count() < 1)
    {
        return;
    }
    if (input_buttonup(MouseButton_Right))
    {
        ms_placing = !ms_placing;
    }
    if (ms_placing)
    {
        pt_light_t light = lights_get_pt(0);
        light.pos = f4_add(cam->position, f4_mulvs(quat_fwd(cam->rotation), 4.0f));

        light.pos.w = 1.0f;
        lights_set_pt(0, light);
        if (input_buttonup(MouseButton_Left))
        {
            lights_add_pt(light);
        }
        if (input_buttonup(MouseButton_Middle))
        {
            i32 ct = lights_pt_count();
            if (ct > 1)
            {
                lights_rm_pt(ct - 1);
            }
        }
    }
}

void camera_logic_init(void)
{
    cvar_reg(&cv_pitchscale);
    cvar_reg(&cv_yawscale);
    cvar_reg(&cv_movescale);
    cvar_reg(&cv_r_fov);
    cvar_reg(&cv_r_znear);
    cvar_reg(&cv_r_zfar);
}

ProfileMark(pm_update, camera_logic_update)
void camera_logic_update(void)
{
    ProfileBegin(pm_update);

    float dYaw = input_delta_axis(MouseAxis_X) * cvar_get_float(&cv_yawscale);
    float dPitch = input_delta_axis(MouseAxis_Y) * cvar_get_float(&cv_pitchscale);

    camera_t camera;
    camera_get(&camera);
    camera.fovy = cvar_get_float(&cv_r_fov);
    camera.zNear = cvar_get_float(&cv_r_znear);
    camera.zFar = cvar_get_float(&cv_r_zfar);
    camera_set(&camera);

    if (input_keyup(KeyCode_Escape))
    {
        window_close(true);
        ProfileEnd(pm_update);
        return;
    }

    GLFWwindow* focus = input_get_focus();

    if (input_keyup(KeyCode_F1))
    {
        input_capture_cursor(focus, !input_cursor_captured(focus));
    }

    if (!input_cursor_captured(focus))
    {
        ProfileEnd(pm_update);
        return;
    }

    const float dt = f1_clamp((float)time_dtf(), 0.0f, 1.0f / 5.0f);
    float moveScale = cvar_get_float(&cv_movescale) * dt;

    quat rot = camera.rotation;
    float4 eye = camera.position;
    float4 fwd = quat_fwd(rot);
    const float4 right = quat_right(rot);
    const float4 up = quat_up(rot);
    const float4 yAxis = { 0.0f, 1.0f, 0.0f, 0.0f };

    if (input_key(KeyCode_W))
    {
        eye = f4_add(eye, f4_mulvs(fwd, moveScale));
    }
    if (input_key(KeyCode_S))
    {
        eye = f4_add(eye, f4_mulvs(fwd, -moveScale));
    }

    if (input_key(KeyCode_D))
    {
        eye = f4_add(eye, f4_mulvs(right, moveScale));
    }
    if (input_key(KeyCode_A))
    {
        eye = f4_add(eye, f4_mulvs(right, -moveScale));
    }

    if (input_key(KeyCode_Space))
    {
        eye = f4_add(eye, f4_mulvs(up, moveScale));
    }
    if (input_key(KeyCode_LeftShift))
    {
        eye = f4_add(eye, f4_mulvs(up, -moveScale));
    }

    float4 at = f4_add(eye, fwd);
    at = f4_add(at, f4_mulvs(right, dYaw));
    at = f4_add(at, f4_mulvs(up, dPitch));
    fwd = f4_normalize3(f4_sub(at, eye));
    rot = quat_lookat(fwd, yAxis);

    camera.position = eye;
    camera.rotation = rot;

    camera_set(&camera);

    LightLogic(&camera);

    ProfileEnd(pm_update);
}

void camera_logic_shutdown(void)
{

}
