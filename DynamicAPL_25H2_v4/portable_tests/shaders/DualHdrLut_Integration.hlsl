// HLSL INTEGRATION TEMPLATE, not a drop-in shader.
// This extends the existing 25H2 HDR 3D-LUT pass, preserving the fork's
// tetrahedral interpolation. The exact resource slots must be verified.
// Confirm target shader input is HDR10 (BT.2020 PQ) and output is linear scRGB.

Texture3D<float4> lut2Apl : register(t3);       // NEW SRV alongside existing LUT t1
cbuffer DynamicAplBlend : register(b1) {        // NEW dynamic constant buffer
    float blend2Apl;  // 0 = 10% correction; 1 = 2% correction
    float3 reservedApl;
};

// IMPLEMENT IN THE 25H2 FORK:
// 1. Generalize existing LutTransformTetrahedral to accept Texture3D LUT.
// 2. Resolve the same 'lutSize', sampler mode, and tetrahedral interpolation
//    for both profiles, not trilinear sampling for one profile.
// 3. Existing pq_eotf and pq_inv_eotf must have identical scaling (0..1
//    for 0..10,000 nits), validate against upstream shader before building.
// float3 pq10 = LutTransformTetrahedral(inputPQ, originalLut);
// float3 pq02 = LutTransformTetrahedral(inputPQ, lut2Apl);
// float3 outLinear = lerp(pq_eotf(pq10), pq_eotf(pq02), saturate(blend2Apl));
// return pq_inv_eotf(max(outLinear,0));
// The blend is IN LINEAR LIGHT, NOT directly on PQ-encoded values.
// Never activate this without the fully bound second LUT and b1 buffer.
