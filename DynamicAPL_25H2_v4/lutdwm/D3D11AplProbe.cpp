#include "pch.h" // MSVC project uses precompiled headers.
#ifdef _WIN32
#include "D3D11AplProbe.hpp"
#include "AplHistogramEmbedded.hpp"
#include <d3dcompiler.h>
#include <cstring>
#include <iterator>
using namespace Microsoft::WRL;
using namespace dynapl;
using Clock=std::chrono::steady_clock;

bool D3D11AplProbe::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    Reset();
    if (!device||!context) {m_error="Missing D3D11 device/context";return false;}
    m_device=device; m_context=context;
    ComPtr<ID3DBlob> blob, err;
    HRESULT hr=D3DCompile(kHistogramCS, std::strlen(kHistogramCS),"AplHistogramCS.hlsl",
                          nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,
                          &blob,&err);
    if (FAILED(hr)) {m_error="Histogram HLSL compilation failed (see D3D compiler)";Reset();return false;}
    hr=m_device->CreateComputeShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&m_cs);
    if (FAILED(hr)) {m_error="CreateComputeShader failed";Reset();return false;}
    D3D11_BUFFER_DESC b{};
    b.ByteWidth=sizeof(Histogram); b.Usage=D3D11_USAGE_DEFAULT;
    b.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
    b.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    b.StructureByteStride=sizeof(uint32_t);
    hr=m_device->CreateBuffer(&b,nullptr,&m_histogramGpu);
    if (FAILED(hr)) {m_error="CreateBuffer histogram failed";Reset();return false;}
    D3D11_UNORDERED_ACCESS_VIEW_DESC v{};
    v.Format=DXGI_FORMAT_UNKNOWN;
    v.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;
    v.Buffer.NumElements=kBins;
    hr=m_device->CreateUnorderedAccessView(m_histogramGpu.Get(),&v,&m_histogramUav);
    if (FAILED(hr)) {m_error="CreateUnorderedAccessView failed";Reset();return false;}
    b.Usage=D3D11_USAGE_STAGING; b.BindFlags=0; b.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    b.MiscFlags=0; b.StructureByteStride=0;
    for(auto& slot:m_slots) {
        hr=m_device->CreateBuffer(&b,nullptr,&slot.readback);
        if(FAILED(hr)) {m_error="CreateBuffer staging failed";Reset();return false;}
        D3D11_QUERY_DESC q{D3D11_QUERY_EVENT,0};
        hr=m_device->CreateQuery(&q,&slot.event);
        if(FAILED(hr)) {m_error="CreateQuery failed";Reset();return false;}
    }
    m_ctrl.Reset();m_lastCapture=Clock::now();m_lastRead={};
    m_ready=true;m_error="OK";
    return true;
}

void D3D11AplProbe::Reset() {
    m_ready=false;
    for(auto& s:m_slots){s.pending=false;s.readback.Reset();s.event.Reset();}
    m_histogramUav.Reset();m_histogramGpu.Reset();m_cs.Reset();
    m_context.Reset();m_device.Reset();
    m_ctrl.Reset();m_lastCapture={};m_lastRead={};
}

void D3D11AplProbe::Drain() {
    if (!m_context)return;
    for(auto& slot:m_slots) {
        if(!slot.pending)continue;
        // Critical: never block the DWM Present thread on GPU completion.
        if(m_context->GetData(slot.event.Get(),nullptr,0,D3D11_ASYNC_GETDATA_DONOTFLUSH)!=S_OK)
            continue;
        D3D11_MAPPED_SUBRESOURCE map{};
        if(SUCCEEDED(m_context->Map(slot.readback.Get(),0,D3D11_MAP_READ,0,&map))) {
            Histogram histogram{};
            std::memcpy(histogram.data(),map.pData,sizeof(Histogram));
            m_context->Unmap(slot.readback.Get(),0);
            const auto now=Clock::now();
            const float dt=(m_lastRead==Clock::time_point{}) ? 0.25f :
                std::chrono::duration<float>(now-m_lastRead).count();
            m_lastRead=now;
            m_ctrl.Update(AnalyzeHistogram(histogram),dt);
        }
        slot.pending=false;
    }
}

bool D3D11AplProbe::Dispatch(ID3D11ShaderResourceView* srv,Slot& slot) {
    if(!m_ready || !srv)return false;
    // Verify SRV sees a linear float scRGB texture. Do not analyze PQ-coded data.
    ComPtr<ID3D11Resource> resource;
    srv->GetResource(&resource);
    ComPtr<ID3D11Texture2D> tex;
    if(FAILED(resource.As(&tex)))return false;
    D3D11_TEXTURE2D_DESC desc{};tex->GetDesc(&desc);
    if(desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT &&
       desc.Format!=DXGI_FORMAT_R32G32B32A32_FLOAT) return false;

    // Restore the compute pipeline after analysis; dwm_lut's own compute state
    // and any other DWM caller must be unaffected.
    ComPtr<ID3D11ComputeShader> oldCs;
    ComPtr<ID3D11ShaderResourceView> oldSrv;
    ComPtr<ID3D11UnorderedAccessView> oldUav;
    m_context->CSGetShader(&oldCs,nullptr,nullptr);
    m_context->CSGetShaderResources(0,1,&oldSrv);
    m_context->CSGetUnorderedAccessViews(0,1,&oldUav);

    const UINT zeroes[4]={0,0,0,0};
    m_context->ClearUnorderedAccessViewUint(m_histogramUav.Get(),zeroes);
    auto* cs=m_cs.Get();auto* uav=m_histogramUav.Get();
    m_context->CSSetShader(cs,nullptr,0);
    m_context->CSSetShaderResources(0,1,&srv);
    m_context->CSSetUnorderedAccessViews(0,1,&uav,nullptr);
    m_context->Dispatch(kWidth/8,kHeight/8,1);
    // Must unbind UAV before CopyResource into staging.
    ID3D11UnorderedAccessView* noUav=nullptr;
    m_context->CSSetUnorderedAccessViews(0,1,&noUav,nullptr);
    m_context->CopyResource(slot.readback.Get(),m_histogramGpu.Get());
    m_context->End(slot.event.Get());
    m_context->CSSetShaderResources(0,1,oldSrv.GetAddressOf());
    m_context->CSSetUnorderedAccessViews(0,1,oldUav.GetAddressOf(),nullptr);
    m_context->CSSetShader(oldCs.Get(),nullptr,0);
    slot.pending=true;
    return true;
}

float D3D11AplProbe::OnFrame(ID3D11ShaderResourceView* preLutScRgbSrv) {
    if(!m_ready)return 0.0f;
    Drain();
    const auto now=Clock::now();
    if(std::chrono::duration<float>(now-m_lastCapture).count()<.25f)
        return m_ctrl.mix2;
    // Limit inflight GPU readbacks so stale captures don't accumulate.
    for(auto& s:m_slots)if(!s.pending) {
        if(!Dispatch(preLutScRgbSrv,s)) {
            // Unsupported source formats are not treated as a small APL window.
            m_ctrl.Update(FrameStats{},.25f); // fade to 10%-profile baseline
        }
        m_lastCapture=now;
        return m_ctrl.mix2;
    }
    // GPU was overloaded or queries did not complete: avoid staying indefinitely
    // on a high 2%-APL boost if the input image is no longer readable.
    if(m_lastRead==Clock::time_point{} ||
       std::chrono::duration<float>(now-m_lastRead).count() > 1.5f)
        m_ctrl.Update(FrameStats{},.25f);
    return m_ctrl.mix2;
}
#endif
