#ifndef WIN_FRAME_ENCODER_HPP
#define WIN_FRAME_ENCODER_HPP

#include <wrl/client.h>
#include <d3d11.h>
#include <d3d11_4.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mftransform.h>
#include <Mferror.h>

#include "IFrameEncoder.hpp"

class WinFrameEncoder : public IFrameEncoder
{
public:
    WinFrameEncoder(ID3D11Device* device, ID3D11DeviceContext* context);
    ~WinFrameEncoder() override;

    bool Initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) override;
    bool SubmitFrame(const VideoFrame& frame) override;
    bool ReceiveFrame(EncodedFrame& outFrame) override;
    void Shutdown() override;

private:
    bool initialized_ = false;

    bool CreateInputSample(ID3D11Texture2D* texture, uint64_t timestamp, IMFSample** sample);

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t fps_ = 0;
    uint32_t bitrate_ = 0;
    uint64_t frameIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IMFTransform> encoder_;
    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> dxgiManager_;
    UINT dxgiResetToken_ = 0;
    Microsoft::WRL::ComPtr<IMFMediaEventGenerator> eventGenerator_;
};

#endif // WIN_FRAME_ENCODER_HPP