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

static ConVar_t cv_pitchscale = { .type = cvart_float,.name = "pitchscale",.value = "2",.minFloat = 0.0f,.maxFloat = 10.0f,.desc = "pitch input sensitivity" };
static ConVar_t cv_yawscale = { .type = cvart_float,.name = "yawscale",.value = "1",.minFloat = 0.0f,.maxFloat = 10.0f,.desc = "yaw input sensitivity" };
static ConVar_t cv_movescale = { .type = cvart_float,.name = "movescale",.value = "8",.minFloat = 0.0f,.maxFloat = 100.0f,.desc = "movement input sensitivity" };
static ConVar_t cv_r_fov =
{
    .type = cvart_float,
    .name = "r_fov",
    .value = "90",
    .minFloat = 1.0f,
    .maxFloat = 170.0f,
    .desc = "Vertical field of fiew, in degrees",
};
static ConVar_t cv_r_znear =
{
    .type = cvart_float,
    .name = "r_znear",
    .value = "0.1",
    .minFloat = 0.01f,
    .maxFloat = 1.0f,
    .desc = "Near clipping plane, in meters",
};
static ConVar_t cv_r_zfar =
{
    .type = cvart_float,
    .name = "r_zfar",
    .value = "500",
    .minFloat = 1.0f,
    .maxFloat = 1000.0f,
    .desc = "Far clipping plane, in meters",
};

void CameraLogic_Init(void)
{
    cvar_reg(&cv_pitchscale);
    cvar_reg(&cv_yawscale);
    cvar_reg(&cv_movescale);
    cvar_reg(&cv_r_fov);
    cvar_reg(&cv_r_znear);
    cvar_reg(&cv_r_zfar);
}

ProfileMark(pm_update, CameraLogic_Update)
void CameraLogic_Update(void)
{
    ProfileBegin(pm_update);

    float dYaw = Input_GetDeltaAxis(MouseAxis_X) * cvar_get_float(&cv_yawscale);
    float dPitch = Input_GetDeltaAxis(MouseAxis_Y) * cvar_get_float(&cv_pitchscale);

    Camera camera;
    Camera_Get(&camera);
    camera.fovy = cvar_get_float(&cv_r_fov);
    camera.zNear = cvar_get_float(&cv_r_znear);
    camera.zFar = cvar_get_float(&cv_r_zfar);
    Camera_Set(&camera);

    if (Input_IsKeyUp(KeyCode_Escape))
    {
        Window_Close(true);
        ProfileEnd(pm_update);
        return;
    }

    GLFWwindow* focus = Input_GetFocus();

    if (Input_IsKeyUp(KeyCode_F1))
    {
        Input_CaptureCursor(focus, !Input_IsCursorCaptured(focus));
    }

    if (!Input_IsCursorCaptured(focus))
    {
        ProfileEnd(pm_update);
        return;
    }

    const float dt = f1_clamp((float)Time_Deltaf(), 0.0f, 1.0f / 5.0f);
    float moveScale = cvar_get_float(&cv_movescale) * dt;

    quat rot = camera.rotation;
    float4 eye = camera.position;
    float4 fwd = quat_fwd(rot);
    const float4 right = quat_right(rot);
    const float4 up = quat_up(rot);
    const float4 yAxis = { 0.0f, 1.0f, 0.0f, 0.0f };

    if (Input_GetKey(KeyCode_W))
    {
        eye = f4_add(eye, f4_mulvs(fwd, moveScale));
    }
    if (Input_GetKey(KeyCode_S))
    {
        eye = f4_add(eye, f4_mulvs(fwd, -moveScale));
    }

    if (Input_GetKey(KeyCode_D))
    {
        eye = f4_add(eye, f4_mulvs(right, moveScale));
    }
    if (Input_GetKey(KeyCode_A))
    {
        eye = f4_add(eye, f4_mulvs(right, -moveScale));
    }

    if (Input_GetKey(KeyCode_Space))
    {
        eye = f4_add(eye, f4_mulvs(up, moveScale));
    }
    if (Input_GetKey(KeyCode_LeftShift))
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

    Camera_Set(&camera);

    ProfileEnd(pm_update);
}

void CameraLogic_Shutdown(void)
{

}
