// Portable, shader-only component for a future dwm_lut HDR pass.
// IMPORTANT: The hooked 25H2 shader must feed BT.2020/ST2084 PQ values;
// these functions do not convert DWM's pre-LUT scRGB texture themselves.
// Bind 10% LUT to t1 and 2% LUT to t3 only AFTER verifying those slots
// against the fork's real pixel shader, which is NOT included in this kit.
// LUT .cube R-fastest => D3D11 Texture3D x=R, y=G, z=B.

float3 PQToLinear10000(float3 pq)
{
    const float m1 = 2610.0 / 16384.0;
    const float m2 = 2523.0 / 32.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;
    float3 a = pow(saturate(pq), 1.0 / m2);
    return pow(max(a - c1, 0.0) / max(c2 - c3 * a, 1e-7), 1.0 / m1);
}

float3 Linear10000ToPQ(float3 linear)
{
    const float m1 = 2610.0 / 16384.0;
    const float m2 = 2523.0 / 32.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;
    float3 a = pow(saturate(linear), m1);
    return pow((c1 + c2 * a) / (1.0 + c3 * a), m2);
}

float3 SampleCubeTetra(Texture3D<float4> lut, float3 inputPQ, uint size)
{
    size = max(size, 2u);
    float3 pos = saturate(inputPQ) * float(size - 1u);
    int3 base = min((int3)floor(pos), int3(size - 2u, size - 2u, size - 2u));
    float3 f = saturate(pos - float3(base));
    float3 c000 = lut.Load(int4(base,0)).rgb;
    float3 c111 = lut.Load(int4(base + int3(1,1,1),0)).rgb;
    float3 c100, c010, c001, c110, c101, c011;
    if (f.r >= f.g) {
        c100 = lut.Load(int4(base + int3(1,0,0),0)).rgb;
        if (f.g >= f.b) {
            c110 = lut.Load(int4(base + int3(1,1,0),0)).rgb;
            return c000 + f.r*(c100-c000) + f.g*(c110-c100) + f.b*(c111-c110);
        } else if (f.r >= f.b) {
            c101 = lut.Load(int4(base + int3(1,0,1),0)).rgb;
            return c000 + f.r*(c100-c000) + f.b*(c101-c100) + f.g*(c111-c101);
        } else {
            c001 = lut.Load(int4(base + int3(0,0,1),0)).rgb;
            c101 = lut.Load(int4(base + int3(1,0,1),0)).rgb;
            return c000 + f.b*(c001-c000) + f.r*(c101-c001) + f.g*(c111-c101);
        }
    } else {
        c010 = lut.Load(int4(base + int3(0,1,0),0)).rgb;
        if (f.r >= f.b) {
            c110 = lut.Load(int4(base + int3(1,1,0),0)).rgb;
            return c000 + f.g*(c010-c000) + f.r*(c110-c010) + f.b*(c111-c110);
        } else if (f.g >= f.b) {
            c011 = lut.Load(int4(base + int3(0,1,1),0)).rgb;
            return c000 + f.g*(c010-c000) + f.b*(c011-c010) + f.r*(c111-c011);
        } else {
            c001 = lut.Load(int4(base + int3(0,0,1),0)).rgb;
            c011 = lut.Load(int4(base + int3(0,1,1),0)).rgb;
            return c000 + f.b*(c001-c000) + f.g*(c011-c001) + f.r*(c111-c011);
        }
    }
}

// Uses identical PQ input for both .cube LUTs, and blends in *linear light*.
float3 ApplyDualHdrLut(Texture3D<float4> lut10, Texture3D<float4> lut2,
                       float3 inputPQ, uint lutSize, float blend2)
{
    float3 out10 = SampleCubeTetra(lut10, inputPQ, lutSize);
    float3 out02 = SampleCubeTetra(lut2, inputPQ, lutSize);
    float3 lin = lerp(PQToLinear10000(out10), PQToLinear10000(out02), saturate(blend2));
    return Linear10000ToPQ(lin);
}
