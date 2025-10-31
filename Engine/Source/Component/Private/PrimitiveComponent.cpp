#include "pch.h"
#include "Component/Public/PrimitiveComponent.h"

#include "Manager/Asset/Public/AssetManager.h"
#include "Physics/Public/AABB.h"
#include "Physics/Public/OBB.h"
#include "Utility/Public/JsonSerializer.h"
#include "Actor/Public/Actor.h"
#include "Core/Public/Object.h"

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
	OverlapInfos.clear();

	if (!bGenerateOverlapEvents)
	{
		return;
	}

	AActor* ThisOwner = GetOwner();
	const TArray<UObject*>& Objects = GetUObjectArray();

	FVector ThisMin, ThisMax;
	GetWorldAABB(ThisMin, ThisMax);
	FAABB ThisAABB(ThisMin, ThisMax);

	for (UObject* Obj : Objects)
	{
		UPrimitiveComponent* OtherPrim = Cast<UPrimitiveComponent>(Obj);
		if (OtherPrim == nullptr || OtherPrim == this)
		{
			continue;
		}

		// Owner가 없는 프리미티브는 스킵
		if (OtherPrim->GetOwner() == nullptr)
		{
			continue;
		}

		// 같은 액터 내부끼리는 스킵(필요 시 제거 가능)
		if (ThisOwner != nullptr && OtherPrim->GetOwner() == ThisOwner)
		{
			continue;
		}

		FVector OtherMin, OtherMax;
		OtherPrim->GetWorldAABB(OtherMin, OtherMax);

		FAABB OtherAABB(OtherMin, OtherMax);
		if (ThisAABB.IsIntersected(OtherAABB))
		{
			FOverlapInfo Info;
			Info.OtherComponent = OtherPrim;
			Info.OtherActor = OtherPrim->GetOwner();
			OverlapInfos.push_back(Info);
		}
	}
}