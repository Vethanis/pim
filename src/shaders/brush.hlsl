
// ----------------------------------------------------------------------------

#define i32             int
#define kMinLightDist   0.01
#define kMinLightDistSq 0.001
#define kMinAlpha       0.00001525878
#define kPi             3.141592653
#define kTau            6.283185307
#define kEpsilon        2.38418579e-7

#define kGiDirections   5

float dotsat(float3 a, float3 b)
{
    return saturate(dot(a, b));
}

float BrdfAlpha(float roughness)
{
    return max(roughness * roughness, kMinAlpha);
}

float3 F_0(float3 albedo, float metallic)
{
    return lerp(0.04, albedo, metallic);
}

float F_90(float3 F0)
{
    return saturate(50.0f * dot(F0, 0.33));
}

float3 F_Schlick(float3 f0, float3 f90, float cosTheta)
{
    float t = 1.0 - cosTheta;
    float t5 = t * t * t * t * t;
    return lerp(f0, f90, t5);
}

float F_Schlick1(float f0, float f90, float cosTheta)
{
    float t = 1.0 - cosTheta;
    float t5 = t * t * t * t * t;
    return lerp(f0, f90, t5);
}

float3 F_SchlickEx(float3 albedo, float metallic, float cosTheta)
{
    float3 f0 = F_0(albedo, metallic);
    float f90 = F_90(f0);
    return F_Schlick(f0, f90, cosTheta);
}

float3 DiffuseColor(float3 albedo, float metallic)
{
    return albedo * (1.0 - metallic);
}

float D_GTR(float NoH, float alpha)
{
    float a2 = alpha * alpha;
    float f = lerp(1.0f, a2, NoH * NoH);
    return a2 / max(kEpsilon, f * f * kPi);
}

float G_SmithGGX(float NoL, float NoV, float alpha)
{
    float a2 = alpha * alpha;
    float v = NoL * sqrt(a2 + (NoV - NoV * a2) * NoV);
    float l = NoV * sqrt(a2 + (NoL - NoL * a2) * NoL);
    return 0.5f / max(kEpsilon, v + l);
}

float Fd_Lambert()
{
    return 1.0f / kPi;
}

float Fd_Burley(
    float NoL,
    float NoV,
    float LoH,
    float roughness)
{
    float fd90 = 0.5f + 2.0f * LoH * LoH * roughness;
    float lightScatter = F_Schlick1(1.0f, fd90, NoL);
    float viewScatter = F_Schlick1(1.0f, fd90, NoV);
    return (lightScatter * viewScatter) / kPi;
}

float3 DirectBRDF(
    float3 V,
    float3 L,
    float3 N,
    float3 albedo,
    float roughness,
    float metallic)
{
    float3 H = normalize(V + L);
    float NoV = dotsat(N, V);
    float NoH = dotsat(N, H);
    float NoL = dotsat(N, L);
    float HoV = dotsat(H, V);
    float LoH = dotsat(L, H);

    float alpha = BrdfAlpha(roughness);
    float3 F = F_SchlickEx(albedo, metallic, HoV);
    float G = G_SmithGGX(NoL, NoV, alpha);
    float D = D_GTR(NoH, alpha);
    float3 Fr = D * G * F;

    float3 Fd = DiffuseColor(albedo, metallic) * Fd_Burley(NoL, NoV, LoH, roughness);

    const float amtSpecular = 1.0;
    float amtDiffuse = 1.0 - metallic;
    float scale = 1.0 / (amtSpecular + amtDiffuse);

    return (Fr + Fd) * scale;
}

float3 EnvBRDF(
    float3 F,
    float NoV,
    float alpha)
{
    const float4 c0 = { -1.0f, -0.0275f, -0.572f, 0.022f };
    const float4 c1 = { 1.0f, 0.0425f, 1.04f, -0.04f };
    float4 r = c0 * alpha + c1;
    float a004 = min(r.x * r.x, exp2(-9.28f * NoV)) * r.x + r.y;
    float a = -1.04f * a004 + r.z;
    float b = 1.04f * a004 + r.w;
    return F * a + b;
}

float3 IndirectBRDF(
    float3 V,           // unit view vector pointing from surface to eye
    float3 N,           // unit normal vector pointing outward from surface
    float3 diffuseGI,   // low frequency scene irradiance
    float3 specularGI,  // high frequency scene irradiance
    float3 albedo,      // surface color
    float roughness,    // perceptual roughness
    float metallic,
    float occlusion)
{
    float alpha = BrdfAlpha(roughness);
    float NoV = dotsat(N, V);

    float3 F = F_SchlickEx(albedo, metallic, NoV);
    float3 Fr = EnvBRDF(F, NoV, alpha);
    Fr = Fr * specularGI;

    float3 Fd = DiffuseColor(albedo, metallic) * diffuseGI;

    const float amtSpecular = 1.0f;
    float amtDiffuse = 1.0f - metallic;
    float scale = 1.0f / (amtSpecular + amtDiffuse);

    return (Fr + Fd) * scale * occlusion;
}

// ----------------------------------------------------------------------------

float3 UnpackEmission(float3 albedo, float e)
{
    e = e * e * e * 100.0;
    return albedo * e;
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
float3 TonemapUncharted2(float3 x)
{
    const float a = 0.15;
    const float b = 0.50;
    const float c = 0.10;
    const float d = 0.20;
    const float e = 0.02;
    const float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 TonemapACES(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

float PerceptualLuminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// ----------------------------------------------------------------------------

float3x3 NormalToTBN(float3 N)
{
    const float3 kX = { 1.0, 0.0, 0.0 };
    const float3 kZ = { 0.0, 0.0, 1.0 };

    float3 a = abs(N.z) < 0.9 ? kZ : kX;
    float3 T = normalize(cross(a, N));
    float3 B = cross(N, T);

    return float3x3(T, B, N);
}

float3 TbnToWorld(float3x3 TBN, float3 nTS)
{
    float3 r = TBN[0] * nTS.x;
    float3 u = TBN[1] * nTS.y;
    float3 f = TBN[2] * nTS.z;
    float3 dir = r + u + f;
    return dir;
}

float3 TanToWorld(float3 normalWS, float3 normalTS)
{
    float3x3 TBN = NormalToTBN(normalWS);
    return TbnToWorld(TBN, normalTS);
}

// ----------------------------------------------------------------------------

float SG_BasisEval(float4 axis, float3 dir)
{
    float sharpness = axis.w;
    float cosTheta = dot(dir, axis.xyz);
    return exp(sharpness * (cosTheta - 1.0f));
}

float3 SG_Eval(float4 axis, float3 amplitude, float3 dir)
{
    return amplitude * SG_BasisEval(axis, dir);
}

float SG_BasisIntegral(float sharpness)
{
    float e = 1.0f - exp(sharpness * -2.0f);
    return kTau * (e / sharpness);
}

float3 SG_Integral(float4 axis, float3 amplitude)
{
    return amplitude * SG_BasisIntegral(axis.w);
}

float3 SG_Irradiance(float4 axis, float3 amplitude, float3 normal)
{
    float muDotN = dot(axis.xyz, normal);
    float lambda = axis.w;

    const float c0 = 0.36f;
    const float c1 = 1.0f / (4.0f * 0.36f);

    float eml = exp(-lambda);
    float eml2 = eml * eml;
    float rl = 1.0f / lambda;

    float scale = 1.0f + 2.0f * eml2 - rl;
    float bias = (eml - eml2) * rl - eml2;

    float x = sqrt(1.0f - scale);
    float x0 = c0 * muDotN;
    float x1 = c1 * x;

    float n = x0 + x1;

    float y = (abs(x0) <= x1) ? (n * n) / x : saturate(muDotN);

    float normalizedIrradiance = scale * y + bias;

    return SG_Integral(axis, amplitude) * normalizedIrradiance;
}

// ----------------------------------------------------------------------------

struct PerCamera
{
    float4x4 worldToClip;
    float4 eye;
    float4 giAxii[kGiDirections];
    uint lmBegin;
    float exposure;
};

[[vk::push_constant]]
cbuffer push_constants
{
    float4x4 localToWorld;
    float4 IMc0;
    float4 IMc1;
    float4 IMc2;
    float4 flatRome;
    uint kAlbedoIndex;
    uint kRomeIndex;
    uint kNormalIndex;
};

// "One-Set Design"
// https://gpuopen.com/wp-content/uploads/2016/03/VulkanFastPaths.pdf#page=10

// binding 1 set 0
[[vk::binding(1)]]
cbuffer cameraData
{
    PerCamera cameraData;
};

[[vk::binding(2)]]
SamplerState samplers[];
[[vk::binding(2)]]
Texture2D textures[];

struct VSInput
{
    float4 positionOS : POSITION;
    float4 normalOS : NORMAL;
    float4 uv01 : TEXCOORD0;
};

struct PSInput
{
    float4 positionCS : SV_Position;
    float3 positionWS : TEXCOORD0;
    float4 uv01 : TEXCOORD1;
    float3x3 TBN : TEXCOORD2;
    nointerpolation uint lmIndex : TEXCOORD3;
};

struct PSOutput
{
    float4 color : SV_Target0;
    half luminance : SV_Target1;
};

float4 SampleTexture(uint index, float2 uv)
{
    return textures[index].Sample(samplers[index], uv);
}

PSInput VSMain(VSInput input)
{
    float4 positionOS = float4(input.positionOS.xyz, 1.0);
    float4 positionWS = mul(localToWorld, positionOS);
    float4 positionCS = mul(cameraData.worldToClip, positionWS);
    float3x3 IM = float3x3(IMc0.xyz, IMc1.xyz, IMc2.xyz);
    float3 normalWS = mul(IM, input.normalOS.xyz);
    float3x3 TBN = NormalToTBN(normalWS);

    PSInput output;
    output.positionCS = positionCS;
    output.positionWS = positionWS.xyz;
    output.TBN = TBN;
    output.uv01 = input.uv01;
    output.lmIndex = (uint)input.normalOS.w;
    return output;
}

PSOutput PSMain(PSInput input)
{
    uint ai = kAlbedoIndex;
    uint ri = kRomeIndex;
    uint ni = kNormalIndex;
    float2 uv0 = input.uv01.xy;

    float3 albedo = SampleTexture(ai, uv0).xyz;
    albedo = max(kEpsilon, albedo);
    float4 rome = SampleTexture(ri, uv0) * flatRome;
    float3 normalTS = SampleTexture(ni, uv0).xyz;

    normalTS = normalize(normalTS * 2.0 - 1.0);
    float3 N = TbnToWorld(input.TBN, normalTS);
    N = normalize(N);

    float3 P = input.positionWS;
    float3 V = normalize(cameraData.eye.xyz - P);

    float roughness = rome.x;
    float occlusion = rome.y;
    float metallic = rome.z;
    float emission = rome.w;
    float3 light = 0.0;
    half luminance = 0.0;
    {
        float3 emissive = UnpackEmission(albedo, emission);
        light += emissive;
        luminance += PerceptualLuminance(emissive);
    }
    {
        //float3 L = cameraData[0].lightDir.xyz;
        //float3 lightColor = cameraData[0].lightColor.xyz;
        //float3 brdf = DirectBRDF(V, L, N, albedo, roughness, metallic);
        //float3 direct = brdf * lightColor * dotsat(N, L);
        //light += direct;
        //luminance += PerceptualLuminance(direct / albedo);
    }
    {
        float2 uv1 = input.uv01.zw;
        uint lmIndex = cameraData.lmBegin + input.lmIndex * kGiDirections;
        float3 R = reflect(-V, N);
        float3 diffuseGI = 0.0;
        float3 specularGI = 0.0;
        for (uint i = 0; i < kGiDirections; ++i)
        {
            float4 ax = cameraData.giAxii[i];
            ax.w = max(ax.w, kEpsilon);
            ax.xyz = TbnToWorld(input.TBN, ax.xyz);
            float3 probe = SampleTexture(lmIndex + i, uv1).xyz;
            diffuseGI += SG_Irradiance(ax, probe, N);
            specularGI += SG_Eval(ax, probe, R);
        }
        float3 indirect = IndirectBRDF(V, N, diffuseGI, specularGI, albedo, roughness, metallic, occlusion);
        light += indirect;
        luminance += PerceptualLuminance(indirect / albedo);
    }

    PSOutput output;
    output.luminance = luminance;
    light *= cameraData.exposure;
    output.color = float4(saturate(TonemapACES(light)), 1.0);
    return output;
}
