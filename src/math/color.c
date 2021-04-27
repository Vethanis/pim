#include "math/color.h"

#include "math/float3x3_funcs.h"
#include "common/stringutil.h"
#include "io/fstr.h"

static const float4x2 kRec709 =
{
    { 0.64f, 0.3f, 0.15f, 0.3127f },
    { 0.33f, 0.6f, 0.06f, 0.3290f },
};
static const float4x2 kRec2020 =
{
    { 0.708f, 0.170f, 0.131f, 0.3127f },
    { 0.292f, 0.797f, 0.046f, 0.3290f },
};
static const float4x2 kAP0 =
{
    { 0.7347f, 0.0f, 0.0001f, 0.32168f },
    { 0.2653f, 1.0f, -0.077f, 0.33767f },
};
static const float4x2 kAP1 =
{
    { 0.713f, 0.165f, 0.128f, 0.32168f },
    { 0.293f, 0.830f, 0.044f, 0.33767f },
};

const char* Colorspace_Str(Colorspace space)
{
    switch (space)
    {
    default:
        ASSERT(false);
        return "Unknown";
    case Colorspace_Rec709:
        return "Rec709";
    case Colorspace_Rec2020:
        return "Rec2020";
    case Colorspace_AP0:
        return "AP0";
    case Colorspace_AP1:
        return "AP1";
    }
}

float4x2 VEC_CALL Colorspace_GetPrimaries(Colorspace space)
{
    float4x2 value = kRec709;
    switch (space)
    {
    default:
        ASSERT(false);
        break;
    case Colorspace_Rec709:
        value = kRec709;
        break;
    case Colorspace_Rec2020:
        value = kRec2020;
        break;
    case Colorspace_AP0:
        value = kAP0;
        break;
    case Colorspace_AP1:
        value = kAP1;
        break;
    }
    return value;
}

float3x3 VEC_CALL Color_RGB_XYZ(float4x2 pr)
{
    // regenerate tristimulus z coordinate
    const float4 r = { pr.c0.x, pr.c1.x, 1.0f - (pr.c0.x + pr.c1.x) };
    const float4 g = { pr.c0.y, pr.c1.y, 1.0f - (pr.c0.y + pr.c1.y) };
    const float4 b = { pr.c0.z, pr.c1.z, 1.0f - (pr.c0.z + pr.c1.z) };
    const float4 w = { pr.c0.w, pr.c1.w, 1.0f - (pr.c0.w + pr.c1.w) };

    // invert tristimulus matrix
    const float3x3 xyz = f3x3_inverse((float3x3) { r, g, b });
    // normalize whitepoint to XYZ
    const float4 W = f4_divvs(w, w.y);
    // 
    const float4 scale = f3x3_mul_col(xyz, W);

    // scale tristimulus matrix by whitepoint scale
    const float3x3 XYZ =
    {
        f4_mulvs(r, scale.x),
        f4_mulvs(g, scale.y),
        f4_mulvs(b, scale.z),
    };

    return XYZ;
}

float3x3 VEC_CALL Color_XYZ_RGB(float4x2 pr)
{
    return f3x3_inverse(Color_RGB_XYZ(pr));
}

static void VEC_CALL Color_LogMatrix(FStream file, float3x3 m, const char* name)
{
    FStream_Printf(file, "%s:\n", name);
    FStream_Printf(file, "c0 = { %.8gf, %.8gf, %.8gf }\n", m.c0.x, m.c0.y, m.c0.z);
    FStream_Printf(file, "c1 = { %.8gf, %.8gf, %.8gf }\n", m.c1.x, m.c1.y, m.c1.z);
    FStream_Printf(file, "c2 = { %.8gf, %.8gf, %.8gf }\n", m.c2.x, m.c2.y, m.c2.z);
}

void Color_DumpConversionMatrices(void)
{
    FStream file = FStream_Open("ColorConversionMatrices.txt", "wb");
    if (FStream_IsOpen(file))
    {
        float3x3 Rec709_XYZ = Color_RGB_XYZ(kRec709);
        float3x3 XYZ_Rec709 = f3x3_inverse(Rec709_XYZ);
        Color_LogMatrix(file, Rec709_XYZ, "Rec709_XYZ");
        Color_LogMatrix(file, XYZ_Rec709, "XYZ_Rec709");
        FStream_Printf(file, "\n");

        float3x3 Rec2020_XYZ = Color_RGB_XYZ(kRec2020);
        float3x3 XYZ_Rec2020 = f3x3_inverse(Rec2020_XYZ);
        Color_LogMatrix(file, Rec2020_XYZ, "Rec2020_XYZ");
        Color_LogMatrix(file, XYZ_Rec2020, "XYZ_Rec2020");
        FStream_Printf(file, "\n");

        float3x3 AP0_XYZ = Color_RGB_XYZ(kAP0);
        float3x3 XYZ_AP0 = f3x3_inverse(AP0_XYZ);
        Color_LogMatrix(file, AP0_XYZ, "AP0_XYZ");
        Color_LogMatrix(file, XYZ_AP0, "XYZ_AP0");
        FStream_Printf(file, "\n");

        float3x3 AP1_XYZ = Color_RGB_XYZ(kAP1);
        float3x3 XYZ_AP1 = f3x3_inverse(AP1_XYZ);
        Color_LogMatrix(file, AP1_XYZ, "AP1_XYZ");
        Color_LogMatrix(file, XYZ_AP1, "XYZ_AP1");
        FStream_Printf(file, "\n");

        float3x3 Rec709_Rec2020 = f3x3_mul(XYZ_Rec2020, Rec709_XYZ);
        float3x3 Rec2020_Rec709 = f3x3_mul(XYZ_Rec709, Rec2020_XYZ);
        Color_LogMatrix(file, Rec709_Rec2020, "Rec709_Rec2020");
        Color_LogMatrix(file, Rec2020_Rec709, "Rec2020_Rec709");
        FStream_Printf(file, "\n");

        float3x3 Rec709_AP0 = f3x3_mul(XYZ_AP0, Rec709_XYZ);
        float3x3 AP0_Rec709 = f3x3_mul(XYZ_Rec709, AP0_XYZ);
        Color_LogMatrix(file, Rec709_AP0, "Rec709_AP0");
        Color_LogMatrix(file, AP0_Rec709, "AP0_Rec709");
        FStream_Printf(file, "\n");

        float3x3 Rec709_AP1 = f3x3_mul(XYZ_AP1, Rec709_XYZ);
        float3x3 AP1_Rec709 = f3x3_mul(XYZ_Rec709, AP1_XYZ);
        Color_LogMatrix(file, Rec709_AP1, "Rec709_AP1");
        Color_LogMatrix(file, AP1_Rec709, "AP1_Rec709");
        FStream_Printf(file, "\n");

        float3x3 Rec2020_AP0 = f3x3_mul(XYZ_AP0, Rec2020_XYZ);
        float3x3 AP0_Rec2020 = f3x3_mul(XYZ_Rec2020, AP0_XYZ);
        Color_LogMatrix(file, Rec2020_AP0, "Rec2020_AP0");
        Color_LogMatrix(file, AP0_Rec2020, "AP0_Rec2020");
        FStream_Printf(file, "\n");

        float3x3 Rec2020_AP1 = f3x3_mul(XYZ_AP1, Rec2020_XYZ);
        float3x3 AP1_Rec2020 = f3x3_mul(XYZ_Rec2020, AP1_XYZ);
        Color_LogMatrix(file, Rec2020_AP1, "Rec2020_AP1");
        Color_LogMatrix(file, AP1_Rec2020, "AP1_Rec2020");
        FStream_Printf(file, "\n");

        FStream_Close(&file);
    }
}
