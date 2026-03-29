struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEXCOORD0;
    float3 WorldPos : WORLD_POS;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float3 normal = normalize(PSIn.Normal);
    float ndl = max(dot(normal, lightDir), 0.0);

    float3 baseColor = float3(0.7, 0.7, 0.7);
    float3 ambient = baseColor * 0.3;
    float3 diffuse = baseColor * 0.7 * ndl;

    PSOut.Color = float4(ambient + diffuse, 1.0);
}
