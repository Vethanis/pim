#pragma once
// Autogenerated, do not edit.
#include "math/float4_funcs.h"

PIM_C_BEGIN

pim_inline float4 VEC_CALL Color_D60_D65(float4 c)
{
    const float4 c0 = { 000.97282726f, -00.00071821f, 000.00293641f };
    const float4 c1 = { -00.00143811f, 000.97380459f, -00.00513566f };
    const float4 c2 = { 000.01311909f, 000.00408883f, 001.05158913f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_D65_D60(float4 c)
{
    const float4 c0 = { 001.02797151f, 000.00077021f, -00.00286672f };
    const float4 c1 = { 000.00145030f, 001.02688038f, 000.00501085f };
    const float4 c2 = { -00.01282999f, -00.00400242f, 000.95095807f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec709_XYZ(float4 c)
{
    const float4 c0 = { 000.41239077f, 000.21263900f, 000.01933080f };
    const float4 c1 = { 000.35758433f, 000.71516865f, 000.11919472f };
    const float4 c2 = { 000.18048084f, 000.07219233f, 000.95053232f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_XYZ_Rec709(float4 c)
{
    const float4 c0 = { 003.24097013f, -00.96924371f, 000.05563008f };
    const float4 c1 = { -01.53738332f, 001.87596750f, -00.20397684f };
    const float4 c2 = { -00.49861082f, 000.04155510f, 001.05697119f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec2020_XYZ(float4 c)
{
    const float4 c0 = { 000.63695812f, 000.26270023f, 000.00000000f };
    const float4 c1 = { 000.14461692f, 000.67799807f, 000.02807269f };
    const float4 c2 = { 000.16888094f, 000.05930171f, 001.06098485f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_XYZ_Rec2020(float4 c)
{
    const float4 c0 = { 001.71665096f, -00.66668433f, 000.01763985f };
    const float4 c1 = { -00.35567081f, 001.61648130f, -00.04277061f };
    const float4 c2 = { -00.25336623f, 000.01576854f, 000.94210327f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_AP0_XYZ(float4 c)
{
    const float4 c0 = { 000.95255244f, 000.34396645f, 000.00000000f };
    const float4 c1 = { 000.00000000f, 000.72816604f, 000.00000000f };
    const float4 c2 = { 000.00009368f, -00.07213254f, 001.00882506f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_XYZ_AP0(float4 c)
{
    const float4 c0 = { 001.04981089f, -00.49590302f, 000.00000000f };
    const float4 c1 = { -00.00000000f, 001.37331307f, -00.00000000f };
    const float4 c2 = { -00.00009748f, 000.09824004f, 000.99125206f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_AP1_XYZ(float4 c)
{
    const float4 c0 = { 000.66245425f, 000.27222878f, -00.00557469f };
    const float4 c1 = { 000.13400421f, 000.67408168f, 000.00406073f };
    const float4 c2 = { 000.15618765f, 000.05368950f, 001.01033890f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_XYZ_AP1(float4 c)
{
    const float4 c0 = { 001.64102328f, -00.66366309f, 000.01172196f };
    const float4 c1 = { -00.32480329f, 001.61533189f, -00.00828445f };
    const float4 c2 = { -00.23642468f, 000.01675636f, 000.98839509f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec709_XYZ_D60(float4 c)
{
    const float4 c0 = { 000.42398635f, 000.21859509f, 000.01826607f };
    const float4 c1 = { 000.36709443f, 000.73419100f, 000.11590769f };
    const float4 c2 = { 000.17343853f, 000.07046746f, 000.90376073f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec2020_XYZ_D60(float4 c)
{
    const float4 c0 = { 000.65515578f, 000.27025232f, -00.00050963f };
    const float4 c1 = { 000.14928520f, 000.69622195f, 000.02967872f };
    const float4 c2 = { 000.16007836f, 000.05677932f, 001.00876522f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_AP0_XYZ_D65(float4 c)
{
    const float4 c0 = { 000.92617434f, 000.33427197f, 000.00103058f };
    const float4 c1 = { -00.00104718f, 000.70909142f, -00.00373961f };
    const float4 c2 = { 000.01342973f, -00.06611815f, 001.06124020f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_AP1_XYZ_D65(float4 c)
{
    const float4 c0 = { 000.64398891f, 000.26459908f, -00.00531512f };
    const float4 c1 = { 000.12944680f, 000.65634423f, 000.00120185f };
    const float4 c2 = { 000.16512112f, 000.05630201f, 001.06264424f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec709_Rec2020(float4 c)
{
    const float4 c0 = { 000.62740374f, 000.06909731f, 000.01639142f };
    const float4 c1 = { 000.32928297f, 000.91954046f, 000.08801328f };
    const float4 c2 = { 000.04331309f, 000.01136230f, 000.89559555f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec2020_Rec709(float4 c)
{
    const float4 c0 = { 001.66049135f, -00.12455052f, -00.01815073f };
    const float4 c1 = { -00.58764112f, 001.13289988f, -00.10057884f };
    const float4 c2 = { -00.07284993f, -00.00834937f, 001.11872911f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec709_AP0(float4 c)
{
    const float4 c0 = { 000.44510370f, 000.09173784f, 000.01810628f };
    const float4 c1 = { 000.38536844f, 000.83761758f, 000.11489374f };
    const float4 c2 = { 000.18198957f, 000.09955067f, 000.89585471f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_AP0_Rec709(float4 c)
{
    const float4 c0 = { 002.48728561f, -00.27056241f, -00.01557129f };
    const float4 c1 = { -01.09167457f, 001.33109200f, -00.14864914f };
    const float4 c2 = { -00.38397157f, -00.09295224f, 001.13593400f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec709_AP1(float4 c)
{
    const float4 c0 = { 000.62045252f, 000.07202560f, 000.02121311f };
    const float4 c1 = { 000.33653942f, 000.94427723f, 000.11278329f };
    const float4 c2 = { 000.04805727f, 000.01386732f, 000.89472198f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_AP1_Rec709(float4 c)
{
    const float4 c0 = { 001.68300891f, -00.12802380f, -00.02376486f };
    const float4 c1 = { -00.59011865f, 001.10586488f, -00.12540756f };
    const float4 c2 = { -00.08125103f, -00.01026359f, 001.12088573f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec2020_AP0(float4 c)
{
    const float4 c0 = { 000.68778974f, 000.04619727f, -00.00050517f };
    const float4 c1 = { 000.15671833f, 000.88501531f, 000.02941909f };
    const float4 c2 = { 000.16795367f, 000.09769358f, 000.99994063f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_AP0_Rec2020(float4 c)
{
    const float4 c0 = { 001.47076619f, -00.07710528f, 000.00301148f };
    const float4 c1 = { -00.25305325f, 001.14687216f, -00.03386985f };
    const float4 c2 = { -00.22231197f, -00.09909794f, 001.00286269f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_Rec2020_AP1(float4 c)
{
    const float4 c0 = { 000.98746759f, 000.00173593f, 000.00493710f };
    const float4 c1 = { 000.01182853f, 001.02605176f, 000.02531640f };
    const float4 c2 = { 000.00575322f, 000.00238259f, 000.99846458f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};
pim_inline float4 VEC_CALL Color_AP1_Rec2020(float4 c)
{
    const float4 c0 = { 001.01274061f, -00.00170165f, -00.00496459f };
    const float4 c1 = { -00.01153201f, 000.97468698f, -00.02465655f };
    const float4 c2 = { -00.00580782f, -00.00231617f, 001.00162518f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
};

PIM_C_END

