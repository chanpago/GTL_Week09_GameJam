#include "pch.h"
#include "Render/RenderPass/Public/FXAAPass.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/Renderer/Public/DeviceResources.h"
#include "Manager/UI/Public/ViewportManager.h"

struct FFullscreenVertex
{
    FVector2 Position;
    FVector2 UV;
};

FFXAAPass::FFXAAPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources, ID3D11VertexShader* InVS,
    ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11SamplerState* InSampler)
    :FRenderPass(InPipeline,nullptr,nullptr)
    , DeviceResources(InDeviceResources)
    , VertexShader(InVS)
    , PixelShader(InPS)
    , InputLayout(InLayout)
    , SamplerState(InSampler)
{
    InitializeFullscreenQuad();
    FXAAConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FFXAAConstants>();
}

FFXAAPass::~FFXAAPass()
{
    Release();
}

void FFXAAPass::Execute(FRenderingContext& Context)
{
    // 입력 텍스처: InputSRV가 설정되어 있으면 사용, 없으면 SceneColor 사용
    ID3D11ShaderResourceView* SceneSRV = InputSRV ? InputSRV : DeviceResources->GetSceneColorShaderResourceView();
    if (!SceneSRV)
    {
        return;
    }

    UpdateConstants();
    SetRenderTargets();

    FPipelineInfo PipelineInfo = {};
    PipelineInfo.InputLayout = InputLayout;
    PipelineInfo.VertexShader = VertexShader;
    PipelineInfo.PixelShader = PixelShader;
    PipelineInfo.DepthStencilState = nullptr;
    PipelineInfo.BlendState = nullptr;
    PipelineInfo.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    Pipeline->UpdatePipeline(PipelineInfo);

    UINT offset = 0;
    Pipeline->SetVertexBuffer(FullscreenVB, FullscreenStride);
    Pipeline->SetIndexBuffer(FullscreenIB, 0);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, FXAAConstantBuffer);
    Pipeline->SetShaderResourceView(0, EShaderType::PS, SceneSRV);
    Pipeline->SetSamplerState(0, EShaderType::PS, SamplerState);

    Pipeline->DrawIndexed(FullscreenIndexCount, 0, 0);

    // 정리
    Pipeline->SetShaderResourceView(0, EShaderType::PS, nullptr);
}

void FFXAAPass::Release()
{
    SafeRelease(FullscreenVB);
    SafeRelease(FullscreenIB);
    SafeRelease(FXAAConstantBuffer);
}

void FFXAAPass::InitializeFullscreenQuad()
{
    static const FFullscreenVertex Vertices[] =
    {
        {{-1.f,  1.f}, {0.f, 0.f}},
        {{ 1.f,  1.f}, {1.f, 0.f}},
        {{ 1.f, -1.f}, {1.f, 1.f}},
        {{-1.f, -1.f}, {0.f, 1.f}},
    };

    static const uint32 Indices[] = { 0, 1, 2, 0, 2, 3 };

    FullscreenStride = sizeof(FFullscreenVertex);
    FullscreenIndexCount = static_cast<UINT>(sizeof(Indices) / sizeof(Indices[0]));

    D3D11_BUFFER_DESC VBDesc = {};
    VBDesc.ByteWidth = sizeof(Vertices);
    VBDesc.Usage = D3D11_USAGE_IMMUTABLE;
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA VBData = {};
    VBData.pSysMem = Vertices;

    DeviceResources->GetDevice()->CreateBuffer(&VBDesc, &VBData, &FullscreenVB);

    D3D11_BUFFER_DESC IBDesc = {};
    IBDesc.ByteWidth = sizeof(Indices);
    IBDesc.Usage = D3D11_USAGE_IMMUTABLE;
    IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA IBData = {};
    IBData.pSysMem = Indices;

    DeviceResources->GetDevice()->CreateBuffer(&IBDesc, &IBData, &FullscreenIB);
}

void FFXAAPass::UpdateConstants()
{
    // ActiveViewportRect 크기를 기반으로 FXAA 해상도 계산
    FRect ActiveRect = UViewportManager::GetInstance().GetActiveViewportRect();
    FXAAParams.InvResolution = FVector2(1.0f / static_cast<float>(ActiveRect.Width),
                                         1.0f / static_cast<float>(ActiveRect.Height));

    // FXAA 품질 설정값을 명시적으로 업데이트
    FXAAParams.FXAASpanMax = 8.0f;
    FXAAParams.FXAAReduceMul = 1.0f / 8.0f;
    FXAAParams.FXAAReduceMin = 1.0f / 128.0f;

    // SceneColor 텍스처 UV 매핑 계산
    const float BackbufferWidth = static_cast<float>(DeviceResources->GetWidth());
    const float BackbufferHeight = static_cast<float>(DeviceResources->GetHeight());

    FXAAParams.ViewportUVOffsetX = static_cast<float>(ActiveRect.Left) / BackbufferWidth;
    FXAAParams.ViewportUVOffsetY = static_cast<float>(ActiveRect.Top) / BackbufferHeight;
    FXAAParams.ViewportUVScaleX = static_cast<float>(ActiveRect.Width) / BackbufferWidth;
    FXAAParams.ViewportUVScaleY = static_cast<float>(ActiveRect.Height) / BackbufferHeight;

    FRenderResourceFactory::UpdateConstantBufferData(FXAAConstantBuffer, FXAAParams);
}

void FFXAAPass::SetRenderTargets()
{
    // OutputRTV가 설정되어 있으면 사용, 없으면 백버퍼 사용
    ID3D11RenderTargetView* RTV = OutputRTV
        ? OutputRTV
        : DeviceResources->GetRenderTargetView(); // 스왑체인 백버퍼

    DeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &RTV, nullptr);

    // 뷰포트 설정: ActiveViewportRect 기반 (위젯 제외 영역에만 출력)
    FRect ActiveRect = UViewportManager::GetInstance().GetActiveViewportRect();
    D3D11_VIEWPORT Viewport = {};
    Viewport.TopLeftX = static_cast<float>(ActiveRect.Left);
    Viewport.TopLeftY = static_cast<float>(ActiveRect.Top);
    Viewport.Width = static_cast<float>(ActiveRect.Width);
    Viewport.Height = static_cast<float>(ActiveRect.Height);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;

    DeviceResources->GetDeviceContext()->RSSetViewports(1, &Viewport);
}
