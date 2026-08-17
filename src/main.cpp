#include <cstdint>
#include <iostream>

#ifdef _WIN32
#include "application/Producer.hpp"
#else
#include "application/Consumer.hpp"
#endif

int main()
{
#ifdef _WIN32
    constexpr const char* destinationAddress = "192.168.0.100";
    constexpr uint16_t destinationPort = 5000;

    Producer app;

    if (!app.initialize(destinationAddress, destinationPort)) {
        std::cerr << "Failed to initialize Producer.\n";
        return 1;
    }

    app.run();

#else
    constexpr uint16_t listenPort = 5000;

    Consumer app;

    if (!app.initialize(listenPort)) {
        std::cerr << "Failed to initialize Consumer.\n";
        return 1;
    }

    app.run();

#endif

    return 0;
}