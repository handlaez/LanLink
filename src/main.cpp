#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>

#include "WinFrameGrabber.hpp"
#include "WinFrameConverter.hpp"
#include "WinFrameEncoder.hpp"
#include "UnifiedPacketizer.hpp"
#include "WinPacketSender.hpp"

uint64_t now100ns()
{
    using namespace std::chrono;
    return duration_cast<duration<uint64_t, std::ratio<1, 10'000'000>>>(steady_clock::now().time_since_epoch()).count();
}

bool QueryFrameDimensions(WinFrameGrabber& grabber, UINT& outWidth, UINT& outHeight)
{
    VideoFrame bgraFrame{};
    for (int i = 0; i < 40; ++i) {
        if (grabber.CaptureFrame(bgraFrame)) {
            auto* texture = static_cast<ID3D11Texture2D*>(bgraFrame.nativeResource);
            D3D11_TEXTURE2D_DESC desc{};
            texture->GetDesc(&desc);

            outWidth = desc.Width;
            outHeight = desc.Height;

            grabber.ReleaseFrame(); // release probe frame immediately
            return true;
        }
        Sleep(4);
    }
    return false;
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> CreateNv12Texture(ID3D11Device* device, UINT width, UINT height)
{
    D3D11_TEXTURE2D_DESC nv12Desc{};
    nv12Desc.Width = width;
    nv12Desc.Height = height;
    nv12Desc.MipLevels = 1;
    nv12Desc.ArraySize = 1;
    nv12Desc.Format = DXGI_FORMAT_NV12;
    nv12Desc.SampleDesc.Count = 1;
    nv12Desc.Usage = D3D11_USAGE_DEFAULT;
    nv12Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> outputNv12;
    HRESULT hr = device->CreateTexture2D(&nv12Desc, nullptr, &outputNv12);
    if (FAILED(hr)) return nullptr;

    return outputNv12;
}

int main()
{
    WinFrameGrabber grabber;
    if (!grabber.Initialize()) {
        std::cerr << "Failed to initialize grabber.\n";
        return 1;
    }

    UINT width = 0, height = 0;
    if (!QueryFrameDimensions(grabber, width, height)) {
        std::cerr << "Failed to capture initial probe frame.\n";
        return 1;
    }
    std::cout << "Width: " << width << " Height: " << height << "\n";

    auto outputNv12 = CreateNv12Texture(grabber.getDevice(), width, height);
    if (!outputNv12) {
        std::cerr << "Failed to create GPU NV12 texture.\n";
        return 1;
    }

    VideoFrame nv12Frame{};
    nv12Frame.nativeResource = outputNv12.Get();
    nv12Frame.width = width;
    nv12Frame.height = height;

    WinFrameConverter converter(grabber.getDevice(), grabber.getContext());
    if (!converter.Initialize(width, height)) {
        std::cerr << "Failed to initialize GPU comp shader.\n";
        return 1;
    }

    WinFrameEncoder encoder(grabber.getDevice(), grabber.getContext());
    if (!encoder.Initialize(width, height, 60, 8'000'000)) {
        std::cerr << "Encoder init failed.\n";
        return 1;
    }

    UnifiedPacketizer packetizer;
    WinPacketSender packetSender;
    if (!packetSender.Open("192.168.0.106", 5000)) {
        std::cerr << "Failed to open WinPacketSender socket.\n";
        return 1;
    }

    EncodedFrame encoded;
    std::vector<Packet> packets;

    while (true)
    {
        VideoFrame bgraFrame{};

        if (grabber.CaptureFrame(bgraFrame))
        {
            ConversionParams params{};
            params.inputNativeResource = bgraFrame;
            params.outputNativeResource = nv12Frame;

            if (converter.ConvertBgraToNv12(params))
            {
                nv12Frame.timestamp = now100ns();
                encoder.SubmitFrame(nv12Frame);
                Sleep(16);

                if (encoder.ReceiveFrame(encoded))
                {
                    packetizer.Packetize(encoded, packets);
                    for (const auto& packet : packets) {
                        packetSender.Send(packet.bytes);
                    }
                }
            }

            grabber.ReleaseFrame();
        }
    }

    return 0;
}