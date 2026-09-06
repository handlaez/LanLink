#include "Logger.hpp"
#include "Producer.hpp"

#include <chrono>
#include <ratio>
#include <thread>
#include <vector>

namespace {
    uint64_t now100ns()
    {
        using namespace std::chrono;

        return duration_cast<duration<uint64_t, std::ratio<1, 10'000'000>>>(steady_clock::now().time_since_epoch()).count();
    }
} // namespace

bool Producer::initialize(const std::string& address, uint16_t port)
{
    if (!frameGrabber_.Initialize()) {
        logger().error("Failed to initialize frame grabber.");
        return false;
    }

    if (!frameGrabber_.GetFrameDimensions(width_, height_)) {
        logger().error("Failed to determine frame dimensions.");
        return false;
    }

    logger().info(QString("Width: %1 Height: %2").arg(width_).arg(height_));

#ifdef _WIN32
    frameConverter_ = std::make_unique<FrameConverter>(
        frameGrabber_.getDevice(),
        frameGrabber_.getContext()
    );
#else
    frameConverter_ = std::make_unique<FrameConverter>();
#endif

    if (!frameConverter_->Initialize(width_, height_)) {
        logger().error("Failed to initialize frame converter.");
        return false;
    }

#ifdef _WIN32
    frameEncoder_ = std::make_unique<FrameEncoder>(
        frameGrabber_.getDevice(),
        frameGrabber_.getContext()
    );
#else
    frameEncoder_ = std::make_unique<FrameEncoder>();
#endif

    if (!frameEncoder_->Initialize(width_, height_, 60, 8'000'000)) {
        logger().error("Failed to initialize frame encoder.");
        return false;
    }

    fecPacketizer_ = std::make_unique<FecPacketizer>(MaxDatagramSize - sizeof(UDPStreamHeader));

    if (!packetSender_.Open(address, port)) {
        logger().error("Failed to open packet sender.");
        return false;
    }

    logger().info(QString("Streaming at %1:%2").arg(QString::fromStdString(address)).arg(port));

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
        VideoFrame capturedFrame{};

        if (!frameGrabber_.CaptureFrame(capturedFrame)) {
            // giving the grabber some time to grab the frame if no frame is ready yet.
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        VideoFrame convertedFrame{};
        convertedFrame.timestamp = now100ns();

        if (!frameConverter_->Convert(capturedFrame, convertedFrame)) {
            logger().error("Failed to convert frame.");
        }
        else {

            if (!frameEncoder_->SubmitFrame(convertedFrame)) {
                logger().error("Failed to submit frame to encoder.");
            }
            else {
#ifdef _WIN32
                // temporary synchronization for the asynchronous hardware encoder.
                // Also limits the submission rate.
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
#endif

                while (frameEncoder_->ReceiveFrame(encoded)) {
                    packets.clear();
                    fecPackets.clear();

                    packetizer_.Packetize(encoded, packets);
                    fecPacketizer_->Packetize(packets, fecPackets);

                    for (const auto& packet : fecPackets) {
                        packetSender_.Send(packet.bytes);
                    }
                }
            }
        }

        frameGrabber_.ReleaseFrame();
    }

    logger().info("Streaming halted.\n");
}
