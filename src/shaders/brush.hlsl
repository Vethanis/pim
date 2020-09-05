
struct PerDraw
{
    float4x4 localToWorld;
    float4x4 worldToLocal;
    float4 textureScale;
    float4 textureBias;
};

struct PerCamera
{
    float4x4 worldToCamera;
    float4x4 cameraToClip;
};

// "One-Set Design"
// https://gpuopen.com/wp-content/uploads/2016/03/VulkanFastPaths.pdf#page=10

// binding 0 set 0
[[vk::binding(0)]]
StructuredBuffer<PerDraw> drawData;

// binding 1 set 0
[[vk::binding(1)]]
StructuredBuffer<PerCamera> cameraData;

[[vk::binding(2)]]
SamplerState samplers[];
[[vk::binding(2)]]
Texture2D textures[];

[[vk::push_constant]]
cbuffer push_constants
{
    uint kDrawIndex;
    uint kCameraIndex;
};

struct VSInput
{
    float4 positionOS : POSITION;
    float4 normalOS : NORMAL;
    float4 uv01 : TEXCOORD0;
};

struct PSInput
{
    float4 positionCS : SV_Position;
    float4 positionWS : TEXCOORD0;
    float4 normalWS : TEXCOORD1;
    float4 uv01 : TEXCOORD2;
};

struct PSOutput
{
    float4 color : SV_Target;
};

PSInput VSMain(VSInput input)
{
    uint di = kDrawIndex;
    uint ci = kCameraIndex;
    PerDraw perDraw = drawData[di];
    PerCamera perCamera = cameraData[ci];
    float4 positionWS = mul(perDraw.localToWorld, input.positionOS);
    float4 positionCS = mul(perCamera.cameraToClip, mul(perCamera.worldToCamera, positionWS));
    float4 normalWS = mul(perDraw.worldToLocal, input.normalOS);

    PSInput output;
    output.positionCS = positionCS;
    output.positionWS = positionWS;
    output.normalWS = normalWS;
    output.uv01 = input.uv01 * perDraw.textureScale + perDraw.textureBias;
    return output;
}

PSOutput PSMain(PSInput input)
{
    uint di = kDrawIndex;
    float4 albedo = textures[di].Sample(samplers[di], input.uv01.xy);
    PSOutput output;
    output.color = albedo;
    output.color.w = 1.0;
    return output;
}
