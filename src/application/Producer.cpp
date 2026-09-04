#include "Producer.hpp"
#include "Logger.hpp"

#include <chrono>
#include <ratio>
#include <thread>
#include <vector>

#include <windows.h>

namespace {
    uint64_t now100ns()
    {
        using namespace std::chrono;

        return duration_cast<duration<uint64_t, std::ratio<1, 10'000'000>>>(steady_clock::now().time_since_epoch()).count();
    }

    bool queryFrameDimensions(WinFrameGrabber& grabber, uint32_t& outWidth, uint32_t& outHeight)
    {
        VideoFrame frame{};

        for (int i = 0; i < 40; ++i) {
            if (grabber.CaptureFrame(frame)) {
                auto* texture = static_cast<ID3D11Texture2D*>(frame.nativeResource);

                if (!texture) {
                    grabber.ReleaseFrame();
                    return false;
                }

                D3D11_TEXTURE2D_DESC desc{};
                texture->GetDesc(&desc);

                outWidth = desc.Width;
                outHeight = desc.Height;

                grabber.ReleaseFrame();
                return true;
            }

            Sleep(4);
        }

        return false;
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> createNv12Texture( ID3D11Device* device, uint32_t width, uint32_t height)
    {
        if (!device || width == 0 || height == 0) {
            return nullptr;
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_NV12;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;

        if (FAILED(device->CreateTexture2D(&desc, nullptr, &texture))) {
            return nullptr;
        }

        return texture;
    }

} // namespace

bool Producer::initialize(const std::string& address, uint16_t port)
{
    if (!frameGrabber_.Initialize()) {
        logger().error("Failed to initialize frame grabber.");
        return false;
    }

    if (!queryFrameDimensions(frameGrabber_, width_, height_)) {
        logger().error("Failed to determine frame dimensions.");
        return false;
    }

    logger().info(QString("Width: %1 Height: %2").arg(width_).arg(height_));

    nv12Texture_ = createNv12Texture(frameGrabber_.getDevice(), width_, height_);

    if (!nv12Texture_) {
        logger().error("Failed to create GPU NV12 texture.");
        return false;
    }

    nv12Frame_.nativeResource = nv12Texture_.Get();
    nv12Frame_.width = width_;
    nv12Frame_.height = height_;

    frameConverter_ = std::make_unique<WinFrameConverter>(
        frameGrabber_.getDevice(),
        frameGrabber_.getContext());

    if (!frameConverter_->Initialize(width_, height_)) {
        logger().error("Failed to initialize frame converter.");
        return false;
    }

    frameEncoder_ = std::make_unique<WinFrameEncoder>(
        frameGrabber_.getDevice(),
        frameGrabber_.getContext());

    if (!frameEncoder_->Initialize(width_, height_, 60, 8'000'000)) {
        logger().error("Failed to initialize frame encoder.");
        return false;
    }

    fecPacketizer_ = std::make_unique<FecPacketizer>(MaxDatagramSize - sizeof(UDPStreamHeader));

    if (!packetSender_.Open(address, port)) {
        logger().error("Failed to open packet sender.");
        return false;
    }

    logger().info(QString("Streaming at %1:%2").arg(address).arg(port));

    return true;
}

void Producer::run(std::atomic<bool>& running)
{
    if (!frameConverter_ || !frameEncoder_ || !fecPacketizer_) {
        logger().error("Producer is not initialized.");
        return;
    }

    logger().info("Producer run() started.");

    EncodedFrame encoded;
    std::vector<Packet> packets;
    std::vector<Packet> fecPackets;

    while (running.load(std::memory_order_relaxed)) {
        VideoFrame bgraFrame{};

        if (!frameGrabber_.CaptureFrame(bgraFrame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        ConversionParams params{};
        params.inputNativeResource = bgraFrame;
        params.outputNativeResource = nv12Frame_;

        if (frameConverter_->Convert(params)) {
            nv12Frame_.timestamp = now100ns();

            frameEncoder_->SubmitFrame(nv12Frame_);

            // yes, a very crude way of doing that. It's temporary.
            std::this_thread::sleep_for(std::chrono::milliseconds(8));

            if (frameEncoder_->ReceiveFrame(encoded)) {

                packetizer_.Packetize(encoded, packets);
                fecPacketizer_->Packetize(packets, fecPackets);

                for (const auto& packet : fecPackets) {
                    packetSender_.Send(packet.bytes);
                }
            }
        }

        frameGrabber_.ReleaseFrame();
    }

    logger().info("Streaming halted.\n");
}