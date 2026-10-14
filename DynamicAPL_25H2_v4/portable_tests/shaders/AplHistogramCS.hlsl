// Pre-LUT scRGB input: R16G16B16A16_FLOAT/R32G32B32A32_FLOAT,
// DWM render linear Rec709 (unit scRGB Y=1 corresponds to 80 nits).
// Histogram is LOG2 luminance: bin floor((log2(nits)+4)/0.5), clamped 0..31.
Texture2D<float4> preLutScRgb : register(t0);
RWStructuredBuffer<uint> histogram : register(u0);
groupshared uint groupBins[32];

[numthreads(8,8,1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID,
          uint3 groupThreadID : SV_GroupThreadID) {
    uint lane=groupThreadID.y * 8 + groupThreadID.x;
    if (lane < 32) groupBins[lane]=0;
    GroupMemoryBarrierWithGroupSync();

    uint width, height;
    preLutScRgb.GetDimensions(width,height);
    if(dispatchThreadID.x < 256 && dispatchThreadID.y < 144 && width>0 && height>0) {
        uint2 p=uint2(
          min(width-1, (dispatchThreadID.x*2+1)*width/(256*2)),
          min(height-1, (dispatchThreadID.y*2+1)*height/(144*2)));
        float3 rgb=preLutScRgb.Load(int3(p,0)).rgb;
        // Negative linear scRGB RGB components are legitimate wide-gamut values.
        float yNits=max(0.0,80.0*dot(rgb,float3(.2126,.7152,.0722)));
        uint bin=0;
        if(yNits>0.0625) {
          bin=(uint)clamp(floor((log2(yNits)+4.0)*2.0),0.0,31.0);
        }
        InterlockedAdd(groupBins[bin],1);
    }
    GroupMemoryBarrierWithGroupSync();
    if(lane < 32) {
        InterlockedAdd(histogram[lane],groupBins[lane]);
    }
}
