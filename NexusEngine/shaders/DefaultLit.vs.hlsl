cbuffer VSConstants
{
    float4x4 g_ViewProj;
};

struct InstanceData
{
    float4x4 World;
};

StructuredBuffer<InstanceData> g_InstanceData;

struct VSInput
{
    float3 Pos    : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV     : ATTRIB2;
};

struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
    float3 WorldPos : WORLD_POS;
};

void main(in VSInput VSIn, in uint InstanceID : SV_InstanceID, out PSInput PSIn)
{
    float4x4 world = g_InstanceData[InstanceID].World;
    float4 worldPos = mul(float4(VSIn.Pos, 1.0), world);
    float3 worldNormal = mul(VSIn.Normal, (float3x3)world);

    PSIn.Pos = mul(worldPos, g_ViewProj);
    PSIn.Normal = worldNormal;
    PSIn.UV = VSIn.UV;
    PSIn.WorldPos = worldPos.xyz;
}
