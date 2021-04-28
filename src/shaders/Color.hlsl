#ifndef COLOR_HLSL
#define COLOR_HLSL

#include "../rendering/r_config.h"

float PerceptualLuminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float AverageLuminance(float3 color)
{
    return dot(color, (1.0 / 3.0));
}

float MaxLuminance(float3 color)
{
    return max(color.r, max(color.g, color.b));
}

float3 UnpackEmission(float3 albedo, float e)
{
    e = e * e * kEmissionScale;
    return albedo * e;
}

float3 Xy16ToNormalTs(float2 xy)
{
    float3 n;
    n.x = xy.x;
    n.y = xy.y;
    n.z = sqrt(max(0.0f, 1.0f - (n.x * n.x + n.y * n.y)));
    return n;
}

float3 SetLum(float3 x, float oldLum, float newLum)
{
    return x * (newLum / max(1e-7f, oldLum));
}

float3 TonemapReinhard(float3 x, float wp)
{
    float l0 = max(1e-7, AverageLuminance(x));
    float n = l0 * (1.0f + (l0 / (wp * wp)));
    float l1 = n / (1.0f + l0);
    return SetLum(x, l0, l1);
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
float3 TonemapUncharted2(float3 x, float wp)
{
    const float a = 0.15;
    const float b = 0.50;
    const float c = 0.10;
    const float d = 0.20;
    const float e = 0.02;
    const float f = 0.30;
    float4 v = float4(x.rgb, wp);
    v = ((v * (a * v + c * b) + d * e) / (v * (a * v + b) + d * f)) - e / f;
    v.rgb = v.rgb * (1.0 / v.a);
    return v.rgb;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 TonemapACES(float3 x)
{
    const float a = 2.43;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    x = (x * (a * x + b)) / (x * (c * x + d) + e);
    return x;
}

float3 Color_Rec709_XYZ(float3 c)
{
    const float3 c0 = { 0.41239089f, 0.21263906f, 0.019330805f };
    const float3 c1 = { 0.35758442f, 0.71516883f, 0.11919476f };
    const float3 c2 = { 0.18048081f, 0.072192319f, 0.9505322f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_XYZ_Rec709(float3 c)
{
    const float3 c0 = { 3.2409692f, -0.96924347f, 0.055630092f };
    const float3 c1 = { -1.5373828f, 1.8759671f, -0.20397688f };
    const float3 c2 = { -0.49861068f, 0.041555069f, 1.0569714f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_Rec2020_XYZ(float3 c)
{
    const float3 c0 = { 0.63695818f, 0.26270026f, 0.0f };
    const float3 c1 = { 0.14461692f, 0.67799813f, 0.028072689f };
    const float3 c2 = { 0.16888095f, 0.059301712f, 1.060985f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_XYZ_Rec2020(float3 c)
{
    const float3 c0 = { 1.7166507f, -0.66668427f, 0.017639853f };
    const float3 c1 = { -0.35567072f, 1.6164811f, -0.042770606f };
    const float3 c2 = { -0.25336623f, 0.015768535f, 0.94210321f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_AP0_XYZ(float3 c)
{
    const float3 c0 = { 0.95255238f, 0.34396645f, 0.0f };
    const float3 c1 = { 0.0f, 0.72816604f, 0.0f };
    const float3 c2 = { 9.3678616e-05f, -0.072132535f, 1.0088251f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_XYZ_AP0(float3 c)
{
    const float3 c0 = { 1.049811f, -0.49590304f, 0.0f };
    const float3 c1 = { -0.0f, 1.3733131f, -0.0f };
    const float3 c2 = { -9.7484539e-05f, 0.098240048f, 0.99125206f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_AP1_XYZ(float3 c)
{
    const float3 c0 = { 0.66245425f, 0.27222878f, -0.0055746892f };
    const float3 c1 = { 0.13400419f, 0.67408162f, 0.0040607289f };
    const float3 c2 = { 0.15618764f, 0.053689498f, 1.0103388f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_XYZ_AP1(float3 c)
{
    const float3 c0 = { 1.6410233f, -0.66366309f, 0.011721959f };
    const float3 c1 = { -0.32480329f, 1.6153321f, -0.0082844514f };
    const float3 c2 = { -0.23642468f, 0.016756363f, 0.98839515f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_AP0_AP1(float3 c)
{
    const float3 c0 = { 1.4514393161f, -0.0765537734f, 0.0083161484f };
    const float3 c1 = { -0.2365107469f, 1.1762296998f, -0.0060324498f };
    const float3 c2 = { -0.2149285693f, -0.0996759264f, 0.9977163014f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_AP1_AP0(float3 c)
{
    const float3 c0 = { 0.6954522414f, 0.0447945634f, -0.0055258826f };
    const float3 c1 = { 0.1406786965f, 0.8596711185f, 0.0040252103f };
    const float3 c2 = { 0.1638690622f, 0.0955343182f, 1.0015006723f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_Rec709_Rec2020(float3 c)
{
    const float3 c0 = { 0.62740386f, 0.06909731f, 0.016391428f };
    const float3 c1 = { 0.32928303f, 0.91954052f, 0.088013299f };
    const float3 c2 = { 0.043313056f, 0.011362299f, 0.89559537f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_Rec2020_Rec709(float3 c)
{
    const float3 c0 = { 1.660491f, -0.12455052f, -0.018150739f };
    const float3 c1 = { -0.587641f, 1.1328998f, -0.10057887f };
    const float3 c2 = { -0.072849929f, -0.0083494037f, 1.1187295f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_Rec709_AP0(float3 c)
{
    const float3 c0 = { 0.43293062f, 0.089413166f, 0.019161701f };
    const float3 c1 = { 0.37538442f, 0.81653321f, 0.11815205f };
    const float3 c2 = { 0.18937808f, 0.10302201f, 0.94221699f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_AP0_Rec709(float3 c)
{
    const float3 c0 = { 2.5583849f, -0.27798539f, -0.017170634f };
    const float3 c1 = { -1.11947f, 1.3660156f, -0.14852904f };
    const float3 c2 = { -0.391812f, -0.09348727f, 1.081018f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_Rec709_AP1(float3 c)
{
    const float3 c0 = { 0.60310686f, 0.07011801f, 0.022178905f };
    const float3 c1 = { 0.32633454f, 0.91991681f, 0.11607833f };
    const float3 c2 = { 0.047995642f, 0.012763575f, 0.94101894f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_AP1_Rec709(float3 c)
{
    const float3 c0 = { 1.7312536f, -0.13161892f, -0.024568275f };
    const float3 c1 = { -0.60404283f, 1.1348411f, -0.12575033f };
    const float3 c2 = { -0.080107749f, -0.0086794198f, 1.0656365f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_Rec2020_AP0(float3 c)
{
    const float3 c0 = { 0.66868573f, 0.044900179f, 0.0f };
    const float3 c1 = { 0.15181769f, 0.8621456f, 0.02782711f };
    const float3 c2 = { 0.17718965f, 0.10192245f, 1.0517036f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_AP0_Rec2020(float3 c)
{
    const float3 c0 = { 1.512861f, -0.079036415f, 0.0020912308f };
    const float3 c1 = { -0.25898734f, 1.1770666f, -0.031144103f };
    const float3 c2 = { -0.22978596f, -0.10075563f, 0.95350415f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_Rec2020_AP1(float3 c)
{
    const float3 c0 = { 0.95993727f, 0.0016225278f, 0.0052900701f };
    const float3 c1 = { 0.01046662f, 0.9996857f, 0.023825262f };
    const float3 c2 = { 0.0070331395f, 0.0014901534f, 1.0501608f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}
float3 Color_AP1_Rec2020(float3 c)
{
    const float3 c0 = { 1.0417912f, -0.0016830862f, -0.0052097263f };
    const float3 c1 = { -0.010741562f, 1.0003656f, -0.022641439f };
    const float3 c2 = { -0.0069618821f, -0.0014082193f, 0.95230216f };
    return c0 * c.x + c1 * c.y + c2 * c.z;
}

float3 Color_SDRToScene(float3 x)
{
#if COLOR_SCENE_REC709
    return x;
#elif COLOR_SCENE_REC2020
    return Color_Rec709_Rec2020(x);
#elif COLOR_SCENE_AP1
    return Color_Rec709_AP1(x);
#elif COLOR_SCENE_AP0
    return Color_Rec709_AP0(x);
#else
#   error Unrecognized scene colorspace
#endif // COLOR_SCENE_X
}
float3 Color_HDRToScene(float3 x)
{
#if COLOR_SCENE_REC709
    return Color_Rec2020_Rec709(x);
#elif COLOR_SCENE_REC2020
    return x;
#elif COLOR_SCENE_AP1
    return Color_Rec2020_AP1(x);
#elif COLOR_SCENE_AP0
    return Color_Rec2020_AP0(x);
#else
#   error Unrecognized scene colorspace
#endif // COLOR_SCENE_X
}
float3 Color_SceneToSDR(float3 x)
{
#if COLOR_SCENE_REC709
    return x;
#elif COLOR_SCENE_REC2020
    return Color_Rec2020_Rec709(x);
#elif COLOR_SCENE_AP1
    return Color_AP1_Rec709(x);
#elif COLOR_SCENE_AP0
    return Color_AP0_Rec709(x);
#else
#   error Unrecognized scene colorspace
#endif // COLOR_SCENE_X
}
float3 Color_SceneToHDR(float3 x)
{
#if COLOR_SCENE_REC709
    return Color_Rec709_Rec2020(x);
#elif COLOR_SCENE_REC2020
    return x;
#elif COLOR_SCENE_AP1
    return Color_AP1_Rec2020(x);
#elif COLOR_SCENE_AP0
    return Color_AP0_Rec2020(x);
#else
#   error Unrecognized scene colorspace
#endif // COLOR_SCENE_X
}

// https://en.wikipedia.org/wiki/Transfer_functions_in_imaging
// OETF: Scene Luminance to Signal; (eg. Camera)
// EOTF: Signal to Display Luminance; (eg. Monitor)
// OOTF: Scene Luminance to Display Luminance; OETF(EOTF(x));

// Rec2100: https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2100-2-201807-I!!PDF-E.pdf

// E: Scene linear light
// Ep: Nonlinear color value
float3 GRec709(float3 E)
{
    float3 a = 267.84 * E;
    float3 b = 1.099 * pow(59.5208 * E, 0.45) - 0.099;
    float3 Ep = lerp(a, b, step(0.0003024, E));
    return Ep;
}

// Ep: Nonlinear color value
// Fd: Display linear light
float3 GRec1886(float3 Ep)
{
    float3 Fd = 100.0f * pow(Ep, 2.4);
    return Fd;
}

// E: Scene linear light
// Fd: Display linear light
float3 PQ_OOTF(float3 E)
{
    float3 Fd = GRec1886(GRec709(E));
    return Fd;
}

// Ep: Nonlinear Signal in [0, 1]
// Fd: Display Luminance in [0, 10000] cd/m^2
float3 PQ_EOTF(float3 Ep)
{
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    const float m1 = 0.15930175781;
    const float m2 = 78.84375;
    float3 t = pow(Ep, 1.0 / m2);
    float3 Y = pow(max(t - c1, 0.0) / (c2 - c3 * t), 1.0 / m1);
    float3 Fd = 10000.0 * Y;
    return Fd;
}

// Fd: Display Luminance in [0, 10000] cd/m^2
// Ep: Signal in [0, 1]
float3 PQ_InverseEOTF(float3 Fd)
{
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    const float m1 = 0.15930175781;
    const float m2 = 78.84375;
    float3 Y = Fd * (1.0 / 10000.0);
    float3 y = pow(Y, m1);
    float3 Ep = pow((c1 + c2 * y) / (1.0 + c3 * y), m2);
    return Ep;
}

// E: Scene Luminance
// Ep: Signal in [0, 1]
float3 PQ_OETF(float3 E)
{
    return PQ_InverseEOTF(PQ_OOTF(E));
}

// L: Display Luminance in [0, 1]
// V: Signal in [0, 1]
// https://nick-shaw.github.io/cinematiccolor/common-rgb-color-spaces.html
float HLG_OETF(float L)
{
    const float a = 0.17883277;
    const float b = 1.0 - 4.0 * a;
    const float c = 0.5 - a * log(4.0 * a);
    const float t = 1.0 / 12.0;
    float3 V = (L <= t) ? sqrt(3.0 * L) : (a * log(12.0 * L - b) + c);
    return V;
}

// L: Display Luminance in [0, 1]
// V: Signal in [0, 1]
// https://nick-shaw.github.io/cinematiccolor/common-rgb-color-spaces.html
float3 HLG_OETF(float3 L)
{
    return float3(HLG_OETF(L.x), HLG_OETF(L.y), HLG_OETF(L.z));
}

// S: Scene Luminance
// P: Peak Display Luminance
// D: Display Luminance
// https://nick-shaw.github.io/cinematiccolor/common-rgb-color-spaces.html
float3 HLG_OOTF(float3 S, float P)
{
    float Ys = dot(S, float3(0.2627, 0.6780, 0.0593));
    float g = 1.2 + 0.42 * log10(P / 1000.0);
    float3 D = P * pow(Ys, g - 1.0) * S;
    return D;
}

#endif // COLOR_HLSL
