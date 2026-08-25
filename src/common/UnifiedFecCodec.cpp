#include "UnifiedFecCodec.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

extern "C" {
#include <fec.h>
}

struct FecCodec::Impl {
    fec_t* fec = nullptr;

    explicit Impl()
    {
        fec_init();

        fec = fec_new(
            static_cast<unsigned short>(FecCodec::DataShards),
            static_cast<unsigned short>(FecCodec::TotalShards)
        );

        if (fec == nullptr) {
            throw std::runtime_error("fec_new() failed");
        }
    }

    ~Impl()
    {
        if (fec != nullptr) {
            fec_free(fec);
        }
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
};

FecCodec::FecCodec(std::size_t shardSize) : impl_(nullptr), shardSize_(shardSize)
{
    if (shardSize == 0) {
        throw std::invalid_argument("FEC shard size must not be zero");
    }

    impl_ = new Impl();
}

FecCodec::~FecCodec()
{
    delete impl_;
}

FecCodec::FecCodec(FecCodec&& other) noexcept
    : impl_(std::exchange(other.impl_, nullptr)),
    shardSize_(std::exchange(other.shardSize_, 0))
{
}


FecCodec& FecCodec::operator=(FecCodec&& other) noexcept
{
    if (this != &other) {
        delete impl_;

        impl_ = std::exchange(other.impl_, nullptr);
        shardSize_ = std::exchange(other.shardSize_, 0);
    }

    return *this;
}

void FecCodec::encode(
    std::span<const std::span<const std::byte>, DataShards> dataShards,
    std::span<std::span<std::byte>, ParityShards> parityShards)
{
    for (const auto shard : dataShards) {
        if (shard.size() != shardSize_) {
            throw std::invalid_argument("FEC data shard has incorrect size.");
        }
    }

    for (const auto shard : parityShards) {
        if (shard.size() != shardSize_) {
            throw std::invalid_argument(
                "FEC parity shard has incorrect size.");
        }
    }

    std::array<const unsigned char*, DataShards> data{};
    std::array<unsigned char*, ParityShards> parity{};

    for (std::size_t i = 0; i < DataShards; ++i) {
        data[i] = reinterpret_cast<const unsigned char*>(dataShards[i].data());
    }

    for (std::size_t i = 0; i < ParityShards; ++i) {
        parity[i] = reinterpret_cast<unsigned char*>(parityShards[i].data());
    }

    constexpr std::array<unsigned int, ParityShards> parityIndices{
        DataShards,
        DataShards + 1
    };

    fec_encode(impl_->fec, data.data(), parity.data(), parityIndices.data(), ParityShards, shardSize_);
}

bool FecCodec::decode(
    std::span<std::span<const std::byte>, DataShards> shards,
    std::span<const std::uint8_t, DataShards> shardIndices,
    std::span<std::span<std::byte>> outputShards)
{
    if (outputShards.empty()) {
        return true;
    }

    std::array<const unsigned char*, DataShards> input{};
    std::array<unsigned int, DataShards> indices{};

    for (std::size_t i = 0; i < DataShards; ++i) {
        if (shards[i].size() != shardSize_) {
            return false;
        }

        if (shardIndices[i] >= TotalShards) {
            return false;
        }

        input[i] = reinterpret_cast<const unsigned char*>(shards[i].data());
        indices[i] = shardIndices[i];
    }

    for (const auto shard : outputShards) {
        if (shard.size() != shardSize_) {
            return false;
        }
    }

    // outputShards must correspond to the missing DATA shards.
    std::array<unsigned char*, DataShards> output{};

    std::size_t outputIndex = 0;

    for (std::size_t dataIndex = 0; dataIndex < DataShards; ++dataIndex)
    {
        const bool present = std::ranges::any_of(shardIndices, [dataIndex](const auto index) 
            {
                return index == dataIndex;
            }
        );

        if (!present) {
            if (outputIndex >= outputShards.size()) 
            {
                return false;
            }

            auto* destination = reinterpret_cast<unsigned char*>(outputShards[outputIndex].data());

            output[outputIndex] = destination;
            ++outputIndex;
        }
    }

    if (outputIndex != outputShards.size()) {
        return false;
    }

    fec_decode(impl_->fec, input.data(), output.data(), indices.data(), shardSize_);

    return true;
}