#include "rendering/librtc.h"
#include <string.h>

static bool ms_once;
rtc_t rtc;

#define RTC_LOAD(name) rtc.name = Library_Sym(rtc.lib, STR_TOK(rtc##name)); ASSERT(rtc.name)

bool rtc_init(void)
{
    if (!ms_once)
    {
        ms_once = true;

        rtc.lib = Library_Open("embree3");
        if (!rtc.lib.handle)
        {
            return false;
        }

        RTC_LOAD(NewDevice);
        RTC_LOAD(RetainDevice);
        RTC_LOAD(ReleaseDevice);
        RTC_LOAD(GetDeviceProperty);
        RTC_LOAD(SetDeviceProperty);
        RTC_LOAD(GetDeviceError);
        RTC_LOAD(SetDeviceErrorFunction);
        RTC_LOAD(SetDeviceMemoryMonitorFunction);

        RTC_LOAD(NewBuffer);
        RTC_LOAD(NewSharedBuffer);
        RTC_LOAD(GetBufferData);
        RTC_LOAD(RetainBuffer);
        RTC_LOAD(ReleaseBuffer);

        RTC_LOAD(NewGeometry);
        RTC_LOAD(RetainGeometry);
        RTC_LOAD(ReleaseGeometry);
        RTC_LOAD(CommitGeometry);
        RTC_LOAD(EnableGeometry);
        RTC_LOAD(DisableGeometry);
        RTC_LOAD(SetGeometryTimeStepCount);
        RTC_LOAD(SetGeometryTimeRange);
        RTC_LOAD(SetGeometryVertexAttributeCount);
        RTC_LOAD(SetGeometryMask);
        RTC_LOAD(SetGeometryBuildQuality);
        RTC_LOAD(SetGeometryBuffer);
        RTC_LOAD(SetSharedGeometryBuffer);
        RTC_LOAD(SetNewGeometryBuffer);
        RTC_LOAD(GetGeometryBufferData);
        RTC_LOAD(UpdateGeometryBuffer);
        RTC_LOAD(SetGeometryIntersectFilterFunction);
        RTC_LOAD(SetGeometryOccludedFilterFunction);
        RTC_LOAD(SetGeometryUserData);
        RTC_LOAD(GetGeometryUserData);
        RTC_LOAD(SetGeometryPointQueryFunction);
        RTC_LOAD(SetGeometryUserPrimitiveCount);
        RTC_LOAD(SetGeometryBoundsFunction);
        RTC_LOAD(SetGeometryIntersectFunction);
        RTC_LOAD(SetGeometryOccludedFunction);
        RTC_LOAD(FilterIntersection);
        RTC_LOAD(FilterOcclusion);
        RTC_LOAD(SetGeometryInstancedScene);
        RTC_LOAD(SetGeometryTransform);
        RTC_LOAD(SetGeometryTransformQuaternion);
        RTC_LOAD(GetGeometryTransform);
        RTC_LOAD(SetGeometryTessellationRate);
        RTC_LOAD(SetGeometryTopologyCount);
        RTC_LOAD(SetGeometrySubdivisionMode);
        RTC_LOAD(SetGeometryVertexAttributeTopology);
        RTC_LOAD(SetGeometryDisplacementFunction);
        RTC_LOAD(GetGeometryFirstHalfEdge);
        RTC_LOAD(GetGeometryFace);
        RTC_LOAD(GetGeometryNextHalfEdge);
        RTC_LOAD(GetGeometryPreviousHalfEdge);
        RTC_LOAD(GetGeometryOppositeHalfEdge);
        RTC_LOAD(Interpolate);
        RTC_LOAD(InterpolateN);

        RTC_LOAD(NewScene);
        RTC_LOAD(GetSceneDevice);
        RTC_LOAD(RetainScene);
        RTC_LOAD(ReleaseScene);
        RTC_LOAD(CommitScene);
        RTC_LOAD(JoinCommitScene);
        RTC_LOAD(AttachGeometry);
        RTC_LOAD(AttachGeometryByID);
        RTC_LOAD(DetachGeometry);
        RTC_LOAD(GetGeometry);
        RTC_LOAD(SetSceneProgressMonitorFunction);
        RTC_LOAD(SetSceneBuildQuality);
        RTC_LOAD(SetSceneFlags);
        RTC_LOAD(GetSceneFlags);
        RTC_LOAD(GetSceneBounds);
        RTC_LOAD(GetSceneLinearBounds);
        RTC_LOAD(PointQuery);
        RTC_LOAD(PointQuery4);
        RTC_LOAD(PointQuery8);
        RTC_LOAD(PointQuery16);
        RTC_LOAD(Intersect1);
        RTC_LOAD(Intersect4);
        RTC_LOAD(Intersect8);
        RTC_LOAD(Intersect16);
        RTC_LOAD(Intersect1M);
        RTC_LOAD(Intersect1Mp);
        RTC_LOAD(IntersectNM);
        RTC_LOAD(IntersectNp);
        RTC_LOAD(Occluded1);
        RTC_LOAD(Occluded4);
        RTC_LOAD(Occluded8);
        RTC_LOAD(Occluded16);
        RTC_LOAD(Occluded1M);
        RTC_LOAD(Occluded1Mp);
        RTC_LOAD(OccludedNM);
        RTC_LOAD(OccludedNp);
        RTC_LOAD(Collide);

        RTC_LOAD(NewBVH);
        RTC_LOAD(RetainBVH);
        RTC_LOAD(ReleaseBVH);
        RTC_LOAD(BuildBVH);

        RTC_LOAD(ThreadLocalAlloc);
    }
    return rtc.lib.handle != NULL;
}

void rtc_shutdown(void)
{
    ms_once = false;
    if (rtc.lib.handle)
    {
        Library_Close(rtc.lib);
        memset(&rtc, 0, sizeof(rtc));
    }
}
