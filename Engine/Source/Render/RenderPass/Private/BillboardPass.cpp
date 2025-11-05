#include "pch.h"
#include "Render/RenderPass/Public/BillboardPass.h"
#include "Editor/Public/Camera.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Texture/Public/Texture.h"
#include "Manager/Time/Public/TimeManager.h"

FBillboardPass::FBillboardPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel,
                               ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS, ID3D11BlendState* InBS)
        : FRenderPass(InPipeline, InConstantBufferCamera, InConstantBufferModel), VS(InVS), PS(InPS), InputLayout(InLayout), DS(InDS), BS(InBS)
{
    ConstantBufferMaterial = FRenderResourceFactory::CreateConstantBuffer<FMaterialConstants>();
    BillboardMaterialConstants.MaterialFlags |= HAS_DIFFUSE_MAP;
    BillboardMaterialConstants.Kd = FVector4(1.0f, 1.0f, 1.0f, 1.0f);  // White to preserve texture colors
}

void FBillboardPass::Execute(FRenderingContext& Context)
{
    FRenderState RenderState = UBillBoardComponent::GetClassDefaultRenderState();
    if (Context.ViewMode == EViewModeIndex::VMI_Wireframe)
    {
        RenderState.CullMode = ECullMode::None;
        RenderState.FillMode = EFillMode::WireFrame;
    }
    FPipelineInfo PipelineInfo = { InputLayout, VS, FRenderResourceFactory::GetRasterizerState(RenderState), DS, PS, BS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
    Pipeline->UpdatePipeline(PipelineInfo);

    if (!(Context.ShowFlags & EEngineShowFlags::SF_Billboard)) { return; }

    // Update global time for all billboards
    BillboardMaterialConstants.Time = UTimeManager::GetInstance().GetGameTime();

    FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferMaterial, BillboardMaterialConstants);
    Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferMaterial);

    // Billboard Sort
    struct FDistanceSortedBillboard
    {
        UBillBoardComponent* BillBoard;
        float DistanceSq;
    };

    std::vector<FDistanceSortedBillboard> SortedBillboards;
    FVector CameraLocation = Context.CurrentCamera->GetLocation();

    for (UBillBoardComponent* BillBoardComp : Context.BillBoards)
    {
        BillBoardComp->FaceCamera(Context.CurrentCamera->GetForward());
        FVector BillboardLocation = BillBoardComp->GetWorldLocation();
        float DistanceSq = FVector::DistSquared(CameraLocation, BillboardLocation);
        SortedBillboards.push_back({ BillBoardComp, DistanceSq });
    }

    // DistanceSq가 클수록 앞에 오도록 정렬
    std::sort(SortedBillboards.begin(), SortedBillboards.end(), [](const FDistanceSortedBillboard& a, const FDistanceSortedBillboard& b) {
        return a.DistanceSq > b.DistanceSq;
    });

    for (const auto& SortedItem : SortedBillboards)
    {
        UBillBoardComponent* BillBoardComp = SortedItem.BillBoard;
        if (!BillBoardComp->IsVisible()) { continue; }
        const FVector4 Tint = BillBoardComp->GetSpriteTint();
        BillboardMaterialConstants.Ka = Tint;
        BillboardMaterialConstants.Kd = Tint;

        // Update SubUV animation parameters
        if (BillBoardComp->IsSubUVAnimationEnabled())
        {
            BillboardMaterialConstants.MaterialFlags |= HAS_SUBUV_ANIMATION;
            BillboardMaterialConstants.SubUVGridColumns = BillBoardComp->GetSubUVGridColumns();
            BillboardMaterialConstants.SubUVGridRows = BillBoardComp->GetSubUVGridRows();
            BillboardMaterialConstants.SubUVAnimationSpeed = BillBoardComp->GetSubUVAnimationSpeed();

            // 상대 시간 모드: 각 빌보드가 독립적으로 애니메이션 재생
            if (BillBoardComp->IsUsingRelativeTime())
            {
                float CurrentTime = UTimeManager::GetInstance().GetGameTime();
                float SpawnTime = BillBoardComp->GetSpawnTime();
                BillboardMaterialConstants.Time = CurrentTime - SpawnTime; // 상대 시간 계산
            }
        }
        else
        {
            BillboardMaterialConstants.MaterialFlags &= ~HAS_SUBUV_ANIMATION;
            BillboardMaterialConstants.SubUVGridColumns = 1;
            BillboardMaterialConstants.SubUVGridRows = 1;
            BillboardMaterialConstants.SubUVAnimationSpeed = 1.0f;
        }

        //UE_LOG("%f %f %f %f", Tint.X,Tint.Y,Tint.Z,Tint.W);
        FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferMaterial, BillboardMaterialConstants);
        Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferMaterial);


        FMatrix WorldMatrix;
        if (BillBoardComp->IsScreenSizeScaled())
        {
            FVector FixedWorldScale = BillBoardComp->GetRelativeScale3D(); 
            FVector BillboardLocation = BillBoardComp->GetWorldLocation();
            FQuaternion BillboardRotation = BillBoardComp->GetWorldRotationAsQuaternion();

            WorldMatrix = FMatrix::GetModelMatrix(BillboardLocation, BillboardRotation, FixedWorldScale);
        }
        else { WorldMatrix = BillBoardComp->GetWorldTransformMatrix(); }

        Pipeline->SetVertexBuffer(BillBoardComp->GetVertexBuffer(), sizeof(FNormalVertex));
        Pipeline->SetIndexBuffer(BillBoardComp->GetIndexBuffer(), 0);

        FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, WorldMatrix);
        Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

        Pipeline->SetShaderResourceView(0, EShaderType::PS, BillBoardComp->GetSprite()->GetTextureSRV());
        Pipeline->SetSamplerState(0, EShaderType::PS, BillBoardComp->GetSprite()->GetTextureSampler());

        Pipeline->DrawIndexed(BillBoardComp->GetNumIndices(), 0, 0);
    }

}

void FBillboardPass::Release()
{
    SafeRelease(ConstantBufferMaterial);
}
