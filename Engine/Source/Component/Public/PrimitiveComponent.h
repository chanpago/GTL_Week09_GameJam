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
	bool bGenerateOverlapEvents = true; // 오버랩 이벤트 생성 여부
    bool bGenerateHitEvents = false;  // Hit 이벤트 생성 여부
	// TODO (SDM) - 추후에 false 시, 오브젝트 통과 불가 설정 추가 필요
	bool bBlockComponent = true; // 컴포넌트 간 충돌 차단 여부

	// TODO(SDM) - 테스트용 델리게이트
	int testvalue = 1;
	void TestFunc(int value)
	{
		UE_LOG("delegate test %d", value);

	}
		
	// 간단 오버랩 정보
	struct FOverlapInfo
	{
		UPrimitiveComponent* OtherComponent = nullptr;
		AActor* OtherActor = nullptr;
		bool operator==(const FOverlapInfo& Other) const
		{
			return OtherComponent == Other.OtherComponent;
		}
	};
	// 간단 Hit 정보
	struct FHitResult
	{
		UPrimitiveComponent* Component = nullptr;
		AActor* Actor = nullptr;
		FVector ImpactPoint = FVector::ZeroVector();
		FVector ImpactNormal = FVector::ZeroVector();
		float Distance = 0.0f;
	};

	const TArray<FOverlapInfo>& GetOverlapInfos() const { return OverlapInfos; }

	// 오버랩 쿼리(즉시 판정)
	bool IsOverlappingComponent(const UPrimitiveComponent* Other) const;
	bool IsOverlappingActor(const AActor* Other) const;

	// 프레임 갱신 시 오버랩 목록 갱신 + 델리게이트 호출
	void UpdateOverlaps();

	// ========== 델리게이트 선언 ==========

	/**
		* @brief 오버랩 시작 이벤트
		* @param OverlappedComponent 오버랩된 컴포넌트 (이 컴포넌트)
		* @param OtherActor 상대 액터
		* @param OtherComp 상대 컴포넌트
		* @param OtherBodyIndex 상대 바디 인덱스 (현재 미사용, 확장용)
		* @param bFromSweep Sweep으로부터 발생했는지 여부 (현재 미사용)
		* @param SweepResult Sweep 결과 (현재 미사용)
		*/
	TDelegate<UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32, bool, const FHitResult&> OnComponentBeginOverlap;

	/**
	 * @brief 오버랩 종료 이벤트
	 * @param OverlappedComponent 오버랩된 컴포넌트 (이 컴포넌트)
	 * @param OtherActor 상대 액터
	 * @param OtherComp 상대 컴포넌트
	 * @param OtherBodyIndex 상대 바디 인덱스 (현재 미사용, 확장용)
	 */
	TDelegate<UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32> OnComponentEndOverlap;

	/**
	 * @brief Hit(충돌) 이벤트
	 * @param HitComponent Hit된 컴포넌트 (이 컴포넌트)
	 * @param OtherActor 상대 액터
	 * @param OtherComp 상대 컴포넌트
	 * @param NormalImpulse 충격량 벡터 (현재 미사용, 물리 엔진 연동 시 사용)
	 * @param Hit Hit 결과 정보
	 */
	TDelegate<UPrimitiveComponent*, AActor*, UPrimitiveComponent*, FVector, const FHitResult&> OnComponentHit;
	TDelegate<int> TestDelegate;
protected:
	TArray<FOverlapInfo> OverlapInfos;
	TArray<FOverlapInfo> PreviousOverlapInfos;  // 이전 프레임 정보
};
