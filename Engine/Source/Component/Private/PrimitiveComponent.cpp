#include "pch.h"
#include "Component/Public/PrimitiveComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"
#include "Physics/Public/OBB.h"
#include "Utility/Public/JsonSerializer.h"
#include "Actor/Public/Actor.h"
#include "Core/Public/Object.h"
#include "Level/Public/Level.h"
#include "Component/Collision/Public/ShapeComponent.h"
#include "Component/Collision/Public/BoxComponent.h"
#include "Component/Collision/Public/SphereComponent.h"
#include "Component/Collision/Public/CapsuleComponent.h"
#include "Physics/Public/BoundingSphere.h"
#include "Physics/Public/BoundingCapsule.h"
#include "Physics/Public/CollisionUtil.h"

IMPLEMENT_ABSTRACT_CLASS(UPrimitiveComponent, USceneComponent)

UPrimitiveComponent::UPrimitiveComponent()
{
	bCanEverTick = true;
}

void UPrimitiveComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
	UpdateOverlaps();
}
void UPrimitiveComponent::OnSelected()
{
	SetColor({ 1.f, 0.8f, 0.2f, 0.4f });
}

void UPrimitiveComponent::OnDeselected()
{
	SetColor({ 0.f, 0.f, 0.f, 0.f });
}

void USceneComponent::SetUniformScale(bool bIsUniform)
{
	bIsUniformScale = bIsUniform;
}

bool USceneComponent::IsUniformScale() const
{
	return bIsUniformScale;
}

const TArray<FNormalVertex>* UPrimitiveComponent::GetVerticesData() const
{
	return Vertices;
}

const TArray<uint32>* UPrimitiveComponent::GetIndicesData() const
{
	return Indices;
}

ID3D11Buffer* UPrimitiveComponent::GetVertexBuffer() const
{
	return VertexBuffer;
}

ID3D11Buffer* UPrimitiveComponent::GetIndexBuffer() const
{
	return IndexBuffer;
}

uint32 UPrimitiveComponent::GetNumVertices() const
{
	return NumVertices;
}

uint32 UPrimitiveComponent::GetNumIndices() const
{
	return NumIndices;
}

void UPrimitiveComponent::SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
	Topology = InTopology;
}

D3D11_PRIMITIVE_TOPOLOGY UPrimitiveComponent::GetTopology() const
{
	return Topology;
}

const IBoundingVolume* UPrimitiveComponent::GetBoundingBox()
{
	BoundingBox->Update(GetWorldTransformMatrix());
	return BoundingBox;
}

void UPrimitiveComponent::GetWorldAABB(FVector& OutMin, FVector& OutMax)
{
	if (!BoundingBox)
	{
		OutMin = FVector(); OutMax = FVector();
		return;
	}

	if (bIsAABBCacheDirty)
	{
		if (BoundingBox->GetType() == EBoundingVolumeType::AABB)
		{
			const FAABB* LocalAABB = static_cast<const FAABB*>(BoundingBox);
			FVector LocalCorners[8] =
			{
				FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Min.Z),
				FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Min.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Min.Z),
				FVector(LocalAABB->Min.X, LocalAABB->Min.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Min.Y, LocalAABB->Max.Z),
				FVector(LocalAABB->Min.X, LocalAABB->Max.Y, LocalAABB->Max.Z), FVector(LocalAABB->Max.X, LocalAABB->Max.Y, LocalAABB->Max.Z)
			};

			const FMatrix& WorldTransform = GetWorldTransformMatrix();
			FVector WorldMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
			FVector WorldMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			for (int32 Idx = 0; Idx < 8; Idx++)
			{
				FVector4 WorldCorner = FVector4(LocalCorners[Idx].X, LocalCorners[Idx].Y, LocalCorners[Idx].Z, 1.0f) * WorldTransform;
				WorldMin.X = min(WorldMin.X, WorldCorner.X);
				WorldMin.Y = min(WorldMin.Y, WorldCorner.Y);
				WorldMin.Z = min(WorldMin.Z, WorldCorner.Z);
				WorldMax.X = max(WorldMax.X, WorldCorner.X);
				WorldMax.Y = max(WorldMax.Y, WorldCorner.Y);
				WorldMax.Z = max(WorldMax.Z, WorldCorner.Z);
			}

			CachedWorldMin = WorldMin;
			CachedWorldMax = WorldMax;
		}
		else if (BoundingBox->GetType() == EBoundingVolumeType::OBB ||
			BoundingBox->GetType() == EBoundingVolumeType::SpotLight)
		{
			const FOBB* OBB = static_cast<const FOBB*>(GetBoundingBox());
			FAABB AABB = OBB->ToWorldAABB();

			CachedWorldMin = AABB.Min;
			CachedWorldMax = AABB.Max;
		}

		bIsAABBCacheDirty = false;
	}

	// 캐시된 값 반환
	OutMin = CachedWorldMin;
	OutMax = CachedWorldMax;
}

void UPrimitiveComponent::MarkAsDirty()
{
	bIsAABBCacheDirty = true;
	Super::MarkAsDirty();
}


UObject* UPrimitiveComponent::Duplicate()
{
	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Super::Duplicate());
	
	PrimitiveComponent->Color = Color;
	PrimitiveComponent->Topology = Topology;
	PrimitiveComponent->RenderState = RenderState;
	PrimitiveComponent->bVisible = bVisible;
	PrimitiveComponent->bReceivesDecals = bReceivesDecals;

	PrimitiveComponent->Vertices = Vertices;
	PrimitiveComponent->Indices = Indices;
	PrimitiveComponent->VertexBuffer = VertexBuffer;
	PrimitiveComponent->IndexBuffer = IndexBuffer;
	PrimitiveComponent->NumVertices = NumVertices;
	PrimitiveComponent->NumIndices = NumIndices;

	if (!bOwnsBoundingBox)
	{
		PrimitiveComponent->BoundingBox = BoundingBox;
	}
	
	return PrimitiveComponent;
}

void UPrimitiveComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);

}
void UPrimitiveComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FString VisibleString;
		FJsonSerializer::ReadString(InOutHandle, "bVisible", VisibleString, "true");
		SetVisibility(VisibleString == "true");
	}
	else
	{
		InOutHandle["bVisible"] = bVisible ? "true" : "false";
	}

}

/* =========================
 *	Collision Section
   ========================= */

/*	
	현재는 AABB 기반의 단순 충돌입니다.
	OBB는 GetWorldAABB에서 월드 AABB로 변환되므로 그대로 동작합니다. 
	스피어/캡슐은 AABB 근사로 처리합니다.
*/
bool UPrimitiveComponent::IsOverlappingComponent(const UPrimitiveComponent* Other) const
{
	if (Other == nullptr || Other == this)
	{
		return false;
	}

	FVector ThisMin, ThisMax;
	FVector OtherMin, OtherMax;

	/* 
		const_cast는 포인터 또는 ref의 const를 잠깐 제거해주는데 사용함.
		- GetWorldAABB가 const 함수가 아니기 때문에 사용
		- 함수 포인터에는 사용 불가능
	*/
	const_cast<UPrimitiveComponent*>(this)->GetWorldAABB(ThisMin, ThisMax);
	const_cast<UPrimitiveComponent*>(Other)->GetWorldAABB(OtherMin, OtherMax);

	FAABB ThisAABB(ThisMin, ThisMax);
	FAABB OtherAABB(OtherMin, OtherMax);
	return ThisAABB.IsIntersected(OtherAABB);
}


bool UPrimitiveComponent::IsOverlappingActor(const AActor* Other) const
{
	if (Other == nullptr)
	{
		return false;
	}

	// 전역 UObject 배열을 순회해 Other 액터의 프리미티브 컴포넌트만 골라서 판정
	const TArray<UObject*>& Objects = GetUObjectArray();
	for (UObject* Obj : Objects)
	{
		if (UPrimitiveComponent* OtherPrim = Cast<UPrimitiveComponent>(Obj))
		{
			if (OtherPrim == nullptr || OtherPrim == this)
			{
				continue;
			}
			if (OtherPrim->GetOwner() != Other)
			{
				continue;
			}
			if (IsOverlappingComponent(OtherPrim))
			{
				return true;
			}
		}
	}
	return false;
}

void UPrimitiveComponent::UpdateOverlaps()
{
	// 이전 프레임 정보 백업
	PreviousOverlapInfos = OverlapInfos;

	// 현재 프레임 충돌 정보 초기화
	OverlapInfos.clear();

	if (!bGenerateOverlapEvents && !bGenerateHitEvents)
	{
		return;
	}

	AActor* ThisOwner = GetOwner();

	// ========== 1차 충돌 검사: 옥트리 (Broad Phase) ==========

	// Level의 옥트리 가져오기
	ULevel* Level = Cast<ULevel>(GWorld->GetLevel());
	if (!Level || !Level->GetStaticOctree())
	{
		return;  // 옥트리가 없으면 검사 불가
	}

	FOctree* Octree = Level->GetStaticOctree();

	// 이 컴포넌트의 AABB
	FVector ThisMin, ThisMax;
	GetWorldAABB(ThisMin, ThisMax);
	FAABB ThisAABB(ThisMin, ThisMax);

	// 옥트리에서 AABB 겹치는 후보군 추출
	TArray<UPrimitiveComponent*> Candidates;
	Octree->QueryOverlap(ThisAABB, Candidates);

	// 동적 오브젝트도 포함 (옥트리에 없는 움직이는 오브젝트들)
	const TArray<UPrimitiveComponent*>& DynamicPrimitives = Level->GetDynamicPrimitives();
	for (UPrimitiveComponent* DynamicPrim : DynamicPrimitives)
	{
		if (DynamicPrim && DynamicPrim != this)
		{
			Candidates.push_back(DynamicPrim);
		}
	}

	// ========== 2차 충돌 검사: Narrow Phase ==========

	// ShapeComponent인지 확인 (Shape가 아니면 정밀 검사 불가)
	UShapeComponent* ThisShape = Cast<UShapeComponent>(this);
	if (!ThisShape)
	{
		return;  // Shape가 아니면 Narrow Phase 불가
	}

	for (UPrimitiveComponent* Candidate : Candidates)
	{
		// 자기 자신 제외
		if (Candidate == this)
		{
			continue;
		}

		// Owner가 없는 컴포넌트 제외
		if (!Candidate->GetOwner())
		{
			continue;
		}

		// 같은 Actor 내부 컴포넌트끼리 제외 (필요 시 제거 가능)
		if (ThisOwner && Candidate->GetOwner() == ThisOwner)
		{
			continue;
		}

		// 상대도 ShapeComponent여야 정밀 검사 가능
		UShapeComponent* OtherShape = Cast<UShapeComponent>(Candidate);
		if (!OtherShape)
		{
			continue;
		}

		// Narrow Phase 충돌 검사
		if (CollisionUtil::TestOverlap(ThisShape, OtherShape))
		{
			FOverlapInfo Info;
			Info.OtherComponent = Candidate;
			Info.OtherActor = Candidate->GetOwner();
			OverlapInfos.push_back(Info);

			// ========== Hit 이벤트 처리 (블로킹 충돌) ==========
			if (bGenerateHitEvents && bBlockComponent && Candidate->bBlockComponent)
			{
				// Hit 이벤트 발생 (양쪽이 모두 Block일 때)
				FHitResult HitResult;
				HitResult.Component = Candidate;
				HitResult.Actor = Candidate->GetOwner();
				HitResult.ImpactPoint = (GetWorldLocation() + Candidate->GetWorldLocation()) * 0.5f;
				HitResult.ImpactNormal = (GetWorldLocation() - Candidate->GetWorldLocation()).GetNormalized();
				HitResult.Distance = FVector::Dist(GetWorldLocation(), Candidate->GetWorldLocation());

				FVector NormalImpulse = FVector::ZeroVector();  // 물리 엔진 연동 시 계산
				OnComponentHit.BroadCast(this, Candidate->GetOwner(), Candidate, NormalImpulse, HitResult);
			}
		}
	}

	// ========== 델리게이트 호출: BeginOverlap / EndOverlap ==========

	if (bGenerateOverlapEvents)
	{
		// 빠른 검색을 위해 Set으로 변환
		TSet<UPrimitiveComponent*> CurrentSet;
		for (const FOverlapInfo& Info : OverlapInfos)
		{
			CurrentSet.insert(Info.OtherComponent);
		}

		TSet<UPrimitiveComponent*> PreviousSet;
		for (const FOverlapInfo& Info : PreviousOverlapInfos)
		{
			PreviousSet.insert(Info.OtherComponent);
		}

		// BeginOverlap: 현재 Set에 있지만 이전 Set에 없는 것
		for (const FOverlapInfo& CurrentInfo : OverlapInfos)
		{
			if (PreviousSet.find(CurrentInfo.OtherComponent) == PreviousSet.end())
			{
				// 새로 시작된 충돌
				FHitResult SweepResult;
				OnComponentBeginOverlap.BroadCast(
					this,
					CurrentInfo.OtherActor,
					CurrentInfo.OtherComponent,
					0,      // OtherBodyIndex (미사용)
					false,  // bFromSweep (미사용)
					SweepResult
				);
			}
		}

		// EndOverlap: 이전 Set에 있지만 현재 Set에 없는 것
		for (const FOverlapInfo& PrevInfo : PreviousOverlapInfos)
		{
			if (CurrentSet.find(PrevInfo.OtherComponent) == CurrentSet.end())
			{
				// 종료된 충돌
				OnComponentEndOverlap.BroadCast(
					this,
					PrevInfo.OtherActor,
					PrevInfo.OtherComponent,
					0  // OtherBodyIndex (미사용)
				);
			}
		}
	}
}