#include <iostream>
#include <filesystem>

#include "WinDuplicationGrabber.hpp"
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

    auto* texture = static_cast<ID3D11Texture2D*>(frame.nativeTextureHandle);

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    std::cout << "Width: " << desc.Width << " Height: " << desc.Height << " Format: " << desc.Format << ".\n";

    D3D11_TEXTURE2D_DESC stagingDesc{};
    stagingDesc.Width = desc.Width;
    stagingDesc.Height = desc.Height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = desc.Format; // DXGI_FORMAT_B8G8R8A8_UNORM
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.BindFlags = 0;
    stagingDesc.MiscFlags = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;

    HRESULT hr = grabber.getDevice()->CreateTexture2D(&stagingDesc, nullptr, &staging);

    if (FAILED(hr)) {
        std::cerr << "Failed to create staging texture.\n";
        grabber.ReleaseFrame();
        return 1;
    }

    grabber.getContext()->CopyResource(staging.Get(), texture);

    D3D11_MAPPED_SUBRESOURCE mapped{};

    hr = grabber.getContext()->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);

    uint8_t* p = static_cast<uint8_t*>(mapped.pData);

    std::cout << "First pixel BGRA = " << (int)p[0] << ", " << (int)p[1] << ", " << (int)p[2] << ", " << (int)p[3] << ".\n";

    if (FAILED(hr)) {
        std::cerr << "Map failed\n";
        grabber.ReleaseFrame();
        return 1;
    }

    std::cout << "Working directory: " << std::filesystem::current_path() << std::endl;

    uint8_t* pixels = static_cast<uint8_t*>(mapped.pData);

    for (uint32_t y = 0; y < desc.Height; ++y) {
        uint8_t* row = pixels + y * mapped.RowPitch;

        for (uint32_t x = 0; x < desc.Width; ++x) {
            uint8_t b = row[x * 4 + 0]; // DXGI Blue
            uint8_t r = row[x * 4 + 2]; // DXGI Red

            row[x * 4 + 0] = r;   // set to Red
            row[x * 4 + 2] = b;   // set to Blue
            row[x * 4 + 3] = 255; // alpha to opaque
        }
    }

    int ok = stbi_write_png("capture.png", static_cast<int>(desc.Width), static_cast<int>(desc.Height), 4, mapped.pData, static_cast<int>(mapped.RowPitch));

    grabber.getContext()->Unmap(staging.Get(), 0);
    grabber.ReleaseFrame();

    if (!ok) {
        std::cerr << "PNG save failed\n";
        return 1;
    }

    std::cout << "Saved capture.png\n";
    return 0;
}