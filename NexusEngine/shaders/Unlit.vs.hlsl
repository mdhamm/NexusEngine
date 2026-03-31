cbuffer VSConstants
{
    float4x4 g_ViewProj;
};

struct VSInput
{
    float3 Pos            : ATTRIB0;
    float3 Normal         : ATTRIB1;
    float2 UV             : ATTRIB2;
    float4 InstanceWorld0 : ATTRIB3;
    float4 InstanceWorld1 : ATTRIB4;
    float4 InstanceWorld2 : ATTRIB5;
    float4 InstanceWorld3 : ATTRIB6;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    const float4x4 world = float4x4(
        VSIn.InstanceWorld0,
        VSIn.InstanceWorld1,
        VSIn.InstanceWorld2,
        VSIn.InstanceWorld3);
    const float4 worldPos = mul(float4(VSIn.Pos, 1.0), world);

    PSIn.Pos = mul(worldPos, g_ViewProj);
    PSIn.Color = VSIn.Normal * 0.5 + 0.5;
}
