#pragma once

class UDeviceResources
{
public:
	UDeviceResources(HWND InWindowHandle);
	~UDeviceResources();

	void Create(HWND InWindowHandle);
	void Release();

	void CreateDeviceAndSwapChain(HWND InWindowHandle);
	void ReleaseDeviceAndSwapChain();
	void CreateFrameBuffer();
	void ReleaseFrameBuffer();
	void CreateNormalBuffer();
	void ReleaseNormalBuffer();
	void CreateDepthBuffer();
	void ReleaseDepthBuffer();

	// Scene Color Texture, rtv, srv
	void CreateSceneColorTarget();
	void ReleaseSceneColorTarget();

	// HitProxy Texture, rtv, srv
	void CreateHitProxyTarget();
	void ReleaseHitProxyTarget();


	// Direct2D/DirectWrite
	void CreateFactories();
	void ReleaseFactories();

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	IDXGISwapChain* GetSwapChain() const { return SwapChain; }
	ID3D11RenderTargetView* GetRenderTargetView() const { return FrameBufferRTV; }
	ID3D11RenderTargetView* GetNormalRenderTargetView() const { return NormalBufferRTV; }
	ID3D11DepthStencilView* GetDepthStencilView() const { return DepthStencilView; }

	ID3D11ShaderResourceView* GetSceneColorSRV() const { return FrameBufferSRV; }
	ID3D11ShaderResourceView* GetNormalSRV() const { return NormalBufferSRV; }
	ID3D11ShaderResourceView* GetDepthSRV() const { return DepthBufferSRV; }
	ID3D11ShaderResourceView* GetDepthStencilSRV() const { return DepthStencilSRV; }

	ID3D11RenderTargetView* GetSceneColorRenderTargetView() const {return SceneColorTextureRTV; }
	ID3D11ShaderResourceView* GetSceneColorShaderResourceView() const{return SceneColorTextureSRV; }
	ID3D11Texture2D* GetSceneColorTexture() const {return SceneColorTexture; }

	ID3D11RenderTargetView* GetHitProxyRenderTargetView() const {return HitProxyTextureRTV; }
	ID3D11ShaderResourceView* GetHitProxyShaderResourceView() const {return HitProxyTextureSRV; }
	ID3D11Texture2D* GetHitProxyTexture() const {return HitProxyTexture; }


	const D3D11_VIEWPORT& GetViewportInfo() const { return ViewportInfo; }
	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }
	void UpdateViewport(float InMenuBarHeight = 0.f);

	/**
	 * @brief 렌더 타겟 및 깊이 버퍼가 사용하는 총 메모리를 계산합니다.
	 * @return 바이트 단위 메모리 사용량
	 */
	uint64 GetTotalRenderTargetMemory() const;

	// Direct2D/DirectWrite factory getters
	IDWriteFactory* GetDWriteFactory() const { return DWriteFactory; }
	ID2D1Factory* GetD2DFactory() const { return D2DFactory; }
	ID2D1RenderTarget* GetD2DRenderTarget() const { return D2DRenderTarget; }

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;
	ID3D11ShaderResourceView* FrameBufferSRV = nullptr;
	
	/** 
	 * This is introduced to support post-process point light effects
	 * without modifying the existing forward rendering pipeline.
	 * 
	 * @note This variable is temporary and intended to be removed 
	 * once a full deferred lighting system is implemented.
	 */
	ID3D11Texture2D* NormalBuffer = nullptr;
	ID3D11RenderTargetView* NormalBufferRTV = nullptr;
	ID3D11ShaderResourceView* NormalBufferSRV = nullptr;

	ID3D11Texture2D* DepthBuffer = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11ShaderResourceView* DepthBufferSRV = nullptr;
	ID3D11ShaderResourceView* DepthStencilSRV = nullptr;

	ID3D11Texture2D* SceneColorTexture = nullptr;
	ID3D11RenderTargetView* SceneColorTextureRTV = nullptr;
	ID3D11ShaderResourceView* SceneColorTextureSRV = nullptr;

	ID3D11Texture2D* HitProxyTexture = nullptr;
	ID3D11RenderTargetView* HitProxyTextureRTV = nullptr;
	ID3D11ShaderResourceView* HitProxyTextureSRV = nullptr;

	D3D11_VIEWPORT ViewportInfo = {};

	uint32 Width = 0;
	uint32 Height = 0;

	// Direct2D/DirectWrite factories
	ID2D1Factory* D2DFactory = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;
	ID2D1RenderTarget* D2DRenderTarget = nullptr;
};
