#include "math/color.h"

#include "math/float3x3_funcs.h"
#include "common/stringutil.h"
#include "io/fstr.h"

static const float4 Rec709x = { 0.64f, 0.3f, 0.15f, 0.3127f };
static const float4 Rec709y = { 0.33f, 0.6f, 0.06f, 0.3290f };

static const float4 Rec2020x = { 0.708f, 0.170f, 0.131f, 0.3127f };
static const float4 Rec2020y = { 0.292f, 0.797f, 0.046f, 0.3290f };

static const float4 AP0x = { 0.7347f, 0.0f, 0.0001f, 0.32168f };
static const float4 AP0y = { 0.2653f, 1.0f, -0.077f, 0.33767f };

static const float4 AP1x = { 0.713f, 0.165f, 0.128f, 0.32168f };
static const float4 AP1y = { 0.293f, 0.830f, 0.044f, 0.33767f };

float3x3 VEC_CALL Color_RGB_XYZ(float4 x, float4 y)
{
    // regenerate tristimulus z coordinate
    const float4 r = { x.x, y.x, 1.0f - (x.x + y.x) };
    const float4 g = { x.y, y.y, 1.0f - (x.y + y.y) };
    const float4 b = { x.z, y.z, 1.0f - (x.z + y.z) };
    const float4 w = { x.w, y.w, 1.0f - (x.w + y.w) };

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

float3x3 VEC_CALL Color_XYZ_RGB(float4 x, float4 y)
{
    return f3x3_inverse(Color_RGB_XYZ(x, y));
}

static void VEC_CALL Color_LogMatrix(FStream file, float3x3 m, const char* name)
{
    FStream_Printf(file, "float3x3 %s =\n", name);
    FStream_Printf(file, "{\n");
    FStream_Printf(file, "    { %.8gf, %.8gf, %.8gf },\n", m.c0.x, m.c0.y, m.c0.z);
    FStream_Printf(file, "    { %.8gf, %.8gf, %.8gf },\n", m.c1.x, m.c1.y, m.c1.z);
    FStream_Printf(file, "    { %.8gf, %.8gf, %.8gf },\n", m.c2.x, m.c2.y, m.c2.z);
    FStream_Printf(file, "};\n");
}

void Color_DumpConversionMatrices(void)
{
    FStream file = FStream_Open("ColorConversionMatrices.txt", "wb");
    if (FStream_IsOpen(file))
    {
        float3x3 Rec709_XYZ = Color_RGB_XYZ(Rec709x, Rec709y);
        float3x3 XYZ_Rec709 = f3x3_inverse(Rec709_XYZ);
        Color_LogMatrix(file, Rec709_XYZ, "Rec709_XYZ");
        Color_LogMatrix(file, XYZ_Rec709, "XYZ_Rec709");
        FStream_Printf(file, "\n");

        float3x3 Rec2020_XYZ = Color_RGB_XYZ(Rec2020x, Rec2020y);
        float3x3 XYZ_Rec2020 = f3x3_inverse(Rec2020_XYZ);
        Color_LogMatrix(file, Rec2020_XYZ, "Rec2020_XYZ");
        Color_LogMatrix(file, XYZ_Rec2020, "XYZ_Rec2020");
        FStream_Printf(file, "\n");

        float3x3 AP0_XYZ = Color_RGB_XYZ(AP0x, AP0y);
        float3x3 XYZ_AP0 = f3x3_inverse(AP0_XYZ);
        Color_LogMatrix(file, AP0_XYZ, "AP0_XYZ");
        Color_LogMatrix(file, XYZ_AP0, "XYZ_AP0");
        FStream_Printf(file, "\n");

        float3x3 AP1_XYZ = Color_RGB_XYZ(AP1x, AP1y);
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

        FStream_Close(&file);
    }
}
