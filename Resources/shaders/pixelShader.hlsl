// struct VSOutput{ // this is what gets passed to the pixel shader
//     float4 pos : SV_POSITION;
//     float4 col : COLOR;
// };

struct PSInput{ // this is what gets passed to the pixel shader
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET{
    return input.col;
} 