#pragma once

#include "common/macro.h"
#include "rendering/pt/pt_types_public.h"

PIM_C_BEGIN

void PtSys_Init(void);
void PtSys_Update(void);
void PtSys_Shutdown(void);

PtSampler VEC_CALL PtSampler_Get(void);
void VEC_CALL PtSampler_Set(PtSampler sampler);
float2 VEC_CALL Pt_Sample2D(PtSampler*const pim_noalias sampler);
float VEC_CALL Pt_Sample1D(PtSampler*const pim_noalias sampler);

PtScene* PtScene_New(void);
void PtScene_Update(PtScene* scene);
void PtScene_Del(PtScene* scene);
void PtScene_Gui(PtScene* scene);

void PtTrace_New(PtTrace* trace, PtScene* scene, int2 imageSize);
void PtTrace_Del(PtTrace* trace);
void PtTrace_Gui(PtTrace* trace);

void DofInfo_New(PtDofInfo* dof);
void DofInfo_Gui(PtDofInfo* dof);

PtRayHit VEC_CALL Pt_Intersect(
    PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar);

PtResult VEC_CALL Pt_TraceRay(
    PtSampler*const pim_noalias sampler,
    PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd);

void Pt_Trace(PtTrace* traceDesc, const Camera* camera);

PtResults Pt_RayGen(
    PtScene*const pim_noalias scene,
    float4 origin,
    i32 count);

PIM_C_END
