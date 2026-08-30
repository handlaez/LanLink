#include "UnifiedFecCodec.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

extern "C" {

#ifdef __cplusplus
#define restrict __restrict__
#endif

#include <fec.h>

#ifdef __cplusplus
#undef restrict
#endif

}

struct FecCodec::Impl {
    fec_t* fec = nullptr;

    Impl(std::size_t dataShards, std::size_t parityShards)
    {
        fec_init();

        const auto k = static_cast<unsigned short>(dataShards);
        const auto n = static_cast<unsigned short>(dataShards + parityShards);

        fec = fec_new(k, n);

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

FecCodec::FecCodec(std::size_t shardSize, std::size_t dataShards, std::size_t parityShards)
    : impl_(nullptr), shardSize_(shardSize), dataShards_(dataShards), parityShards_(parityShards)
{
    if (shardSize_ == 0) {
        throw std::invalid_argument("FEC shard size must not be zero.");
    }

    if (dataShards_ == 0) {
        throw std::invalid_argument("FEC data shard count must not be zero.");
    }

    if (parityShards_ == 0) {
        throw std::invalid_argument("FEC parity shard count must not be zero.");
    }

    const std::size_t totalShards = dataShards_ + parityShards_;

    // fec_new() takes unsigned short counts.
    if (totalShards > 255) {
        throw std::invalid_argument("FEC shard count exceeds supported limit.");
    }

    impl_ = new Impl(dataShards_, parityShards_);
}

FecCodec::~FecCodec()
{
    delete impl_;
}

FecCodec::FecCodec(FecCodec&& other) noexcept
    : impl_(std::exchange(other.impl_, nullptr)),
    shardSize_(std::exchange(other.shardSize_, 0)),
    dataShards_(std::exchange(other.dataShards_, 0)),
    parityShards_(std::exchange(other.parityShards_, 0))
{
}

FecCodec& FecCodec::operator=(FecCodec&& other) noexcept
{
    if (this != &other) {
        delete impl_;

        impl_ = std::exchange(other.impl_, nullptr);
        shardSize_ = std::exchange(other.shardSize_, 0);
        dataShards_ = std::exchange(other.dataShards_, 0);
        parityShards_ = std::exchange(other.parityShards_, 0);
    }

    return *this;
}

void FecCodec::encode(
    std::span<const std::span<const std::byte>> dataShards,
    std::span<std::span<std::byte>> parityShards)
{
    if (dataShards.size() != dataShards_) {
        throw std::invalid_argument("Incorrect number of FEC data shards.");
    }

    if (parityShards.size() != parityShards_) {
        throw std::invalid_argument("Incorrect number of FEC parity shards.");
    }

    for (const auto shard : dataShards) {
        if (shard.size() != shardSize_) {
            throw std::invalid_argument("FEC data shard has incorrect size.");
        }
    }

    for (const auto shard : parityShards) {
        if (shard.size() != shardSize_) {
            throw std::invalid_argument("FEC parity shard has incorrect size.");
        }
    }

    std::vector<const unsigned char*> data(dataShards_);
    std::vector<unsigned char*> parity(parityShards_);
    std::vector<unsigned int> parityIndices(parityShards_);

    for (std::size_t i = 0; i < dataShards_; ++i) {
        data[i] = reinterpret_cast<const unsigned char*>(dataShards[i].data());
    }

    for (std::size_t i = 0; i < parityShards_; ++i) {
        parity[i] = reinterpret_cast<unsigned char*>(parityShards[i].data());

        parityIndices[i] = static_cast<unsigned int>(dataShards_ + i);
    }

    fec_encode(impl_->fec, data.data(), parity.data(), parityIndices.data(), static_cast<unsigned int>(parityShards_), static_cast<unsigned long>(shardSize_));
}

bool FecCodec::decode(
    std::span<const std::span<const std::byte>> shards,
    std::span<const std::uint8_t> shardIndices,
    std::span<std::span<std::byte>> outputShards)
{
    if (shards.size() != dataShards_) {
        return false;
    }

    if (shardIndices.size() != dataShards_) {
        return false;
    }

    if (outputShards.empty()) {
        return true;
    }

    for (const auto shard : shards) {
        if (shard.size() != shardSize_) {
            return false;
        }
    }

    for (const auto shard : outputShards) {
        if (shard.size() != shardSize_) {
            return false;
        }
    }

    const std::size_t totalShards = dataShards_ + parityShards_;

    std::vector<const unsigned char*> input(dataShards_);
    std::vector<unsigned int> indices(dataShards_);

    for (std::size_t i = 0; i < dataShards_; ++i) {
        if (shardIndices[i] >= totalShards) {
            return false;
        }

        input[i] = reinterpret_cast<const unsigned char*>(shards[i].data());
        indices[i] = static_cast<unsigned int>(shardIndices[i]);
    }

    std::vector<bool> present(dataShards_, false);

    for (const auto index : shardIndices) {
        if (index < dataShards_) {
            present[index] = true;
        }
    }

    std::size_t missingCount = 0;

    for (std::size_t i = 0; i < dataShards_; ++i) {
        if (!present[i]) {
            ++missingCount;
        }
    }

    if (missingCount != outputShards.size()) {
        return false;
    }

    std::vector<unsigned char*> output(outputShards.size());

    std::size_t outputIndex = 0;

    for (std::size_t i = 0; i < dataShards_; ++i) {
        if (!present[i]) {
            output[outputIndex] = reinterpret_cast<unsigned char*>(outputShards[outputIndex].data());
            ++outputIndex;
        }
    }

    fec_decode(impl_->fec, input.data(), output.data(), indices.data(), static_cast<unsigned long>(shardSize_));

    return true;
}