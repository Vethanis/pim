#include "logic/camera_logic.h"

#include "input/input_system.h"
#include "rendering/camera.h"
#include "rendering/window.h"
#include "math/quat_funcs.h"
#include "common/time.h"
#include "common/profiler.h"

static float ms_pitchScale = 720.0f;
static float ms_yawScale = 720.0f;
static float ms_moveScale = 5.0f;

void camera_logic_init(void)
{

}

ProfileMark(pm_update, camera_logic_update)
void camera_logic_update(void)
{
    ProfileBegin(pm_update);

    const float dx = input_delta_axis(MouseAxis_X);
    const float dy = input_delta_axis(MouseAxis_Y);
    const float dscroll = input_delta_axis(MouseAxis_ScrollY);
    camera_t camera;
    camera_get(&camera);

    if (input_keyup(KeyCode_Escape))
    {
        window_close(true);
        ProfileEnd(pm_update);
        return;
    }

    if (input_keyup(KeyCode_F1))
    {
        window_capture_cursor(!window_cursor_captured());
    }

    if (!window_cursor_captured())
    {
        ProfileEnd(pm_update);
        return;
    }

    const float dt = f1_clamp((float)time_dtf(), 0.0f, 1.0f / 15.0f);
    float moveScale = ms_moveScale * dt;
    float pitchScale = f1_radians(ms_pitchScale * dt);
    float yawScale = f1_radians(ms_yawScale * dt);

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
        eye = f4_add(eye, f4_mulvs(up, -dt * ms_moveScale));
    }

    float4 at = f4_add(eye, fwd);
    at = f4_add(at, f4_mulvs(right, dx));
    at = f4_add(at, f4_mulvs(up, dy));
    fwd = f4_normalize3(f4_sub(at, eye));
    rot = quat_lookat(fwd, yAxis);

    camera.fovy = f1_clamp(camera.fovy + dscroll, 30.0f, 150.0f);

    camera.position = eye;
    camera.rotation = rot;

    camera_set(&camera);

    ProfileEnd(pm_update);
}

void camera_logic_shutdown(void)
{

}
