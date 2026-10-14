// Shader COMPILE TEST ONLY. Output remains PQ, NOT the 25H2 DWM HDR output format.
// Do not use this file as a drop-in DWM shader.
#include "DualLutTetrahedral.hlsli"
Texture2D<float4> testInputPQ : register(t0);
Texture3D<float4> reference10 : register(t1);
Texture3D<float4> reference2 : register(t3);
SamplerState testSampler : register(s0);
cbuffer DualLutParameters : register(b1) {
    float blend2Apl;
    uint lutSize;
    float2 padding;
};
struct PSIn { float4 pos:SV_Position; float2 uv:TEXCOORD0; };
float4 main(PSIn input) : SV_Target {
    float3 pq = testInputPQ.SampleLevel(testSampler,input.uv,0).rgb;
    return float4(ApplyDualHdrLut(reference10,reference2,pq,lutSize,blend2Apl),1.0);
}
