#include "WinFrameConverter.hpp"
#include <d3dcompiler.h>
#include <iostream>

// HLSL Shader Source embedded as a raw string literal
static const char* g_BgraToNv12ShaderSource = R"(
Texture2D<float4> inputTex : register(t0);
RWTexture2D<float> outputY : register(u0);
RWTexture2D<float2> outputUV : register(u1);

[numthreads(16, 16, 1)]
void main(uint2 pos : SV_DispatchThreadID)
{
    uint width, height;
    inputTex.GetDimensions(width, height);
    if (pos.x >= width || pos.y >= height)
        return;

    float3 c0 = saturate(inputTex[pos].rgb);
    outputY[pos] = saturate(dot(c0, float3(0.182586, 0.614231, 0.062007)) + (16.0 / 255.0));

    if (all((pos & 1) == 0))
    {
        uint2 maxPos = uint2(width - 1, height - 1);
        uint2 p1 = uint2(min(pos.x + 1, maxPos.x), pos.y);
        uint2 p2 = uint2(pos.x, min(pos.y + 1, maxPos.y));
        uint2 p3 = min(pos + 1, maxPos);

        float3 avg = (c0 + saturate(inputTex[p1].rgb) + saturate(inputTex[p2].rgb) + saturate(inputTex[p3].rgb)) * 0.25;

        float u = dot(avg, float3(-0.100644, -0.338572, 0.439216)) + (128.0 / 255.0);
        float v = dot(avg, float3(0.439216, -0.398942, -0.040274)) + (128.0 / 255.0);

        outputUV[pos / 2] = saturate(float2(u, v));
    }
}
)";

WinFrameConverter::WinFrameConverter(ID3D11Device* device, ID3D11DeviceContext* context)
    : m_device(device), m_context(context)
{
    if (!m_device || !m_context) {
        throw std::invalid_argument("WinFrameConverter: Device and Context cannot be null.");
    }
}

bool WinFrameConverter::Initialize(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;

    try {
        CreateComputeShader();
        m_initialized = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "WinFrameConverter: " << e.what() << ".\n";
        return false;
    }
}

void WinFrameConverter::CreateComputeShader() {
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompile(
        g_BgraToNv12ShaderSource,
        strlen(g_BgraToNv12ShaderSource),
        "BgraToNv12Inline",
        nullptr, nullptr,
        "main", "cs_5_0",
        compileFlags, 0,
        &shaderBlob, &errorBlob
    );

    if (FAILED(hr)) {
        std::string errorMsg = "Failed to compile CS shader.";
        if (errorBlob) {
            errorMsg += " Details: ";
            errorMsg.append(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        }
        throw std::runtime_error(errorMsg);
    }

    hr = m_device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &m_computeShader);

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create DX11 Compute Shader.");
    }
}

void WinFrameConverter::RecreateViewsForOutput(ID3D11Texture2D* pOutputNv12) {
    m_uavY.Reset();
    m_uavUV.Reset();

    // Y channel (DXGI_FORMAT_R8_UNORM)
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavYDesc{};
    uavYDesc.Format = DXGI_FORMAT_R8_UNORM;
    uavYDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavYDesc.Texture2D.MipSlice = 0;

    HRESULT hr = m_device->CreateUnorderedAccessView(pOutputNv12, &uavYDesc, &m_uavY);
    if (FAILED(hr)) throw std::runtime_error("Failed to create UAV for Y plane.");

    // UV channels (DXGI_FORMAT_R8G8_UNORM)
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavUVDesc{};
    uavUVDesc.Format = DXGI_FORMAT_R8G8_UNORM;
    uavUVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavUVDesc.Texture2D.MipSlice = 0;

    hr = m_device->CreateUnorderedAccessView(pOutputNv12, &uavUVDesc, &m_uavUV);
    if (FAILED(hr)) throw std::runtime_error("Failed to create UAV for UV plane.");

    m_lastOutputTexture = pOutputNv12;
}

bool WinFrameConverter::ConvertBgraToNv12(const ConversionParams& params) {
    if (!m_initialized) return false;

    auto* pInputBgra = static_cast<ID3D11Texture2D*>(params.inputNativeResource.nativeResource);
    auto* pOutputNv12 = static_cast<ID3D11Texture2D*>(params.outputNativeResource.nativeResource);

    if (!pInputBgra || !pOutputNv12) return false;

    // caching UAVs if same output texture is reused
    if (m_lastOutputTexture != pOutputNv12) {
        try {
            RecreateViewsForOutput(pOutputNv12);
        }
        catch (const std::exception& e) {
            std::cerr << "[WinFrameConverter Error] " << e.what() << "\n";
            return false;
        }
    }

    // temp SRV for input
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* inputSRV = nullptr;
    auto it = m_srvCache.find(pInputBgra);
    if (it != m_srvCache.end()) {
        inputSRV = it->second.Get();
    }
    else {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newSrv;
        HRESULT hr = m_device->CreateShaderResourceView(pInputBgra, &srvDesc, &newSrv);
        if (FAILED(hr)) return false;

        inputSRV = newSrv.Get();
        m_srvCache[pInputBgra] = newSrv;
    }

    // bind
    m_context->CSSetShader(m_computeShader.Get(), nullptr, 0);

    ID3D11ShaderResourceView* srvs[] = { inputSRV };
    m_context->CSSetShaderResources(0, 1, srvs);

    ID3D11UnorderedAccessView* uavs[] = { m_uavY.Get(), m_uavUV.Get() };
    m_context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    // dispatch (16x16)
    UINT dispatchX = (params.outputNativeResource.width + 15) / 16;
    UINT dispatchY = (params.outputNativeResource.height + 15) / 16;
    m_context->Dispatch(dispatchX, dispatchY, 1);

    // unbind
    ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr, nullptr };
    ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
    m_context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
    m_context->CSSetShaderResources(0, 1, nullSRVs);

    return true;
}