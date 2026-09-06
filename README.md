# LanLink
LanLink is a C++20 application for streaming a computer's screen over a local network (LAN) to another device running the same application.

<img width="492" height="683" alt="image" src="https://github.com/user-attachments/assets/edbfd277-6027-422a-8407-67ed5f4c9649" />

The project is intended to provide a simple cross-platform screen-streaming solution where one device acts as the producer and another acts as the receiver.

## MVP Status
LanLink has reached its first working MVP.

The current implementation supports:

* Windows screen capture and streaming.
* Linux screen capture and streaming.
* Windows receiving streams.
* Linux receiving streams.
* LAN-based UDP video transport.
* H.265 / HEVC video encoding.
* FEC (Forward Error Correction) for packet loss resilience.
* A Qt 6-based user interface.

The Linux producer currently uses CPU-based screen capture, pixel conversion, and H.265 encoding. Performance is therefore significantly lower than the Windows implementation and is still being optimized.

At the moment, the project is best considered a **functional MVP rather than a production-ready application**.

## Architecture
The streaming pipeline is designed around platform-independent interfaces with platform-specific implementations.

### Producer

```text
Screen Capture --> Frame Conversion --> H.265 / HEVC Encoding --> Packetization --> FEC --> UDP --> Network
```

Platform-specific components handle screen capture, pixel conversion, and video encoding while the packetization and transport layers are shared where possible.

### Windows

The Windows implementation uses GPU-based processing where available.

```text
Desktop Capture --> GPU Frame --> NV12 Conversion --> H.265 Encoder --> UDP
```

### Linux
The current Linux implementation uses X11 and CPU-based processing.

```text
X11 Capture --> Packed Pixel Data --> YUV420P Conversion --> Software H.265 / x265 --> UDP
```

The current Linux capture implementation is primarily an MVP implementation and is not yet optimized for high frame rates.

## Video
The current MVP uses:

* Codec: H.265 / HEVC
* Encoder: x265
* Pixel format: YUV420P
* Transport: UDP
* Error correction: FEC

H.264 support is planned for a later stage.

The current implementation is focused on establishing a reliable end-to-end streaming pipeline before optimizing performance and expanding codec support.

## Supported Platforms

### Windows

Windows currently supports both:

* Streaming
* Receiving

### Linux

Linux currently supports both:

* Streaming
* Receiving

The Linux producer currently targets X11/Xorg for screen capture.
The primary development target for Linux is currently Raspberry Pi hardware running a Linux desktop environment.

## Current Limitations

The MVP has several known limitations:

### Linux performance

Linux screen capture and video conversion are currently CPU-based.

At higher resolutions, this results in significantly lower frame rates than the Windows implementation.
Performance improvements are planned, including more efficient screen capture and pixel conversion.

### Configuration
Many streaming parameters are currently hard-coded, including values such as:

* Resolution
* Frame rate
* Bitrate
* Encoder configuration

A proper configuration system will be added later.

### Network
The current network implementation is designed primarily for devices on the same LAN.

Internet/WAN streaming is not currently a target of the MVP.

### UI

The Qt 6 UI is functional but still under development.

## Technologies

* C++20
* Qt 6
* CMake
* vcpkg
* FFmpeg
* x265
* X11/Xorg
* UDP
* Forward Error Correction (FEC)
* Windows
* Linux

## Building

The project uses CMake and vcpkg for dependency management.

### Requirements

At minimum:

* C++20-compatible compiler
* CMake
* Qt 6
* vcpkg
* FFmpeg
* x265 support through FFmpeg

Platform-specific dependencies are handled through the project's build configuration.

### Configure

The project uses the vcpkg toolchain:

```bash
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_OVERLAY_PORTS=ports
```

### Build

```bash
cmake --build build
```

On Windows, configure and build using the corresponding vcpkg installation and CMake generator.

## Project Status
LanLink is currently in the **MVP stage**.

The core streaming pipeline is functional, but the project is still under active development.

Expect:

* Performance issues
* Incomplete configuration
* Experimental APIs
* Changes to the networking protocol
* Changes to the project structure
* UI changes
* Platform-specific limitations

The MVP is primarily intended to validate the architecture and establish a working cross-platform streaming pipeline.

It should **not** be considered production-ready.

## Planned

Future development may include:

* Improved Linux capture performance.
* Audio streaming.
* Hardware-accelerated encoding where available.
* Hardware-accelerated pixel conversion.
* H.264 support.
* Improved H.265 encoding performance and configuration.
* Improved bitrate and quality control.
* Better packet loss handling.
* More robust connection management.
* Stream discovery on the local network.
* Configurable streaming parameters.
* Improved Qt 6 UI.
* Better platform abstraction.
* Additional Linux desktop environments.
* Support for additional operating systems and devices.
* Improved documentation and installation instructions.

This list is not final and will change as development continues.

## License

License information will be added later.
