#ifndef COLOR_HLSL
#define COLOR_HLSL

#define kEmissionScale 100.0f

float PerceptualLuminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float AverageLuminance(float3 color)
{
    return dot(color, (1.0 / 3.0));
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
float3 TonemapACES(float3 x, float wp)
{
    const float a = 2.43;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    float4 v = float4(x.rgb, wp);
    v = (v * (a * v + b)) / (v * (c * v + d) + e);
    v.rgb = v.rgb * (1.0 / v.a);
    return v.rgb;
}

static const float3x3 kBT709_To_XYZ =
float3x3(
    float3(506752.0 / 1228815.0, 87881.0 / 245763.0, 12673.0 / 70218.0),
    float3(87098.0 / 409605.0, 175762.0 / 245763.0, 12673.0 / 175545.0),
    float3(7918.0 / 409605.0, 87881.0 / 737289.0, 1001167.0 / 1053270.0));

static const float3x3 kXYZ_To_BT709 =
float3x3(
    float3(12831.0 / 3959.0, -329.0 / 214.0, -1974.0 / 3959.0),
    float3(-851781.0 / 878810.0, 1648619.0 / 878810.0, 36519.0 / 878810.0),
    float3(705.0 / 12673.0, -2585.0 / 12673.0, 705.0 / 667.0));

float3 Color_BT709_To_XYZ(float3 x)
{
    return mul(kBT709_To_XYZ, x);
}

float3 Color_XYZ_To_BT709(float3 x)
{
    return mul(kXYZ_To_BT709, x);
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
