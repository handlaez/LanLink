#ifndef UNIFIED_FEC_CODEC_HPP
#define UNIFIED_FEC_CODEC_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

class FecCodec {
public:
    static constexpr std::size_t DataShards = 10;
    static constexpr std::size_t ParityShards = 2;
    static constexpr std::size_t TotalShards = DataShards + ParityShards;

    explicit FecCodec(std::size_t shardSize);
    ~FecCodec();

    FecCodec(const FecCodec&) = delete;
    FecCodec& operator=(const FecCodec&) = delete;

    FecCodec(FecCodec&&) noexcept;
    FecCodec& operator=(FecCodec&&) noexcept;

    /**
     * Generate the two parity shards from the ten data shards.
     * Every shard must have exactly shardSize() bytes.
     */
    void encode(
        std::span<const std::span<const std::byte>, DataShards> dataShards,
        std::span<std::span<std::byte>, ParityShards> parityShards
    );

    /**
     * Reconstruct missing data shards. Missing data shards are written to `outputShards`.
     *
     * outputShards must contain one buffer for every missing data shard,
     * in ascending data-shard index order.
     * 
     * Returns false if too few valid shards are supplied.
     */
    bool decode(
        std::span<std::span<const std::byte>, DataShards> shards,
        std::span<const std::uint8_t, DataShards> shardIndices,
        std::span<std::span<std::byte>> outputShards
    );

    [[nodiscard]]
    std::size_t shardSize() const noexcept {
        return shardSize_;
    }

private:
    struct Impl;
    Impl* impl_ = nullptr;

    std::size_t shardSize_;
};

#endif // !UNIFIED_FEC_CODEC_HPP
