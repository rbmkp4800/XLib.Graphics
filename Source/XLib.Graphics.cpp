#include <d3d11.h>
#include <dxgi1_3.h>

#include <XLib.Util.h>
#include <XLib.Platform.D3D11.Helpers.h>
#include <XLib.Platform.DXGI.Helpers.h>

#include "XLib.Graphics.h"
#include "XLib.Graphics.Internal.Shaders.h"

using namespace XLib;
using namespace XLib::Platform;
using namespace XLib::Graphics;
using namespace XLib::Graphics::Internal;

static_assert(
	uint32(PrimitiveType::Points) == D3D11_PRIMITIVE_TOPOLOGY_POINTLIST &&
	uint32(PrimitiveType::LineList) == D3D11_PRIMITIVE_TOPOLOGY_LINELIST &&
	uint32(PrimitiveType::LineStrip) == D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP &&
	uint32(PrimitiveType::TriangleList) == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST &&
	uint32(PrimitiveType::TriangleStrip) == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	"invalid PrimitiveType value");

COMPtr<IDXGIFactory3> Device::dxgiFactory;

struct VSConstants
{
	float32x4 tranfromRow0;
	float32x4 tranfromRow1;

	inline void set(const Matrix2x3& transform)
	{
		tranfromRow0 = { transform[0][0], transform[0][1], transform[0][2], 0.0f };
		tranfromRow1 = { transform[1][0], transform[1][1], transform[1][2], 0.0f };
	}
};

bool Device::initialize()
{
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL(0);
	D3D_FEATURE_LEVEL requestedFeatureLevel = D3D_FEATURE_LEVEL_10_0;

	uint32 deviceCreationFlags = 0;
#ifdef _DEBUG
	deviceCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		deviceCreationFlags, &requestedFeatureLevel, 1, D3D11_SDK_VERSION,
		d3dDevice.initRef(), &featureLevel, d3dContext.initRef());

	if (!dxgiFactory.isInitialized())
		CreateDXGIFactory1(dxgiFactory.uuid(), dxgiFactory.voidInitRef());

	D3D11_INPUT_ELEMENT_DESC d3dVertexColor2DILDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	d3dDevice->CreateInputLayout(d3dVertexColor2DILDesc, countof(d3dVertexColor2DILDesc),
		Shaders::Color2DVS.data, Shaders::Color2DVS.size, d3dColor2DIL.initRef());
	d3dDevice->CreateVertexShader(Shaders::Color2DVS.data, Shaders::Color2DVS.size, nullptr, d3dColor2DVS.initRef());
	d3dDevice->CreatePixelShader(Shaders::ColorPS.data, Shaders::ColorPS.size, nullptr, d3dColorPS.initRef());

	d3dDevice->CreateBuffer(&D3D11BufferDesc(sizeof(VSConstants), D3D11_BIND_CONSTANT_BUFFER), nullptr, d3dConstantBuffer.initRef());

	return true;
}

void Device::clear(RenderTarget& renderTarget, Color color)
{
	d3dContext->ClearRenderTargetView(renderTarget.d3dRTV, to<float32*>(&color.toF32x4()));
}

void Device::setRenderTarget(RenderTarget& renderTarget)
{
	ID3D11RenderTargetView *d3dRTV = renderTarget.d3dRTV;
	d3dContext->OMSetRenderTargets(1, &d3dRTV, nullptr);
	d3dContext->RSSetViewports(1, &D3D11ViewPort(0.0f, 0.0f,
		float32(renderTarget.getWidth()), float32(renderTarget.getHeight())));
}

void Device::setTransform2D(const Matrix2x3& transform)
{
	VSConstants constants;
	constants.set(transform);
	d3dContext->UpdateSubresource(d3dConstantBuffer, 0, nullptr, &constants, 0, 0);
}

void Device::updateBuffer(Buffer& buffer, const void* data, uint32 baseOffset, uint32 size)
{
	d3dContext->UpdateSubresource(buffer.d3dBuffer, 0,
		&D3D11Box(baseOffset, baseOffset + size), data, 0, 0);
}

void Device::draw2D(PrimitiveType primitiveType, Effect effect, Buffer& vertexBuffer,
	uint32 baseOffset, uint32 vertexStride, uint32 vertexCount)
{
	d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY(primitiveType));

	{
		ID3D11Buffer *d3dBuffer = vertexBuffer.d3dBuffer;
		UINT stride = vertexStride;
		UINT offset = baseOffset;
		d3dContext->IASetVertexBuffers(0, 1, &d3dBuffer, &stride, &offset);
	}

	ID3D11InputLayout *d3dIL = nullptr;
	ID3D11VertexShader *d3dVS = nullptr;
	ID3D11PixelShader *d3dPS = nullptr;

	switch (effect)
	{
		case Effect::PerVertexColor:
			d3dIL = d3dColor2DIL;
			d3dVS = d3dColor2DVS;
			d3dPS = d3dColorPS;
			break;

		//case Effect::GlobalColor:
			//break;
		default:
			return;
	}

	d3dContext->IASetInputLayout(d3dIL);
	d3dContext->VSSetShader(d3dVS, nullptr, 0);
	d3dContext->PSSetShader(d3dPS, nullptr, 0);
	ID3D11Buffer *d3dVSCB = d3dConstantBuffer;
	d3dContext->VSSetConstantBuffers(0, 1, &d3dVSCB);

	d3dContext->Draw(vertexCount, 0);
}

// Buffer ===================================================================================//

bool Buffer::initialize(ID3D11Device* d3dDevice, uint32 size, const void* initialData)
{
	d3dDevice->CreateBuffer(
		&D3D11BufferDesc(size, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER),
		initialData ? &D3D11SubresourceData(initialData, size, 0) : nullptr,
		d3dBuffer.initRef());

	return true;
}

// RenderTarget =============================================================================//

bool RenderTarget::initialize(ID3D11Device* d3dDevice,
	ID3D11Texture2D* d3dTexture, uint16 width, uint16 height)
{
	this->width = width;
	this->height = height;
	d3dDevice->CreateRenderTargetView(d3dTexture, nullptr, d3dRTV.initRef());

	return true;
}

// WindowRenderTarget =======================================================================//

bool WindowRenderTarget::initialize(ID3D11Device* d3dDevice,
	IDXGIFactory3* dxgiFactory, void* hWnd, uint16 width, uint16 height)
{
	dxgiFactory->CreateSwapChainForHwnd(d3dDevice, HWND(hWnd),
		&DXGISwapChainDesc1(width, height), nullptr, nullptr, dxgiSwapChain.initRef());

	COMPtr<ID3D11Texture2D> d3dBackTexture;
	dxgiSwapChain->GetBuffer(0, d3dBackTexture.uuid(), d3dBackTexture.voidInitRef());

	return RenderTarget::initialize(d3dDevice, d3dBackTexture, width, height);
}

bool WindowRenderTarget::resize(ID3D11Device* d3dDevice, uint16 width, uint16 height)
{
	RenderTarget::~RenderTarget();

	dxgiSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

	COMPtr<ID3D11Texture2D> d3dBackTexture;
	dxgiSwapChain->GetBuffer(0, d3dBackTexture.uuid(), d3dBackTexture.voidInitRef());
	return RenderTarget::initialize(d3dDevice, d3dBackTexture, width, height);
}

void WindowRenderTarget::present(bool sync)
{
	dxgiSwapChain->Present(sync ? 1 : 0, 0);
}