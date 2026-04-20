cbuffer VSConstants
{
    float4 g_ViewProjCol0;
    float4 g_ViewProjCol1;
    float4 g_ViewProjCol2;
    float4 g_ViewProjCol3;
};

struct InstanceData
{
    float4 Col0;
    float4 Col1;
    float4 Col2;
    float4 Col3;
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
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in VSInput VSIn, in uint InstanceID : SV_InstanceID, out PSInput PSIn)
{
    InstanceData inst = g_InstanceData[InstanceID];
    float4x4 world = float4x4(inst.Col0, inst.Col1, inst.Col2, inst.Col3);
    float4x4 viewProj = float4x4(g_ViewProjCol0, g_ViewProjCol1, g_ViewProjCol2, g_ViewProjCol3);
    float4 worldPos = mul(world, float4(VSIn.Pos, 1.0));

    PSIn.Pos = mul(viewProj, worldPos);
    PSIn.Color = VSIn.Normal * 0.5 + 0.5;
}
