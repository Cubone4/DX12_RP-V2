struct VSInput {
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 projection;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, world);
    pos = mul(pos, view);
    pos = mul(pos, projection);

    output.pos = pos;
    output.col = input.color;

    return output;
}
