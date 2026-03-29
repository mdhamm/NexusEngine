cbuffer VSConstants
{
    float4x4 g_WorldViewProj;
    float4x4 g_World;
};

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

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_WorldViewProj);
    PSIn.Normal = mul(float4(VSIn.Normal, 0.0), g_World).xyz;
    PSIn.UV = VSIn.UV;
    PSIn.WorldPos = mul(float4(VSIn.Pos, 1.0), g_World).xyz;
}
