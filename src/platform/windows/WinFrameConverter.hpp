#ifndef WIN_FRAME_CONVERTER_HPP
#define WIN_FRAME_CONVERTER_HPP

#include <d3d11.h>
#include <wrl/client.h>
#include <stdexcept>
#include <unordered_map>

#include "common/IFrameConverter.hpp"

class WinFrameConverter final : public IFrameConverter {
public:
	explicit WinFrameConverter(ID3D11Device* device, ID3D11DeviceContext* context);
	~WinFrameConverter() override = default;

	WinFrameConverter(const WinFrameConverter&) = delete;
	WinFrameConverter& operator=(const WinFrameConverter&) = delete;

	bool Initialize(uint32_t width, uint32_t height) override;
	bool ConvertBgraToNv12(const ConversionParams& params) override;

private:
	void CreateComputeShader();
	void RecreateViewsForOutput(ID3D11Texture2D* outputNv12);

	Microsoft::WRL::ComPtr<ID3D11Device> m_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_computeShader;

	std::unordered_map<ID3D11Texture2D*, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_srvCache;

	// cache views for the current output texture
	ID3D11Texture2D* m_lastOutputTexture = nullptr;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_uavY;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_uavUV;

	uint32_t m_width = 0;
	uint32_t m_height = 0;
	bool m_initialized = false;
};

#endif // !WIN_FRAME_CONVERTER_HPP
