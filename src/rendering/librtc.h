#pragma once

#include "common/macro.h"
#include "common/library.h"
#include <embree3/rtcore.h>

PIM_C_BEGIN

typedef struct RTCIntersectFunctionNArguments RTCIntersectFunctionNArguments;
typedef struct RTCFilterFunctionNArguments RTCFilterFunctionNArguments;
typedef struct RTCOccludedFunctionNArguments RTCOccludedFunctionNArguments;
typedef struct RTCQuaternionDecomposition RTCQuaternionDecomposition;
typedef struct RTCInterpolateArguments RTCInterpolateArguments;
typedef struct RTCInterpolateNArguments RTCInterpolateNArguments;
typedef struct RTCBounds RTCBounds;
typedef struct RTCLinearBounds RTCLinearBounds;
typedef struct RTCPointQuery RTCPointQuery;
typedef struct RTCPointQueryContext RTCPointQueryContext;
typedef struct RTCPointQueryFunctionArguments RTCPointQueryFunctionArguments;
typedef struct RTCPointQuery4 RTCPointQuery4;
typedef struct RTCPointQuery8 RTCPointQuery8;
typedef struct RTCPointQuery16 RTCPointQuery16;
typedef struct RTCIntersectContext RTCIntersectContext;
typedef struct RTCRayHit RTCRayHit;
typedef struct RTCRayHit4 RTCRayHit4;
typedef struct RTCRayHit8 RTCRayHit8;
typedef struct RTCRayHit16 RTCRayHit16;
typedef struct RTCRayHitN RTCRayHitN;
typedef struct RTCRayHitNp RTCRayHitNp;
typedef struct RTCRay RTCRay;
typedef struct RTCRay4 RTCRay4;
typedef struct RTCRay8 RTCRay8;
typedef struct RTCRay16 RTCRay16;
typedef struct RTCRayN RTCRayN;
typedef struct RTCRayNp RTCRayNp;
typedef struct RTCBuildArguments RTCBuildArguments;

typedef enum RTCError RTCError;
typedef enum RTCDeviceProperty RTCDeviceProperty;
typedef enum RTCGeometryType RTCGeometryType;
typedef enum RTCBuildQuality RTCBuildQuality;
typedef enum RTCBufferType RTCBufferType;
typedef enum RTCFormat RTCFormat;
typedef enum RTCSubdivisionMode RTCSubdivisionMode;
typedef enum RTCSceneFlags RTCSceneFlags;

typedef struct LibRtc_s
{
    Library lib;

    // Device API -------------------------------------------------------------

    RTCDevice(*NewDevice)(const char* config);
    void(*RetainDevice)(RTCDevice device);
    void(*ReleaseDevice)(RTCDevice device);

    ssize_t(*GetDeviceProperty)(RTCDevice device, RTCDeviceProperty prop);
    void(*SetDeviceProperty)(RTCDevice device, const RTCDeviceProperty prop, ssize_t value);

    RTCError(*GetDeviceError)(RTCDevice device);
    void(*SetDeviceErrorFunction)(RTCDevice device, RTCErrorFunction error, void* userPtr);
    void(*SetDeviceMemoryMonitorFunction)(RTCDevice device, RTCMemoryMonitorFunction memoryMonitor, void* userPtr);

    // Buffer API -------------------------------------------------------------

    RTCBuffer(*NewBuffer)(RTCDevice device, size_t byteSize);
    RTCBuffer(*NewSharedBuffer)(RTCDevice device, void* ptr, size_t byteSize);
    void* (*GetBufferData)(RTCBuffer buffer);
    void(*RetainBuffer)(RTCBuffer buffer);
    void(*ReleaseBuffer)(RTCBuffer buffer);

    // Geometry API -----------------------------------------------------------

    RTCGeometry(*NewGeometry)(RTCDevice device, RTCGeometryType type);
    void(*RetainGeometry)(RTCGeometry geometry);
    void(*ReleaseGeometry)(RTCGeometry geometry);
    void(*CommitGeometry)(RTCGeometry geometry);
    void(*EnableGeometry)(RTCGeometry geometry);
    void(*DisableGeometry)(RTCGeometry geometry);
    void(*SetGeometryTimeStepCount)(RTCGeometry geometry, unsigned int timeStepCount);
    void(*SetGeometryTimeRange)(RTCGeometry geometry, float startTime, float endTime);
    void(*SetGeometryVertexAttributeCount)(RTCGeometry geometry, unsigned int vertexAttributeCount);
    void(*SetGeometryMask)(RTCGeometry geometry, unsigned int mask);
    void(*SetGeometryBuildQuality)(RTCGeometry geometry, RTCBuildQuality quality);

    void(*SetGeometryBuffer)(RTCGeometry geometry, RTCBufferType type, unsigned int slot, RTCFormat format, RTCBuffer buffer, size_t byteOffset, size_t byteStride, size_t itemCount);

    void(*SetSharedGeometryBuffer)(RTCGeometry geometry, RTCBufferType type, unsigned int slot, RTCFormat format, const void* ptr, size_t byteOffset, size_t byteStride, size_t itemCount);

    void* (*SetNewGeometryBuffer)(RTCGeometry geometry, RTCBufferType type, unsigned int slot, RTCFormat format, size_t byteStride, size_t itemCount);

    void* (*GetGeometryBufferData)(RTCGeometry geometry, RTCBufferType type, unsigned int slot);

    void(*UpdateGeometryBuffer)(RTCGeometry geometry, RTCBufferType type, unsigned int slot);


    void(*SetGeometryIntersectFilterFunction)(RTCGeometry geometry, RTCFilterFunctionN filter);

    void(*SetGeometryOccludedFilterFunction)(RTCGeometry geometry, RTCFilterFunctionN filter);

    void(*SetGeometryUserData)(RTCGeometry geometry, void* ptr);
    void* (*GetGeometryUserData)(RTCGeometry geometry);

    void(*SetGeometryPointQueryFunction)(RTCGeometry geometry, RTCPointQueryFunction pointQuery);

    void(*SetGeometryUserPrimitiveCount)(RTCGeometry geometry, unsigned int userPrimitiveCount);

    void(*SetGeometryBoundsFunction)(RTCGeometry geometry, RTCBoundsFunction bounds, void* userPtr);

    void(*SetGeometryIntersectFunction)(RTCGeometry geometry, RTCIntersectFunctionN intersect);

    void(*SetGeometryOccludedFunction)(RTCGeometry geometry, RTCOccludedFunctionN occluded);

    void(*FilterIntersection)(const RTCIntersectFunctionNArguments* args, const RTCFilterFunctionNArguments* filterArgs);

    void(*FilterOcclusion)(const RTCOccludedFunctionNArguments* args, const RTCFilterFunctionNArguments* filterArgs);

    void(*SetGeometryInstancedScene)(RTCGeometry geometry, RTCScene scene);

    void(*SetGeometryTransform)(RTCGeometry geometry, unsigned int timeStep, RTCFormat format, const void* xfm);

    void(*SetGeometryTransformQuaternion)(RTCGeometry geometry, unsigned int timeStep, const RTCQuaternionDecomposition* qd);

    void(*GetGeometryTransform)(RTCGeometry geometry, float time, RTCFormat format, void* xfm);

    void(*SetGeometryTessellationRate)(RTCGeometry geometry, float tessellationRate);

    void(*SetGeometryTopologyCount)(RTCGeometry geometry, unsigned int topologyCount);

    void(*SetGeometrySubdivisionMode)(RTCGeometry geometry, unsigned int topologyID, RTCSubdivisionMode mode);

    void(*SetGeometryVertexAttributeTopology)(RTCGeometry geometry, unsigned int vertexAttributeID, unsigned int topologyID);

    void(*SetGeometryDisplacementFunction)(RTCGeometry geometry, RTCDisplacementFunctionN displacement);

    unsigned int(*GetGeometryFirstHalfEdge)(RTCGeometry geometry, unsigned int faceID);

    unsigned int(*GetGeometryFace)(RTCGeometry geometry, unsigned int edgeID);

    unsigned int(*GetGeometryNextHalfEdge)(RTCGeometry geometry, unsigned int edgeID);

    unsigned int(*GetGeometryPreviousHalfEdge)(RTCGeometry geometry, unsigned int edgeID);

    unsigned int(*GetGeometryOppositeHalfEdge)(RTCGeometry geometry, unsigned int topologyID, unsigned int edgeID);

    void(*Interpolate)(const RTCInterpolateArguments* args);
    void(*InterpolateN)(const RTCInterpolateNArguments* args);

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

    void(*SetSceneBuildQuality)(RTCScene scene, RTCBuildQuality quality);

    void(*SetSceneFlags)(RTCScene scene, RTCSceneFlags flags);
    RTCSceneFlags(*GetSceneFlags)(RTCScene scene);

    void(*GetSceneBounds)(RTCScene scene, RTCBounds* bounds_o);
    void(*GetSceneLinearBounds)(RTCScene scene, RTCLinearBounds* bounds_o);

    bool(*PointQuery)(RTCScene scene, RTCPointQuery* query, RTCPointQueryContext* context, RTCPointQueryFunction queryFunc, void* userPtr);

    bool(*PointQuery4)(const int* valid, RTCScene scene, RTCPointQuery4* query, RTCPointQueryContext* context, RTCPointQueryFunction queryFunc, void** userPtr);

    bool(*PointQuery8)(const int* valid, RTCScene scene, RTCPointQuery8* query, RTCPointQueryContext* context, RTCPointQueryFunction queryFunc, void** userPtr);

    bool(*PointQuery16)(const int* valid, RTCScene scene, RTCPointQuery16* query, RTCPointQueryContext* context, RTCPointQueryFunction queryFunc, void** userPtr);

    void(*Intersect1)(RTCScene scene, RTCIntersectContext* context, RTCRayHit* rayhit);

    void(*Intersect4)(const int* valid, RTCScene scene, RTCIntersectContext* context, RTCRayHit4* rayhit);

    void(*Intersect8)(const int* valid, RTCScene scene, RTCIntersectContext* context, RTCRayHit8* rayhit);

    void(*Intersect16)(const int* valid, RTCScene scene, RTCIntersectContext* context, RTCRayHit16* rayhit);

    void(*Intersect1M)(RTCScene scene, RTCIntersectContext* context, RTCRayHit* rayhit, unsigned int M, size_t byteStride);

    void(*Intersect1Mp)(RTCScene scene, RTCIntersectContext* context, RTCRayHit** rayhit, unsigned int M);

    void(*IntersectNM)(RTCScene scene, RTCIntersectContext* context, RTCRayHitN* rayhit, unsigned int N, unsigned int M, size_t byteStride);

    void(*IntersectNp)(RTCScene scene, RTCIntersectContext* context, const RTCRayHitNp* rayhit, unsigned int N);

    void(*Occluded1)(RTCScene scene, RTCIntersectContext* context, RTCRay* ray);

    void(*Occluded4)(const int* valid, RTCScene scene, RTCIntersectContext* context, RTCRay4* ray);

    void(*Occluded8)(const int* valid, RTCScene scene, RTCIntersectContext* context, RTCRay8* ray);

    void(*Occluded16)(const int* valid, RTCScene scene, RTCIntersectContext* context, RTCRay16* ray);

    void(*Occluded1M)(RTCScene scene, RTCIntersectContext* context, RTCRay* ray, unsigned int M, size_t byteStride);

    void(*Occluded1Mp)(RTCScene scene, RTCIntersectContext* context, RTCRay** ray, unsigned int M);

    void(*OccludedNM)(RTCScene scene, RTCIntersectContext* context, RTCRayN* ray, unsigned int N, unsigned int M, size_t byteStride);

    void(*OccludedNp)(RTCScene scene, RTCIntersectContext* context, const RTCRayNp* ray, unsigned int N);

    void(*Collide)(RTCScene scene0, RTCScene scene1, RTCCollideFunc callback, void* userPtr);

    // BVH API ----------------------------------------------------------------

    RTCBVH(*NewBVH)(RTCDevice device);
    void(*RetainBVH)(RTCBVH bvh);
    void(*ReleaseBVH)(RTCBVH bvh);
    void* (*BuildBVH)(const RTCBuildArguments* args);

    void* (*ThreadLocalAlloc)(RTCThreadLocalAllocator allocator, size_t bytes, size_t align);

} LibRtc;

extern LibRtc rtc;

bool LibRtc_Init(void);
void LibRtc_Shutdown(void);

PIM_C_END
