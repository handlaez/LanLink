#include "WinFrameGrabber.hpp"
#include "Logger.hpp"

bool WinFrameGrabber::Initialize()
{
	HRESULT hr;
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
	D3D_FEATURE_LEVEL featureLevel;

	hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&device_,
		&featureLevel,
		&context_);

	if (FAILED(hr)) {
		logger().error("D3D11CreateDevice failed");
		return false;
	}

	Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
	device_.As(&dxgiDevice);
	
	Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
	dxgiDevice->GetAdapter(&adapter);

	Microsoft::WRL::ComPtr<IDXGIOutput> output;
	hr = adapter->EnumOutputs(0, &output);

	if (FAILED(hr)) {
		logger().error("EnumOutputs failed");
		return false;
	}

	DXGI_OUTPUT_DESC desc;
	output->GetDesc(&desc);

	width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
	height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

	Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
	output.As(&output1);

	hr = output1->DuplicateOutput(device_.Get(), &duplication_);

	if (FAILED(hr)) {
		logger().error("DuplicateOutput failed");
		return false;
	}

	logger().info(QString("Desktop duplication initialized: %1 x %2").arg(width).arg(height));

	return true;
}

bool WinFrameGrabber::CaptureFrame(VideoFrame& outFrame)
{
	DXGI_OUTDUPL_FRAME_INFO frameInfo{};

	Microsoft::WRL::ComPtr<IDXGIResource> resource;

	HRESULT hr = duplication_->AcquireNextFrame(1000, &frameInfo, &resource);

	if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;

	if (frameInfo.LastPresentTime.QuadPart == 0) {
		duplication_->ReleaseFrame();
		return false;
	}

	if (frameInfo.ProtectedContentMaskedOut) {
		logger().warn("DRM detected -> captured screen might be black.");
	}

	if (FAILED(hr)) {
		logger().error("AcquireNextFrame failed");
		return false;
	}

	hr = resource.As(&acquiredTexture_);

	if (FAILED(hr)) {
		duplication_->ReleaseFrame();
		return false;
	}

	D3D11_TEXTURE2D_DESC desc{};
	acquiredTexture_->GetDesc(&desc);

	outFrame.nativeResource = acquiredTexture_.Get();
	outFrame.width = desc.Width;
	outFrame.height = desc.Height;

	return true;
}

void WinFrameGrabber::ReleaseFrame()
{
	acquiredTexture_.Reset();

	if (duplication_)
		duplication_->ReleaseFrame();
}

ID3D11Device* WinFrameGrabber::getDevice() const
{
	return device_.Get();
}

ID3D11DeviceContext* WinFrameGrabber::getContext() const
{
	return context_.Get();
}