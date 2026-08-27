# LanLink

A work-in-progress C++20 application for streaming a computer's screen over a local network (LAN) to other devices running the same application.

The main goal is to allow one device to act as the streamer while other devices on the network receive and display the streamed screen.

## Current State

This project is very much a work in progress.

Currently:

* Windows can stream its screen.
* Linux devices can receive the stream.
* The application has a Qt 6-based UI.
* The project is written in C++20.
* Streaming is currently intended for devices connected to the same LAN.
* Windows -> Linux streaming is currently the main working setup.

For example, the current setup can stream a Windows PC's screen to an Arduino/Linux-based device running the application.

## Planned

There is still a lot to do.

Some planned features and improvements include:

* Support for streaming between more device types.
* Better performance and lower latency.
* Improved video/image compression.
* More robust network communication.
* Improved Qt 6 UI.
* Configuration options for streaming quality and other parameters.
* Better Linux support.

This list will change as development continues.

## Technologies

* C++20
* Qt 6
* LAN/network streaming
* Windows
* Linux

## Building

Build instructions are currently not finalized.

More information will be added as the project develops.

## Project Status

This project should currently be considered experimental.

Things may be broken, incomplete, or change without notice. APIs, networking protocols, project structure, and UI are all subject to change.

If you are looking at this project and wondering whether it is production-ready:

No. Not yet.

## License

License information will be added later.
