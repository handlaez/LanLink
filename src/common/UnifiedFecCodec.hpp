#ifndef UNIFIED_FEC_CODEC_HPP
#define UNIFIED_FEC_CODEC_HPP

#include <cstddef>
#include <cstdint>
#include <span>

class FecCodec {
public:
    FecCodec(std::size_t shardSize, std::size_t dataShards, std::size_t parityShards);

    ~FecCodec();

    FecCodec(const FecCodec&) = delete;
    FecCodec& operator=(const FecCodec&) = delete;

    FecCodec(FecCodec&&) noexcept;
    FecCodec& operator=(FecCodec&&) noexcept;

    /**
     * Generate parity shards from the data shards.
       
       dataShards.size() must equal dataShards().
       parityShards.size() must equal parityShards().
     
     * Every shard must contain exactly shardSize() bytes.
     */
    void encode(
        std::span<const std::span<const std::byte>> dataShards,
        std::span<std::span<std::byte>> parityShards);

    /**
     * Reconstruct missing data shards.
      
     * `shards` contains the available shards.
     * `shardIndices[i]` specifies the original index of shards[i].

       outputShards must contain one buffer for every missing data shard, in ascending data-shard index order.
      
     * Returns false if too few valid shards are supplied.
     */
    bool decode(
        std::span<const std::span<const std::byte>> shards,
        std::span<const std::uint8_t> shardIndices,
        std::span<std::span<std::byte>> outputShards);

    [[nodiscard]]
    std::size_t shardSize() const noexcept {
        return shardSize_;
    }

    [[nodiscard]]
    std::size_t dataShards() const noexcept {
        return dataShards_;
    }

    [[nodiscard]]
    std::size_t parityShards() const noexcept {
        return parityShards_;
    }

    [[nodiscard]]
    std::size_t totalShards() const noexcept {
        return dataShards_ + parityShards_;
    }

private:
    struct Impl;
    Impl* impl_ = nullptr;

    std::size_t shardSize_;
    std::size_t dataShards_;
    std::size_t parityShards_;
};

#endif // UNIFIED_FEC_CODEC_HPP