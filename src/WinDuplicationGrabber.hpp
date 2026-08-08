#ifndef WIN_DUPLICATION_GRABBER_H
#define WIN_DUPLICATION_GRABBER_H

#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include "IFrameGrabber.hpp"

class WinDuplicationGrabber : public IFrameGrabber {
public: 
	bool Initialize() override;
	bool CaptureFrame(FrameData& outFrame) override;
	void ReleaseFrame() override;

	ID3D11Device* getDevice() const;
	ID3D11DeviceContext* getContext() const;

private:
	Microsoft::WRL::ComPtr<ID3D11Device> device_;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
	Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication_;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> acquiredTexture_;

	uint32_t width = 0;
	uint32_t height = 0;
};

#endif // 
