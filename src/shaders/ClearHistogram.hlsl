#include "Exposure.hlsl"

[numthreads(32, 1, 1)]
void CSMain(uint3 tid3 : SV_DispatchThreadID)
{
    uint i = tid3.x;
    if (i < R_ExposureHistogramSize)
    {
        HistogramBuffer[i] = 0;
    }
}
