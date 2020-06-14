#pragma once

#include "common/macro.h"
#include "common/library.h"
#include <embree3/rtcore.h>

PIM_C_BEGIN

typedef struct rtc_s
{
    library_t lib;

    // Device API -------------------------------------------------------------

    RTCDevice(*NewDevice)(const char* config);
    void(*RetainDevice)(RTCDevice device);
    void(*ReleaseDevice)(RTCDevice device);

    ssize_t(*GetDeviceProperty)(RTCDevice device, enum RTCDeviceProperty prop);
    void(*SetDeviceProperty)(RTCDevice device, const enum RTCDeviceProperty prop, ssize_t value);

    enum RTCError(*GetDeviceError)(RTCDevice device);
    void(*SetDeviceErrorFunction)(RTCDevice device, RTCErrorFunction error, void* userPtr);
    void(*SetDeviceMemoryMonitorFunction)(RTCDevice device, RTCMemoryMonitorFunction memoryMonitor, void* userPtr);

    // Buffer API -------------------------------------------------------------

    RTCBuffer(*NewBuffer)(RTCDevice device, size_t byteSize);
    RTCBuffer(*NewSharedBuffer)(RTCDevice device, void* ptr, size_t byteSize);
    void* (*GetBufferData)(RTCBuffer buffer);
    void(*RetainBuffer)(RTCBuffer buffer);
    void(*ReleaseBuffer)(RTCBuffer buffer);

    // Geometry API -----------------------------------------------------------

    RTCGeometry(*NewGeometry)(RTCDevice device, enum RTCGeometryType type);
    void(*RetainGeometry)(RTCGeometry geometry);
    void(*ReleaseGeometry)(RTCGeometry geometry);
    void(*CommitGeometry)(RTCGeometry geometry);
    void(*EnableGeometry)(RTCGeometry geometry);
    void(*DisableGeometry)(RTCGeometry geometry);
    void(*SetGeometryTimeStepCount)(RTCGeometry geometry, unsigned int timeStepCount);
    void(*SetGeometryTimeRange)(RTCGeometry geometry, float startTime, float endTime);
    void(*SetGeometryVertexAttributeCount)(RTCGeometry geometry, unsigned int vertexAttributeCount);
    void(*SetGeometryMask)(RTCGeometry geometry, unsigned int mask);
    void(*SetGeometryBuildQuality)(RTCGeometry geometry, enum RTCBuildQuality quality);

    void(*SetGeometryBuffer)(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot, enum RTCFormat format, RTCBuffer buffer, size_t byteOffset, size_t byteStride, size_t itemCount);

    void(*SetSharedGeometryBuffer)(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot, enum RTCFormat format, const void* ptr, size_t byteOffset, size_t byteStride, size_t itemCount);

    void* (*SetNewGeometryBuffer)(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot, enum RTCFormat format, size_t byteStride, size_t itemCount);

    void* (*GetGeometryBufferData)(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot);

    void(*UpdateGeometryBuffer)(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot);


    void(*SetGeometryIntersectFilterFunction)(RTCGeometry geometry, RTCFilterFunctionN filter);

    void(*SetGeometryOccludedFilterFunction)(RTCGeometry geometry, RTCFilterFunctionN filter);

    void(*SetGeometryUserData)(RTCGeometry geometry, void* ptr);
    void* (*GetGeometryUserData)(RTCGeometry geometry);

    void(*SetGeometryPointQueryFunction)(RTCGeometry geometry, RTCPointQueryFunction pointQuery);

    void(*SetGeometryUserPrimitiveCount)(RTCGeometry geometry, unsigned int userPrimitiveCount);

    void(*SetGeometryBoundsFunction)(RTCGeometry geometry, RTCBoundsFunction bounds, void* userPtr);

    void(*SetGeometryIntersectFunction)(RTCGeometry geometry, RTCIntersectFunctionN intersect);

    void(*SetGeometryOccludedFunction)(RTCGeometry geometry, RTCOccludedFunctionN occluded);

    void(*FilterIntersection)(const struct RTCIntersectFunctionNArguments* args, const struct RTCFilterFunctionNArguments* filterArgs);

    void(*FilterOcclusion)(const struct RTCOccludedFunctionNArguments* args, const struct RTCFilterFunctionNArguments* filterArgs);

    void(*SetGeometryInstancedScene)(RTCGeometry geometry, RTCScene scene);

    void(*SetGeometryTransform)(RTCGeometry geometry, unsigned int timeStep, enum RTCFormat format, const void* xfm);

    void(*SetGeometryTransformQuaternion)(RTCGeometry geometry, unsigned int timeStep, const struct RTCQuaternionDecomposition* qd);

    void(*GetGeometryTransform)(RTCGeometry geometry, float time, enum RTCFormat format, void* xfm);

    void(*SetGeometryTessellationRate)(RTCGeometry geometry, float tessellationRate);

    void(*SetGeometryTopologyCount)(RTCGeometry geometry, unsigned int topologyCount);

    void(*SetGeometrySubdivisionMode)(RTCGeometry geometry, unsigned int topologyID, enum RTCSubdivisionMode mode);

    void(*SetGeometryVertexAttributeTopology)(RTCGeometry geometry, unsigned int vertexAttributeID, unsigned int topologyID);

    void(*SetGeometryDisplacementFunction)(RTCGeometry geometry, RTCDisplacementFunctionN displacement);

    unsigned int(*GetGeometryFirstHalfEdge)(RTCGeometry geometry, unsigned int faceID);

    unsigned int(*GetGeometryFace)(RTCGeometry geometry, unsigned int edgeID);

    unsigned int(*GetGeometryNextHalfEdge)(RTCGeometry geometry, unsigned int edgeID);

    unsigned int(*GetGeometryPreviousHalfEdge)(RTCGeometry geometry, unsigned int edgeID);

    unsigned int(*GetGeometryOppositeHalfEdge)(RTCGeometry geometry, unsigned int topologyID, unsigned int edgeID);

    void(*Interpolate)(const struct RTCInterpolateArguments* args);
    void(*InterpolateN)(const struct RTCInterpolateNArguments* args);

    // Scene API --------------------------------------------------------------

    RTCScene(*NewScene)(RTCDevice device);
    RTCDevice(*GetSceneDevice)(RTCScene hscene);
    void(*RetainScene)(RTCScene scene);
    void(*ReleaseScene)(RTCScene scene);
    void(*CommitScene)(RTCScene scene);
    void(*JoinCommitScene)(RTCScene scene);

    unsigned int(*AttachGeometry)(RTCScene scene, RTCGeometry geometry);
    void(*AttachGeometryByID)(RTCScene scene, RTCGeometry geometry, unsigned int geomID);
    void(*DetachGeometry)(RTCScene scene, unsigned int geomID);
    RTCGeometry(*GetGeometry)(RTCScene scene, unsigned int geomID);

    void(*SetSceneProgressMonitorFunction)(RTCScene scene, RTCProgressMonitorFunction progress, void* ptr);

    void(*SetSceneBuildQuality)(RTCScene scene, enum RTCBuildQuality quality);

    void(*SetSceneFlags)(RTCScene scene, enum RTCSceneFlags flags);
    enum RTCSceneFlags(*GetSceneFlags)(RTCScene scene);

    void(*GetSceneBounds)(RTCScene scene, struct RTCBounds* bounds_o);
    void(*GetSceneLinearBounds)(RTCScene scene, struct RTCLinearBounds* bounds_o);

    bool(*PointQuery)(RTCScene scene, struct RTCPointQuery* query, struct RTCPointQueryContext* context, RTCPointQueryFunction queryFunc, void* userPtr);

    bool(*PointQuery4)(const int* valid, RTCScene scene, struct RTCPointQuery4* query, struct RTCPointQueryContext* context, RTCPointQueryFunction queryFunc, void** userPtr);

    bool(*PointQuery8)(const int* valid, RTCScene scene, struct RTCPointQuery8* query, struct RTCPointQueryContext* context, RTCPointQueryFunction queryFunc, void** userPtr);

    bool(*PointQuery16)(const int* valid, RTCScene scene, struct RTCPointQuery16* query, struct RTCPointQueryContext* context, RTCPointQueryFunction queryFunc, void** userPtr);

    void(*Intersect1)(RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit* rayhit);

    void(*Intersect4)(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit4* rayhit);

    void(*Intersect8)(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit8* rayhit);

    void(*Intersect16)(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit16* rayhit);

    void(*Intersect1M)(RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit* rayhit, unsigned int M, size_t byteStride);

    void(*Intersect1Mp)(RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHit** rayhit, unsigned int M);

    void(*IntersectNM)(RTCScene scene, struct RTCIntersectContext* context, struct RTCRayHitN* rayhit, unsigned int N, unsigned int M, size_t byteStride);

    void(*IntersectNp)(RTCScene scene, struct RTCIntersectContext* context, const struct RTCRayHitNp* rayhit, unsigned int N);

    void(*Occluded1)(RTCScene scene, struct RTCIntersectContext* context, struct RTCRay* ray);

    void(*Occluded4)(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRay4* ray);

    void(*Occluded8)(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRay8* ray);

    void(*Occluded16)(const int* valid, RTCScene scene, struct RTCIntersectContext* context, struct RTCRay16* ray);

    void(*Occluded1M)(RTCScene scene, struct RTCIntersectContext* context, struct RTCRay* ray, unsigned int M, size_t byteStride);

    void(*Occluded1Mp)(RTCScene scene, struct RTCIntersectContext* context, struct RTCRay** ray, unsigned int M);

    void(*OccludedNM)(RTCScene scene, struct RTCIntersectContext* context, struct RTCRayN* ray, unsigned int N, unsigned int M, size_t byteStride);

    void(*OccludedNp)(RTCScene scene, struct RTCIntersectContext* context, const struct RTCRayNp* ray, unsigned int N);

    void(*Collide)(RTCScene scene0, RTCScene scene1, RTCCollideFunc callback, void* userPtr);

    // BVH API ----------------------------------------------------------------

    RTCBVH(*NewBVH)(RTCDevice device);
    void(*RetainBVH)(RTCBVH bvh);
    void(*ReleaseBVH)(RTCBVH bvh);
    void* (*BuildBVH)(const struct RTCBuildArguments* args);

    void* (*ThreadLocalAlloc)(RTCThreadLocalAllocator allocator, size_t bytes, size_t align);

} rtc_t;

extern rtc_t rtc;

bool rtc_init(void);
void rtc_shutdown(void);

PIM_C_END
