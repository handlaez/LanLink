#include <iostream>
#include <filesystem>
#include <fstream>

#include "WinDuplicationGrabber.hpp"
#include "WinFrameConverter.hpp"
#include "stb_image_write.h"

int main()
{
    WinDuplicationGrabber grabber;

    if (!grabber.Initialize()) {
        std::cerr << "Failed to initialize grabber.\n";
        return 1;
    }

    FrameData frame;
    bool frameCaptured = false;

    for (int i = 0; i < 50; ++i) {
        if (grabber.CaptureFrame(frame)) {
            frameCaptured = true;
            break;
        }
        Sleep(16); // few first frames are pitch black hence the sleep
    }

    if (!frameCaptured) {
        std::cerr << "Failed to capture a valid frame.\n";
        return 1;
    }

    auto* bgraTexture = static_cast<ID3D11Texture2D*>(frame.nativeTextureHandle);

    D3D11_TEXTURE2D_DESC desc{};
    bgraTexture->GetDesc(&desc);

    std::cout << "Width: " << desc.Width << " Height: " << desc.Height << " Format: " << desc.Format << ".\n";

    D3D11_TEXTURE2D_DESC nv12Desc{};
    nv12Desc.Width = desc.Width;
    nv12Desc.Height = desc.Height;
    nv12Desc.MipLevels = 1;
    nv12Desc.ArraySize = 1;
    nv12Desc.Format = DXGI_FORMAT_NV12;
    nv12Desc.SampleDesc.Count = 1;
    nv12Desc.Usage = D3D11_USAGE_DEFAULT;
    nv12Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> outputNv12;

    HRESULT hr = grabber.getDevice()->CreateTexture2D(&nv12Desc, nullptr, &outputNv12);
    if (FAILED(hr)) {
        std::cerr << "Failed to create GPU NV12 texture.\n";
        grabber.ReleaseFrame();
        return 1;
    }

    WinFrameConverter converter(grabber.getDevice(), grabber.getContext());
    if (!converter.Initialize(desc.Width, desc.Height)) {
        std::cerr << "Failed to initialize GPU comp shader.\n";
        grabber.ReleaseFrame();
        return 1;
    }

    ConversionParams params{};
    params.width = desc.Width;
    params.height = desc.Height;
    params.inputNativeResource = bgraTexture;
    params.outputNativeResource = outputNv12.Get();

    if (!converter.ConvertBgraToNv12(params)) {
        std::cerr << "GPU BGRA->NV12 conversion failed.\n";
        grabber.ReleaseFrame();
        return 1;
    }

    std::cout << "Working directory: " << std::filesystem::current_path() << std::endl;
    std::cout << "Conversion complete.\n";

    // dumping texture to disk
    D3D11_TEXTURE2D_DESC stagingDesc = nv12Desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
    grabber.getDevice()->CreateTexture2D(&stagingDesc, nullptr, &staging);
    grabber.getContext()->CopyResource(staging.Get(), outputNv12.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = grabber.getContext()->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        std::ofstream yuvFile("capture.yuv", std::ios::binary);

        auto* srcBytes = static_cast<uint8_t*>(mapped.pData);

        // Y plane
        for (uint32_t y = 0; y < desc.Height; ++y) {
            yuvFile.write(reinterpret_cast<char*>(srcBytes + y * mapped.RowPitch), desc.Width);
        }

        // UV plane
        uint8_t* uvStart = srcBytes + (mapped.RowPitch * desc.Height);
        for (uint32_t y = 0; y < desc.Height / 2; ++y) {
            yuvFile.write(reinterpret_cast<char*>(uvStart + y * mapped.RowPitch), desc.Width);
        }

        grabber.getContext()->Unmap(staging.Get(), 0);
        std::cout << "Dumped raw GPU NV12 buffer to capture.yuv\n";
    }

    grabber.ReleaseFrame();
    return 0;
}