#include <iostream>

#include "WinFrameEncoder.hpp"

WinFrameEncoder::WinFrameEncoder(ID3D11Device* device, ID3D11DeviceContext* context)
	: device_(device), context_(context)
{
}

WinFrameEncoder::~WinFrameEncoder()
{
	Shutdown();
}

bool WinFrameEncoder::Initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate)
{
    if (initialized_)
        return true;

    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cerr << "MFStartup failed: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    hr = MFCreateDXGIDeviceManager(&dxgiResetToken_, &dxgiManager_);
    if (FAILED(hr)) {
        std::cerr << "MFCreateDXGIDeviceManager failed\n";
        return false;
    }

    // multithreading protection
    Microsoft::WRL::ComPtr<ID3D11Multithread> multithread;
    hr = context_->QueryInterface(IID_PPV_ARGS(&multithread));
    if (SUCCEEDED(hr)) {
        multithread->SetMultithreadProtected(TRUE);
    }
    else {
        std::cerr << "Failed to enable D3D11 multithread protection: 0x"
            << std::hex << hr << std::dec << '\n';
        return false;
    }

    hr = dxgiManager_->ResetDevice(device_.Get(), dxgiResetToken_);
    if (FAILED(hr)) {
        std::cerr << "ResetDevice failed\n";
        return false;
    }

    IMFActivate** activates = nullptr;
    UINT32 count = 0;

    MFT_REGISTER_TYPE_INFO inputInfo{};
    inputInfo.guidMajorType = MFMediaType_Video;
    inputInfo.guidSubtype = MFVideoFormat_NV12;

    MFT_REGISTER_TYPE_INFO outputInfo{};
    outputInfo.guidMajorType = MFMediaType_Video;
    outputInfo.guidSubtype = MFVideoFormat_HEVC;

    hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, MFT_ENUM_FLAG_HARDWARE, &inputInfo, &outputInfo, &activates, &count);

    if (FAILED(hr) || count == 0) {
        std::cerr << "No hardware HEVC encoder found\n";
        return false;
    }

    bool encoderReady = false;

    // finding an encoder matching the device
    for (UINT32 i = 0; i < count; ++i) {
        hr = activates[i]->ActivateObject(IID_PPV_ARGS(&encoder_));
        if (SUCCEEDED(hr)) {
            Microsoft::WRL::ComPtr<IMFAttributes> attributes;
            if (SUCCEEDED(encoder_->GetAttributes(&attributes))) {
                attributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE);
            }

            hr = encoder_->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(dxgiManager_.Get()));

            if (SUCCEEDED(hr)) {
                encoderReady = true;
                break; 
            }

            encoder_.Reset();
        }
    }

    for (UINT32 i = 0; i < count; ++i) {
        activates[i]->Release();
    }
    CoTaskMemFree(activates);

    if (!encoderReady) {
        std::cerr << "Failed to find an HEVC encoder compatible with this D3D11 device.\n";
        return false;
    }

    if (FAILED(hr))
    {
        std::cerr << "MFT_MESSAGE_SET_D3D_MANAGER failed: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    // out HEVC
    Microsoft::WRL::ComPtr<IMFMediaType> outType;
    hr = MFCreateMediaType(&outType);
    if (FAILED(hr))
        return false;

    outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_HEVC);
    outType->SetUINT32(MF_MT_AVG_BITRATE, bitrate_);
    outType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    MFSetAttributeSize(outType.Get(), MF_MT_FRAME_SIZE, width_, height_);
    MFSetAttributeRatio(outType.Get(), MF_MT_FRAME_RATE, fps_, 1);
    MFSetAttributeRatio(outType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = encoder_->SetOutputType(0, outType.Get(), 0);
    if (FAILED(hr)) {
        std::cerr << "SetOutputType failed: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    // in nv12
    Microsoft::WRL::ComPtr<IMFMediaType> inType;
    hr = MFCreateMediaType(&inType);
    if (FAILED(hr))
        return false;

    inType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    inType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    MFSetAttributeSize(inType.Get(), MF_MT_FRAME_SIZE, width_, height_);
    MFSetAttributeRatio(inType.Get(), MF_MT_FRAME_RATE, fps_, 1);
    MFSetAttributeRatio(inType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = encoder_->SetInputType(0, inType.Get(), 0);
    if (FAILED(hr)) {
        std::cerr << "SetInputType failed: 0x"
            << std::hex << hr << std::dec << '\n';
        return false;
    }

    encoder_->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    encoder_->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    encoder_->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

    initialized_ = true;
    std::cout << "Media Foundation HEVC encoder initialized\n";

    return true;
}

bool WinFrameEncoder::EncodeFrame(const VideoFrame& frame, EncodedFrame& outFrame)
{
	return false;
}

void WinFrameEncoder::Shutdown()
{
    if (!initialized_)
        return;

    if (encoder_) {
        encoder_->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
        encoder_->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
    }

    encoder_.Reset();
    dxgiManager_.Reset();
    context_.Reset();
    device_.Reset();

    MFShutdown();

    initialized_ = false;
}
