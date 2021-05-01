#include "math/color.h"

#include "math/float3x3_funcs.h"
#include "common/stringutil.h"
#include "io/fstr.h"

// http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
static const float3x3 kBradford =
{
    { 0.8951000f, -0.7502000f, 0.0389000f },
    { 0.2664000f, 1.7135000f, -0.0685000f },
    { -0.1614000f, 0.0367000f, 1.0296000f },
};
static const float3x3 kBradfordInverse =
{
    { 0.9869929f, 0.4323053f, -0.0085287f },
    { -0.1470543f, 0.5183603f, 0.0400428f },
    { 0.1599627f, 0.0492912f, 0.9684867f },
};

static const float2 kD60 = { 0.32168f, 0.33767f };
static const float2 kD65 = { 0.3127f, 0.3290f };

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

// http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
float3x3 VEC_CALL Color_BradfordChromaticAdaptation(float2 wpxySrc, float2 wpxyDst)
{
    const float4 wpSrc = { wpxySrc.x, wpxySrc.y, 1.0f - (wpxySrc.x + wpxySrc.y) };
    const float4 wpDst = { wpxyDst.x, wpxyDst.y, 1.0f - (wpxyDst.x + wpxyDst.y) };
    const float4 scale = f4_div(wpDst, wpSrc);
    const float3x3 scaleMatrix =
    {
        { scale.x, 0.0f, 0.0f },
        { 0.0f, scale.y, 0.0f },
        { 0.0f, 0.0f, scale.z },
    };
    return f3x3_mul(kBradfordInverse, f3x3_mul(scaleMatrix, kBradford));
}

// http://www.brucelindbloom.com/index.html?Eqn_RGB_to_XYZ.html
float3x3 VEC_CALL Color_RGB_XYZ(float4x2 pr)
{
    // regenerate tristimulus z coordinate
    const float4 xyzr = { pr.c0.x, pr.c1.x, 1.0f - (pr.c0.x + pr.c1.x) };
    const float4 xyzg = { pr.c0.y, pr.c1.y, 1.0f - (pr.c0.y + pr.c1.y) };
    const float4 xyzb = { pr.c0.z, pr.c1.z, 1.0f - (pr.c0.z + pr.c1.z) };
    const float4 xyzw = { pr.c0.w, pr.c1.w, 1.0f - (pr.c0.w + pr.c1.w) };

    // XYZ = xyz / y
    const float4 XYZr = f4_divvs(xyzr, xyzr.y);
    const float4 XYZg = f4_divvs(xyzg, xyzg.y);
    const float4 XYZb = f4_divvs(xyzb, xyzb.y);
    const float4 XYZw = f4_divvs(xyzw, xyzw.y);

    const float3x3 XYZ = { XYZr, XYZg, XYZb };
    const float4 S = f3x3_mul_col(f3x3_inverse(XYZ), XYZw);

    const float3x3 M =
    {
        f4_mulvs(XYZr, S.x),
        f4_mulvs(XYZg, S.y),
        f4_mulvs(XYZb, S.z),
    };

    return M;
}

float3x3 VEC_CALL Color_XYZ_RGB(float4x2 pr)
{
    return f3x3_inverse(Color_RGB_XYZ(pr));
}

static void VEC_CALL Color_LogMatrix_C(FStream file, float3x3 m, const char* name)
{
    FStream_Printf(file, "pim_inline float4 VEC_CALL Color_%s(float4 c)\n", name);
    FStream_Puts(file, "{");
    FStream_Printf(file, "    const float4 c0 = { %012.8ff, %012.8ff, %012.8ff };\n", m.c0.x, m.c0.y, m.c0.z);
    FStream_Printf(file, "    const float4 c1 = { %012.8ff, %012.8ff, %012.8ff };\n", m.c1.x, m.c1.y, m.c1.z);
    FStream_Printf(file, "    const float4 c2 = { %012.8ff, %012.8ff, %012.8ff };\n", m.c2.x, m.c2.y, m.c2.z);
    FStream_Puts(file, "    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));");
    FStream_Puts(file, "};");
}
static void VEC_CALL Color_LogMatrix_HLSL(FStream file, float3x3 m, const char* name)
{
    FStream_Printf(file, "float3 Color_%s(float3 c)\n", name);
    FStream_Puts(file, "{");
    FStream_Printf(file, "    const float3 c0 = { %012.8f, %012.8f, %012.8f };\n", m.c0.x, m.c0.y, m.c0.z);
    FStream_Printf(file, "    const float3 c1 = { %012.8f, %012.8f, %012.8f };\n", m.c1.x, m.c1.y, m.c1.z);
    FStream_Printf(file, "    const float3 c2 = { %012.8f, %012.8f, %012.8f };\n", m.c2.x, m.c2.y, m.c2.z);
    FStream_Puts(file, "    return c0 * c.x + c1 * c.y + c2 * c.z;");
    FStream_Puts(file, "};");
}

void Color_DumpConversionMatrices(void)
{
    const float3x3 D60_D65 = Color_BradfordChromaticAdaptation(kD60, kD65);
    const float3x3 D65_D60 = Color_BradfordChromaticAdaptation(kD65, kD60);

    const float3x3 Rec709_XYZ = Color_RGB_XYZ(kRec709);
    const float3x3 XYZ_Rec709 = f3x3_inverse(Rec709_XYZ);

    const float3x3 Rec2020_XYZ = Color_RGB_XYZ(kRec2020);
    const float3x3 XYZ_Rec2020 = f3x3_inverse(Rec2020_XYZ);

    const float3x3 AP0_XYZ = Color_RGB_XYZ(kAP0);
    const float3x3 XYZ_AP0 = f3x3_inverse(AP0_XYZ);

    const float3x3 AP1_XYZ = Color_RGB_XYZ(kAP1);
    const float3x3 XYZ_AP1 = f3x3_inverse(AP1_XYZ);

    const float3x3 Rec709_XYZ_D60 = f3x3_mul(D65_D60, Rec709_XYZ);
    const float3x3 Rec2020_XYZ_D60 = f3x3_mul(D65_D60, Rec2020_XYZ);

    const float3x3 AP0_XYZ_D65 = f3x3_mul(D60_D65, AP0_XYZ);
    const float3x3 AP1_XYZ_D65 = f3x3_mul(D60_D65, AP1_XYZ);

    const float3x3 Rec709_Rec2020 = f3x3_mul(XYZ_Rec2020, Rec709_XYZ);
    const float3x3 Rec2020_Rec709 = f3x3_mul(XYZ_Rec709, Rec2020_XYZ);

    const float3x3 Rec709_AP0 = f3x3_mul(XYZ_AP0, Rec709_XYZ_D60);
    const float3x3 AP0_Rec709 = f3x3_mul(XYZ_Rec709, AP0_XYZ_D65);

    const float3x3 Rec709_AP1 = f3x3_mul(XYZ_AP1, Rec709_XYZ_D60);
    const float3x3 AP1_Rec709 = f3x3_mul(XYZ_Rec709, AP1_XYZ_D65);

    const float3x3 Rec2020_AP0 = f3x3_mul(XYZ_AP0, Rec2020_XYZ_D60);
    const float3x3 AP0_Rec2020 = f3x3_mul(XYZ_Rec2020, AP0_XYZ_D65);

    const float3x3 Rec2020_AP1 = f3x3_mul(XYZ_AP1, Rec2020_XYZ_D60);
    const float3x3 AP1_Rec2020 = f3x3_mul(XYZ_Rec2020, AP1_XYZ_D65);

#define MATRICES(fn) \
    fn(D60_D65), \
    fn(D65_D60), \
    fn(Rec709_XYZ), \
    fn(XYZ_Rec709), \
    fn(Rec2020_XYZ), \
    fn(XYZ_Rec2020), \
    fn(AP0_XYZ), \
    fn(XYZ_AP0), \
    fn(AP1_XYZ), \
    fn(XYZ_AP1), \
    fn(Rec709_XYZ_D60), \
    fn(Rec2020_XYZ_D60), \
    fn(AP0_XYZ_D65), \
    fn(AP1_XYZ_D65), \
    fn(Rec709_Rec2020), \
    fn(Rec2020_Rec709), \
    fn(Rec709_AP0), \
    fn(AP0_Rec709), \
    fn(Rec709_AP1), \
    fn(AP1_Rec709), \
    fn(Rec2020_AP0), \
    fn(AP0_Rec2020), \
    fn(Rec2020_AP1), \
    fn(AP1_Rec2020),

    const float3x3 matrices[] =
    {
#define MATRICES_FN(x) x
        MATRICES(MATRICES_FN)
#undef MATRICES_FN
    };
    const char* names[] =
    {
#define MATRICES_FN(x) #x
        MATRICES(MATRICES_FN)
#undef MATRICES_FN
    };

#undef MATRICES

    FStream file = FStream_Open("src/math/color_gen.h", "wb");
    if (FStream_IsOpen(file))
    {
        FStream_Puts(file, "#pragma once");
        FStream_Puts(file, "// Autogenerated, do not edit.");
        FStream_Puts(file, "#include \"math/float4_funcs.h\"");
        FStream_Puts(file, "");
        FStream_Puts(file, "PIM_C_BEGIN");
        FStream_Puts(file, "");
        for (i32 iMatrix = 0; iMatrix < NELEM(matrices); ++iMatrix)
        {
            Color_LogMatrix_C(file, matrices[iMatrix], names[iMatrix]);
        }
        FStream_Puts(file, "");
        FStream_Puts(file, "PIM_C_END");
        FStream_Puts(file, "");
        FStream_Close(&file);
    }
    file = FStream_Open("src/shaders/color_gen.h", "wb");
    if (FStream_IsOpen(file))
    {
        FStream_Puts(file, "#ifndef COLOR_GEN_H");
        FStream_Puts(file, "#define COLOR_GEN_H");
        FStream_Puts(file, "// Autogenerated, do not edit.");
        FStream_Puts(file, "");
        for (i32 iMatrix = 0; iMatrix < NELEM(matrices); ++iMatrix)
        {
            Color_LogMatrix_HLSL(file, matrices[iMatrix], names[iMatrix]);
        }
        FStream_Puts(file, "");
        FStream_Puts(file, "#endif // COLOR_GEN_H");
        FStream_Puts(file, "");
        FStream_Close(&file);
    }
}
