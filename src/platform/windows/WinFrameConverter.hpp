#ifndef WIN_FRAME_CONVERTER_HPP
#define WIN_FRAME_CONVERTER_HPP

#include <d3d11.h>
#include <wrl/client.h>
#include <stdexcept>
#include <unordered_map>

#include "common/IFrameConverter.hpp"

class FrameConverter final : public IFrameConverter {
public:
	explicit FrameConverter(ID3D11Device* device, ID3D11DeviceContext* context);
	~FrameConverter() override = default;

	FrameConverter(const FrameConverter&) = delete;
	FrameConverter& operator=(const FrameConverter&) = delete;

	bool Initialize(uint32_t width, uint32_t height) override;
	bool Convert(const VideoFrame& input, VideoFrame& output) override;

private:
	void CreateComputeShader();
	void CreateOutputTexture();
	void RecreateViewsForOutput(ID3D11Texture2D* outputNv12);

	Microsoft::WRL::ComPtr<ID3D11Device> device_;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> computeShader_;

	std::unordered_map<ID3D11Texture2D*, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> srvCache_;

	// Converter-owned output.
	Microsoft::WRL::ComPtr<ID3D11Texture2D> outputTexture_;
	VideoFrame outputFrame_{};

	// cache views for the current output texture
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uavY_;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uavUV_;

	uint32_t width_ = 0;
	uint32_t height_ = 0;
	bool initialized_ = false;
};

#endif // !WIN_FRAME_CONVERTER_HPP
