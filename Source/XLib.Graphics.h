#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Color.h>
#include <XLib.Vectors.h>
#include <XLib.Platform.COMPtr.h>
#include <XLib.Math.Matrix2x3.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11InputLayout;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11Buffer;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;

struct IDXGIFactory3;
struct IDXGISwapChain1;

namespace XLib::Graphics
{
	enum class PrimitiveType : uint8
	{
		None = 0,
		Points = 1,
		LineList = 2,
		LineStrip = 3,
		TriangleList = 4,
		TriangleStrip = 5,
	};

	enum class Effect : uint8
	{
		None = 0,
		PerVertexColor = 1,
		GlobalColor = 2,
	};

	struct VertexBase2D
	{
		float32x2 position;
	};

	struct VertexBase3D
	{
		float32x2 position;
	};

	struct VertexColor2D
	{
		float32x2 position;
		uint32 color;
	};

	class Buffer : public XLib::NonCopyable
	{
		friend class Device;

	private:
		XLib::Platform::COMPtr<ID3D11Buffer> d3dBuffer;

		bool initialize(ID3D11Device* d3dDevice, uint32 size, const void* initialData);

	public:
		
	};

	class RenderTarget abstract : public XLib::NonCopyable
	{
		friend class Device;

	private:
		XLib::Platform::COMPtr<ID3D11RenderTargetView> d3dRTV;
		uint16 width = 0, height = 0;

	protected:
		bool initialize(ID3D11Device* d3dDevice,
			ID3D11Texture2D* d3dTexture, uint16 width, uint16 height);

	public:
		RenderTarget() = default;
		inline ~RenderTarget() { width = 0; height = 0; }

		inline uint16 getWidth() { return width; }
		inline uint16 getHeight() { return height; }
	};

	class WindowRenderTarget : public RenderTarget
	{
		friend class Device;

	private:
		XLib::Platform::COMPtr<IDXGISwapChain1> dxgiSwapChain;

		bool initialize(ID3D11Device* d3dDevice, IDXGIFactory3* dxgiFactory,
			void* hWnd, uint16 width, uint16 height);
		bool resize(ID3D11Device* d3dDevice, uint16 width, uint16 height);

	public:
		WindowRenderTarget() = default;
		~WindowRenderTarget() = default;

		void present(bool sync = true);
	};

	class Device : public XLib::NonCopyable
	{
	private:
		static XLib::Platform::COMPtr<IDXGIFactory3> dxgiFactory;

		XLib::Platform::COMPtr<ID3D11Device> d3dDevice;
		XLib::Platform::COMPtr<ID3D11DeviceContext> d3dContext;

		XLib::Platform::COMPtr<ID3D11InputLayout> d3dColor2DIL;
		XLib::Platform::COMPtr<ID3D11VertexShader> d3dColor2DVS;
		XLib::Platform::COMPtr<ID3D11PixelShader> d3dColorPS;

		XLib::Platform::COMPtr<ID3D11Buffer> d3dConstantBuffer;

	public:
		bool initialize();

		void clear(RenderTarget& renderTarget, Color color);
		void setRenderTarget(RenderTarget& renderTarget);
		void setTransform2D(const Matrix2x3& transform);
		void updateBuffer(Buffer& buffer, const void* data, uint32 baseOffset, uint32 size);
		void draw2D(PrimitiveType primitiveType, Effect effect, Buffer& vertexBuffer,
			uint32 baseOffset, uint32 vertexStride, uint32 vertexCount);
		void drawIndexed2D(PrimitiveType primitiveType, Effect effect, Buffer& vertexBuffer,
			uint32 baseOffset, uint32 vertexStride, uint32 vertexCount);

		inline bool createWindowRenderTarget(void* hWnd, uint16 width, uint16 height, WindowRenderTarget& renderTarget)
			{ return renderTarget.initialize(d3dDevice, dxgiFactory, hWnd, width, height); }
		inline bool createBuffer(uint32 size, const void* initialData, Buffer& buffer)
			{ return buffer.initialize(d3dDevice, size, initialData); }
	};
}