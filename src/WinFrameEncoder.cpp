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

    hr = encoder_.As(&eventGenerator_);
    if (FAILED(hr)) {
        std::cerr << "Failed to get IMFMediaEventGenerator: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    initialized_ = true;
    std::cout << "Media Foundation HEVC encoder initialized\n";

    return true;
}

bool WinFrameEncoder::SubmitFrame(const VideoFrame& frame)
{
    if (!initialized_)
        return false;

    auto* texture = static_cast<ID3D11Texture2D*>(frame.nativeResource);

    if (!texture)
        return false;

    Microsoft::WRL::ComPtr<IMFSample> sample;

    if (!CreateInputSample(texture, frame.timestamp, &sample))
        return false;

    HRESULT hr = encoder_->ProcessInput(0, sample.Get(), 0);

    if (hr == MF_E_NOTACCEPTING)
        return false;

    if (FAILED(hr)) {
        std::cerr << "WinEncoder -- ProcessInput failed: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    return true;
}

bool WinFrameEncoder::ReceiveFrame(EncodedFrame& outFrame)
{
    if (!initialized_)
        return false;

    // output requirements.
    MFT_OUTPUT_STREAM_INFO streamInfo{};
    HRESULT hr = encoder_->GetOutputStreamInfo(0, &streamInfo);

    if (FAILED(hr)) {
        std::cerr << "GetOutputStreamInfo failed: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    Microsoft::WRL::ComPtr<IMFSample> sample;
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;

    MFT_OUTPUT_DATA_BUFFER output{};
    output.dwStreamID = 0;

    if (!(streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES))
    {
        hr = MFCreateSample(&sample);
        if (FAILED(hr))
            return false;

        hr = MFCreateMemoryBuffer(streamInfo.cbSize, &buffer);
        if (FAILED(hr))
            return false;

        sample->AddBuffer(buffer.Get());
        output.pSample = sample.Get();
    }

    DWORD status = 0;

    hr = encoder_->ProcessOutput(0, 1, &output, &status);

    // no frame is ready yet
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        return false;

    if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
        std::cerr << "Encoder output stream changed.\n";
        return false;
    }

    if (FAILED(hr)) {
        std::cerr << "ProcessOutput failed: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    IMFSample* outputSample = output.pSample ? output.pSample : sample.Get();

    if (!outputSample)
        return false;

    Microsoft::WRL::ComPtr<IMFMediaBuffer> contiguous;

    hr = outputSample->ConvertToContiguousBuffer(&contiguous);
    if (FAILED(hr))
        return false;

    BYTE* data = nullptr;
    DWORD maxLen = 0;
    DWORD curLen = 0;

    hr = contiguous->Lock(&data, &maxLen, &curLen);
    if (FAILED(hr))
        return false;

    outFrame.data.assign(data, data + curLen);

    contiguous->Unlock();

    LONGLONG ts = 0;
    if (SUCCEEDED(outputSample->GetSampleTime(&ts)))
        outFrame.timestamp = static_cast<uint64_t>(ts);

    // keyframe detection
    UINT32 cleanPoint = FALSE;
    if (SUCCEEDED(outputSample->GetUINT32(MFSampleExtension_CleanPoint, &cleanPoint)) && cleanPoint) {
        outFrame.type = EncodedFrameType::Keyframe;
    }
    else {
        outFrame.type = EncodedFrameType::Delta;
    }

    if (output.pEvents)
        output.pEvents->Release();

    return true;
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

bool WinFrameEncoder::CreateInputSample(ID3D11Texture2D* texture, uint64_t timestamp, IMFSample** sample)
{
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;

    HRESULT hr = MFCreateDXGISurfaceBuffer( __uuidof(ID3D11Texture2D), texture, 0, FALSE, &buffer);

    if (FAILED(hr)) {
        std::cerr << "MFCreateDXGISurfaceBuffer failed: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    hr = MFCreateSample(sample);
    if (FAILED(hr)) {
        std::cerr << "MFCreateSample failed: 0x" << std::hex << hr << std::dec << '\n';
        return false;
    }

    hr = (*sample)->AddBuffer(buffer.Get());
    if (FAILED(hr))
        return false;

    // MediaFoundation timestamps are in 100-ns units.
    (*sample)->SetSampleTime(static_cast<LONGLONG>(timestamp));

    const LONGLONG duration = 10'000'000LL / fps_;
    (*sample)->SetSampleDuration(duration);

    return true;
}
