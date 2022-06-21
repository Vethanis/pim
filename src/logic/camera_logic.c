#include "logic/camera_logic.h"

#include "input/input_system.h"
#include "rendering/camera.h"
#include "rendering/r_window.h"
#include "math/quat_funcs.h"
#include "common/time.h"
#include "common/profiler.h"
#include "common/cvars.h"
#include "rendering/lights.h"
#include "rendering/cubemap.h"

void CameraLogic_Init(void)
{

}

ProfileMark(pm_update, CameraLogic_Update)
void CameraLogic_Update(void)
{
    ProfileBegin(pm_update);

    float dYaw = Input_GetDeltaAxis(MouseAxis_X) * ConVar_GetFloat(&cv_in_yawscale);
    float dPitch = Input_GetDeltaAxis(MouseAxis_Y) * ConVar_GetFloat(&cv_in_pitchscale);

    Camera camera;
    Camera_Get(&camera);
    camera.fovy = ConVar_GetFloat(&cv_r_fov);
    camera.zNear = ConVar_GetFloat(&cv_r_znear);
    camera.zFar = ConVar_GetFloat(&cv_r_zfar);
    Camera_Set(&camera);

    if (Input_IsKeyUp(KeyCode_Escape))
    {
        Window_Close(true);
        goto cleanup;
    }

    GLFWwindow* focus = Input_GetFocus();

    if (Input_IsKeyUp(KeyCode_F1))
    {
        Input_CaptureCursor(focus, !Input_IsCursorCaptured(focus));
    }

    if (!Input_IsCursorCaptured(focus))
    {
        goto cleanup;
    }

    const float dt = f1_clamp((float)Time_Deltaf(), 0.0f, 1.0f / 5.0f);
    float moveScale = ConVar_GetFloat(&cv_in_movescale) * dt;

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

    fwd = f4_add(fwd, f4_mulvs(right, dYaw));
    fwd = f4_add(fwd, f4_mulvs(up, dPitch));
    fwd = f4_normalize3(fwd);
    rot = quat_lookat(fwd, yAxis);

    camera.position = eye;
    camera.rotation = rot;

    Camera_Set(&camera);

cleanup:
    ProfileEnd(pm_update);
}

void CameraLogic_Shutdown(void)
{

}
