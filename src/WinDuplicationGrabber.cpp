#include "WinDuplicationGrabber.hpp"

#include <iostream> 

bool WinDuplicationGrabber::Initialize()
{
	HRESULT hr;
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
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
		std::cerr << "D3D11CreateDevice failed\n";
		return false;
	}

	Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
	device_.As(&dxgiDevice);
	
	Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
	dxgiDevice->GetAdapter(&adapter);

	Microsoft::WRL::ComPtr<IDXGIOutput> output;
	hr = adapter->EnumOutputs(0, &output);

	if (FAILED(hr)) {
		std::cerr << "EnumOutputs failed\n";
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
		std::cerr << "DuplicateOutput failed\n";
		return false;
	}

	std::cout << "Desktop duplication initialized: " << width << "x" << height << std::endl;

	return true;
}

bool WinDuplicationGrabber::CaptureFrame(FrameData& outFrame)
{
	DXGI_OUTDUPL_FRAME_INFO frameInfo{};

	Microsoft::WRL::ComPtr<IDXGIResource> resource;

	HRESULT hr = duplication_->AcquireNextFrame(1000, &frameInfo, &resource);

	if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;
	if (FAILED(hr)) {
		std::cerr << "AcquireNextFrame failed\n";
		return false;
	}

	hr = resource.As(&acquiredTexture_);

	if (FAILED(hr)) {
		duplication_->ReleaseFrame();
		return false;
	}

	D3D11_TEXTURE2D_DESC desc{};
	acquiredTexture_->GetDesc(&desc);

	outFrame.nativeTextureHandle = acquiredTexture_.Get();
	outFrame.width = desc.Width;
	outFrame.height = desc.Height;
}

void WinDuplicationGrabber::ReleaseFrame()
{
	acquiredTexture_.Reset();

	if (duplication_)
		duplication_->ReleaseFrame();
}

