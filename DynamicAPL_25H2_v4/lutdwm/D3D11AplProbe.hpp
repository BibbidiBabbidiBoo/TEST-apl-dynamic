#pragma once
#ifdef _WIN32
#include "AplEstimator.hpp"
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <chrono>
namespace dynapl {
// One instance per output/monitor; MUST run only on DWM's graphics thread.
// No hooks or injection live here; caller supplies a PRE-LUT scRGB SRV.
// Return conservative mix=0 if formats are unsupported or capture unavailable.
class D3D11AplProbe {
public:
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    // Call from an already-active DWM Present hook, BEFORE the LUT draw.
    float OnFrame(ID3D11ShaderResourceView* preLutScRgbSrv);
    void Reset();
    Controller& GetController() {return m_ctrl;}
    const char* LastError() const {return m_error;}
private:
    struct Slot {
        Microsoft::WRL::ComPtr<ID3D11Buffer> readback;
        Microsoft::WRL::ComPtr<ID3D11Query> event;
        bool pending=false;
    };
    bool Dispatch(ID3D11ShaderResourceView* srvr, Slot& slot);
    void Drain();
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_cs;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_histogramGpu;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_histogramUav;
    std::array<Slot,3> m_slots{};
    Controller m_ctrl;
    std::chrono::steady_clock::time_point m_lastCapture{};
    std::chrono::steady_clock::time_point m_lastRead{};
    const char* m_error="Not initialized";
    bool m_ready=false;
};
} // namespace dynapl
#endif
