#ifndef I_PACKET_SENDER_HPP
#define I_PACKET_SENDER_HPP

#include <cstdint>
#include <span>
#include <string>

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual bool Open(const std::string& host, uint16_t port) = 0;
    virtual bool Send(std::span<const uint8_t> bytes) = 0;
    virtual void Close() = 0;
};

#endif 