#pragma once
#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
namespace dynapl {
// Bridge helper: call Begin() immediately before HDR LUT pixel shader draw,
// then End() after draw. The fork still owns first LUT (t1).
class D3D11DualLutBindings {
public:
    bool Initialize(ID3D11Device* device, const std::wstring& lut2AplCube);
    bool Begin(ID3D11DeviceContext* context,float mix2);
    void End(ID3D11DeviceContext* context);
    void Reset();
private:
    Microsoft::WRL::ComPtr<ID3D11Texture3D> m_texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_lut2Srv;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_blendConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_previousSrv;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_previousCb;
    bool m_begun=false;
};
} // namespace dynapl
#endif
