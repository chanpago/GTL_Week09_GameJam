#pragma once
#include "Component/Public/SceneComponent.h"
#include "Physics/Public/BoundingVolume.h"
#include "Core/Public/Object.h" // GetUObjectArray 사용 예정

UCLASS()
class UPrimitiveComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

public:
	UPrimitiveComponent();

	void TickComponent(float DeltaTime) override;
	virtual void OnSelected() override;
	virtual void OnDeselected() override;

	const TArray<FNormalVertex>* GetVerticesData() const;
	const TArray<uint32>* GetIndicesData() const;
	ID3D11Buffer* GetVertexBuffer() const;
	ID3D11Buffer* GetIndexBuffer() const;
	uint32 GetNumVertices() const;
	uint32 GetNumIndices() const;

	const FRenderState& GetRenderState() const { return RenderState; }

	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology);
	D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;
	//void Render(const URenderer& Renderer) const override;

	bool IsVisible() const { return bVisible; }
	void SetVisibility(bool bVisibility) { bVisible = bVisibility; }
	
	bool CanPick() const { return bCanPick; }
	void SetCanPick(bool bInCanPick) { bCanPick = bInCanPick; }
	

	FVector4 GetColor() const { return Color; }
	void SetColor(const FVector4& InColor) { Color = InColor; }

	virtual const IBoundingVolume* GetBoundingBox();
	void GetWorldAABB(FVector& OutMin, FVector& OutMax);

	virtual void MarkAsDirty() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	// 데칼에 덮일 수 있는가
	bool bReceivesDecals = true;

	// 다른 곳에서 사용할 인덱스
	mutable int32 CachedAABBIndex = -1;
	mutable uint32 CachedFrame = 0;

protected:
	const TArray<FNormalVertex>* Vertices = nullptr;
	const TArray<uint32>* Indices = nullptr;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;

	uint32 NumVertices = 0;
	uint32 NumIndices = 0;

	FVector4 Color = FVector4{ 0.f,0.f,0.f,0.f };

	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FRenderState RenderState = {};

	bool bVisible = true;
	bool bCanPick = true;

	IBoundingVolume* BoundingBox = nullptr;
	bool bOwnsBoundingBox = false;
	
	mutable FVector CachedWorldMin;
	mutable FVector CachedWorldMax;
	mutable bool bIsAABBCacheDirty = true;

public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;


/* =========================
 *	Collision Section
   ========================= */
public:
	// 충돌/오버랩 설정
	bool bGenerateOverlapEvents = true;
	bool bBlockComponent = false;

	// 간단 오버랩 정보
	struct FOverlapInfo
	{
		UPrimitiveComponent* OtherComponent = nullptr;
		AActor* OtherActor = nullptr;
	};

	const TArray<FOverlapInfo>& GetOverlapInfos() const { return OverlapInfos; }

	// 오버랩 쿼리(즉시 판정)
	bool IsOverlappingComponent(const UPrimitiveComponent* Other) const;
	bool IsOverlappingActor(const AActor* Other) const;

	// 프레임 갱신 시 오버랩 목록 갱신(델리게이트 없는 경량 버전)
	// TODO(SDM) - 추후에 델리게이트 호출 버전으로
	void UpdateOverlaps();
protected:
	TArray<FOverlapInfo> OverlapInfos;
};
