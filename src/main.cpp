#include <iostream>
#include <filesystem>
#include <fstream>

#include "WinFrameGrabber.hpp"
#include "WinFrameConverter.hpp"
#include "WinFrameEncoder.hpp"
#include "UnifiedPacketizer.hpp"
#include "WinPacketSender.hpp"

int main()
{
    WinFrameGrabber grabber;

    if (!grabber.Initialize()) {
        std::cerr << "Failed to initialize grabber.\n";
        return 1;
    }

    VideoFrame bgraFrame{};
    bool frameCaptured = false;

    for (int i = 0; i < 40; ++i) {
        if (grabber.CaptureFrame(bgraFrame)) {
            frameCaptured = true;
            break;
        }
        Sleep(4);
    }
    if (!frameCaptured) {
        std::cerr << "Failed to capture a valid frame (DXGI timeout).\n";
        return 1;
    }

    auto* bgraTexture = static_cast<ID3D11Texture2D*>(bgraFrame.nativeResource);
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

    VideoFrame nv12Frame{};
    nv12Frame.nativeResource = outputNv12.Get();
    nv12Frame.width = desc.Width;
    nv12Frame.height = desc.Height;

    WinFrameConverter converter(grabber.getDevice(), grabber.getContext());
    if (!converter.Initialize(desc.Width, desc.Height)) {
        std::cerr << "Failed to initialize GPU comp shader.\n";
        grabber.ReleaseFrame();
        return 1;
    }

    ConversionParams params{};
    params.inputNativeResource = bgraFrame;
    params.outputNativeResource = nv12Frame;

    if (!converter.ConvertBgraToNv12(params)) {
        std::cerr << "GPU BGRA->NV12 conversion failed.\n";
        grabber.ReleaseFrame();
        return 1;
    }

    WinFrameEncoder encoder(grabber.getDevice(), grabber.getContext());
    if (!encoder.Initialize(desc.Width, desc.Height, 60, 8'000'000)) {
        std::cerr << "Encoder init failed.\n";
        return 1;
    }

    // pipeline test
    UnifiedPacketizer packetizer;
    WinPacketSender packetSender;

    if (!packetSender.Open("127.0.0.1", 12345)) {
        std::cerr << "Failed to open WinPacketSender socket.\n";
        return 1;
    }

    EncodedFrame encoded;

    for (int i = 0; i < 120; ++i)
    {
        nv12Frame.timestamp = i * (10'000'000ULL / 60);

        encoder.SubmitFrame(nv12Frame);

        Sleep(16); // ~60 FPS pacing

        if (encoder.ReceiveFrame(encoded))
        {
            auto packets = packetizer.Packetize(encoded);

            for (auto packet : packets) {
                packetSender.Send(packet.bytes);
            }

            std::cout << "Sent " << packets.size() << " UDP packets (ts: " << encoded.timestamp << ")\n";
        }
    }

    grabber.ReleaseFrame();
    return 0;
}