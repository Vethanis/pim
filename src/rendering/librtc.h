#pragma once

#include "common/macro.h"
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

PIM_C_END
