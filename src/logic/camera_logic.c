#include "logic/camera_logic.h"

#include "input/input_system.h"
#include "rendering/camera.h"
#include "rendering/window.h"
#include "math/quat_funcs.h"
#include "common/time.h"

static float ms_mouseX = 0.0f;
static float ms_mouseY = 0.0f;
static float ms_pitchScale = 720.0f;
static float ms_yawScale = 720.0f;
static float ms_moveScale = 2.0f;

void camera_logic_init(void)
{

}

void camera_logic_update(void)
{
    if (input_get_key(KeyCode_Escape))
    {
        window_close(true);
        return;
    }

    if (input_get_key(KeyCode_F1) == IS_Press)
    {
        window_capture_cursor(!window_cursor_captured());
    }

    if (!window_cursor_captured())
    {
        return;
    }

    camera_t camera;
    camera_get(&camera);

    float dt = (float)time_dtf();
    dt = f1_clamp(dt, 0.0f, 1.0f / 15.0f);
    float moveScale = ms_moveScale * dt;
    float pitchScale = f1_radians(ms_pitchScale * dt);
    float yawScale = f1_radians(ms_yawScale * dt);

    quat rot = camera.rotation;
    float3 eye = camera.position;
    float3 fwd = quat_fwd(rot);
    float3 right = quat_right(rot);
    float3 up = { 0.0f, 1.0f, 0.0f };

    if (input_get_key(KeyCode_W))
    {
        eye = f3_add(eye, f3_mulvs(fwd, moveScale));
    }
    if (input_get_key(KeyCode_S))
    {
        eye = f3_add(eye, f3_mulvs(fwd, -moveScale));
    }

    if (input_get_key(KeyCode_D))
    {
        eye = f3_add(eye, f3_mulvs(right, moveScale));
    }
    if (input_get_key(KeyCode_A))
    {
        eye = f3_add(eye, f3_mulvs(right, -moveScale));
    }

    if (input_get_key(KeyCode_Space))
    {
        eye = f3_add(eye, f3_mulvs(up, moveScale));
    }
    if (input_get_key(KeyCode_LeftShift))
    {
        eye = f3_add(eye, f3_mulvs(up, -dt * ms_moveScale));
    }

    const float mx = input_get_axis(MouseAxis_X);
    const float my = input_get_axis(MouseAxis_Y);
    const float dx = mx - ms_mouseX;
    const float dy = my - ms_mouseY;
    if (fabsf(dx) > 1.0f || fabsf(dy) > 1.0f)
    {
        // sometimes glfw sends out huge input spikes at startup
        // we don't want to introduce these to the game, so we discard them
        return;
    }

    float3 at = f3_add(eye, fwd);
    at = f3_add(at, f3_mulvs(right, dx));
    at = f3_add(at, f3_mulvs(up, dy));
    fwd = f3_normalize(f3_sub(at, eye));
    rot = quat_lookat(fwd, up);

    camera.position = eye;
    camera.rotation = rot;
    ms_mouseX = mx;
    ms_mouseY = my;

    camera_set(&camera);
}

void camera_logic_shutdown(void)
{

}
