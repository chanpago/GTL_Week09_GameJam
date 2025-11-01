#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/LightComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/PointLightComponent.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Component/Public/AmbientLightComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Component/Collision/Public/ShapeComponent.h"
#include "Core/Public/Object.h"
#include "Editor/Public/Editor.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Global/Octree.h"
#include "Level/Public/Level.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Utility/Public/JsonSerializer.h"
#include "Manager/UI/Public/ViewportManager.h"
#include <json.hpp>

IMPLEMENT_CLASS(ULevel, UObject)

ULevel::ULevel()
{
	StaticOctree = new FOctree(FVector(0, 0, -5), 75, 0);
}

ULevel::~ULevel()
{
	// LevelActors 배열에 남아있는 모든 액터의 메모리를 해제합니다.
	for (const auto& Actor : LevelActors)
	{
		DestroyActor(Actor);
	}
	LevelActors.clear();

	// 모든 액터 객체가 삭제되었으므로, 포인터를 담고 있던 컨테이너들을 비웁니다.
	SafeDelete(StaticOctree);
}

void ULevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		LightComponents.clear();
		ShapeComponents.clear();

		// 동적 오브젝트 추적 정보 초기화
		DynamicPrimitiveMap.clear();
		while (!DynamicPrimitiveQueue.empty())
		{
			DynamicPrimitiveQueue.pop();
		}
		OctreeInsertRetryCount.clear();
		// 옥트리 완전 초기화
		if (StaticOctree)
		{
			StaticOctree->Clear();
		}

		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		uint32 NextUUID = 0;
		FJsonSerializer::ReadUint32(InOutHandle, "NextUUID", NextUUID);

		JSON ActorsJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "Actors", ActorsJson))
		{
			for (auto& Pair : ActorsJson.ObjectRange())
			{
				JSON& ActorDataJson = Pair.second;

				FString TypeString;
				FJsonSerializer::ReadString(ActorDataJson, "Type", TypeString);
				
				UClass* ActorClass = UClass::FindClass(TypeString);
				SpawnActorToLevel(ActorClass, &ActorDataJson); 
			}
		}

		// 뷰포트 카메라 정보 로드
		UViewportManager::GetInstance().SerializeViewports(bInIsLoading, InOutHandle);
	}
	// 저장
	else
	{
		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		InOutHandle["NextUUID"] = 0;

		JSON ActorsJson = json::Object();
		for (AActor* Actor : LevelActors)
		{
			JSON ActorJson;
			ActorJson["Type"] = Actor->GetClass()->GetName().ToString();
			Actor->Serialize(bInIsLoading, ActorJson); 

			ActorsJson[std::to_string(Actor->GetUUID())] = ActorJson;
		}
		InOutHandle["Actors"] = ActorsJson;

		// 뷰포트 카메라 정보 저장
		UViewportManager::GetInstance().SerializeViewports(bInIsLoading, InOutHandle);
	}
}

void ULevel::Init()
{
	for (AActor* Actor: LevelActors)
	{
		if (Actor)
		{
			Actor->BeginPlay();
		}
	}
}

AActor* ULevel::SpawnActorToLevel(UClass* InActorClass, JSON* ActorJsonData)
{
	if (!InActorClass)
	{
		return nullptr;
	}

	AActor* NewActor = Cast<AActor>(NewObject(InActorClass, this));
	if (NewActor)
	{
		LevelActors.push_back(NewActor);
		if (ActorJsonData != nullptr)
		{
			NewActor->Serialize(true, *ActorJsonData);
		}
		else
		{
			NewActor->InitializeComponents();
		}
		NewActor->BeginPlay();
		AddLevelComponent(NewActor);
		return NewActor;
	}

	return nullptr;
}

void ULevel::RegisterComponent(UActorComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	if (!StaticOctree)
	{
		return;
	}

	if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent))
	{
		// StaticOctree에 먼저 삽입 시도
		if (!(StaticOctree->Insert(PrimitiveComponent)))
		{
			// 실패하면 DynamicPrimitiveQueue 목록에 추가
			OnPrimitiveUpdated(PrimitiveComponent);
		}
		if (auto Shape = Cast<UShapeComponent>(PrimitiveComponent))
		{
			ShapeComponents.push_back(Shape);
		}
	}
	else if (auto LightComponent = Cast<ULightComponent>(InComponent))
	{
		if (auto PointLightComponent = Cast<UPointLightComponent>(LightComponent))
		{
			if (auto SpotLightComponent = Cast<USpotLightComponent>(PointLightComponent))
			{
				LightComponents.push_back(SpotLightComponent);
			}
			else
			{
				LightComponents.push_back(PointLightComponent);
			}
		}
		if (auto DirectionalLightComponent = Cast<UDirectionalLightComponent>(LightComponent))
		{
			LightComponents.push_back(DirectionalLightComponent);
		}
		if (auto AmbientLightComponent = Cast<UAmbientLightComponent>(LightComponent))
		{
			LightComponents.push_back(AmbientLightComponent);
		}
	}
	UE_LOG("Level: '%s' 컴포넌트를 씬에 등록했습니다.", InComponent->GetName().ToString().data());
}

void ULevel::UnregisterComponent(UActorComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	if (!StaticOctree)
	{
		return;
	}

	if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent))
	{
		// StaticOctree에서 제거 시도
		StaticOctree->Remove(PrimitiveComponent);
	
		OnPrimitiveUnregistered(PrimitiveComponent);
		if (UShapeComponent* ShapeComponent = Cast<UShapeComponent>(PrimitiveComponent))
		{
			// 해당 포인터의 모든 인스턴스 제거
			ShapeComponents.erase(
				std::remove(ShapeComponents.begin(), ShapeComponents.end(), ShapeComponent),
				ShapeComponents.end()
			);
		}
	}
	else if (auto LightComponent = Cast<ULightComponent>(InComponent))
	{
		// Erase-Remove Idiom: 해당 컴포넌트의 모든 인스턴스를 한 번에 제거
		LightComponents.erase(
			std::remove(LightComponents.begin(), LightComponents.end(), LightComponent),
			LightComponents.end()
		);
	}
}

void ULevel::AddActorToLevel(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	LevelActors.push_back(InActor);
}

void ULevel::AddLevelComponent(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// 각 컴포넌트를 RegisterComponent에 위임
	for (auto& Component : Actor->GetOwnedComponents())
	{
		RegisterComponent(Component);
	}
}

// Level에서 Actor 제거하는 함수
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}

	// 컴포넌트들을 옥트리에서 제거
	for (auto& Component : InActor->GetOwnedComponents())
	{
		UnregisterComponent(Component);
	}

	// LevelActors 리스트에서 제거
	if (auto It = std::find(LevelActors.begin(), LevelActors.end(), InActor); It != LevelActors.end())
	{
		*It = std::move(LevelActors.back());
		LevelActors.pop_back();
	}

	// Remove Actor Selection
	UEditor* Editor = GEditor->GetEditorModule();
	if (Editor->GetSelectedActor() == InActor)
	{
		Editor->SelectActor(nullptr);
		Editor->SelectComponent(nullptr);
	}

	// Remove
	SafeDelete(InActor);

	return true;
}

void ULevel::UpdatePrimitiveInOctree(UPrimitiveComponent* InComponent)
{
	if (!StaticOctree->Remove(InComponent))
		return;
	OnPrimitiveUpdated(InComponent);
}

UObject* ULevel::Duplicate()
{
	ULevel* Level = Cast<ULevel>(Super::Duplicate());
	Level->ShowFlags = ShowFlags;
	return Level;
}

void ULevel::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	ULevel* DuplicatedLevel = Cast<ULevel>(DuplicatedObject);

	for (AActor* Actor : LevelActors)
	{
		AActor* DuplicatedActor = Cast<AActor>(Actor->Duplicate());
		DuplicatedLevel->LevelActors.push_back(DuplicatedActor);
		DuplicatedLevel->AddLevelComponent(DuplicatedActor);
	}
}

/*-----------------------------------------------------------------------------
	Octree Management
-----------------------------------------------------------------------------*/

void ULevel::UpdateOctree()
{
	if (!StaticOctree)
	{
		return;
	}
	
	uint32 Count = 0;
	FDynamicPrimitiveQueue NotInsertedQueue;
	
	while (!DynamicPrimitiveQueue.empty() && Count < MAX_OBJECTS_TO_INSERT_PER_FRAME)
	{
		auto [Component, TimePoint] = DynamicPrimitiveQueue.front();
		DynamicPrimitiveQueue.pop();

		if (auto It = DynamicPrimitiveMap.find(Component); It != DynamicPrimitiveMap.end())
		{
			if (It->second <= TimePoint)
			{
				// 큐에 기록된 오브젝트의 마지막 변경 시간 이후로 변경이 없었다면 Octree에 재삽입한다.
				if (StaticOctree->Insert(Component))
				{
					DynamicPrimitiveMap.erase(It);
					OctreeInsertRetryCount.erase(Component);
				}
				// 삽입이 안됐다면 다시 Queue에 들어가기 위해 저장
				else
				{
					// 재시도 횟수 확인
					int32 CurrentRetryCount = 0;
					if (auto RetryIt = OctreeInsertRetryCount.find(Component); RetryIt != OctreeInsertRetryCount.end())
					{
						CurrentRetryCount = RetryIt->second;
					}

					// 최대 10회까지 재시도
					if (CurrentRetryCount < 10)
					{
						// 재시도 횟수 증가
						OctreeInsertRetryCount[Component] = CurrentRetryCount + 1;

						// 다시 큐에 추가 (2개 필드만 사용)
						NotInsertedQueue.push({ Component, It->second });
					}
					else
					{
						// 10회 실패 시 추적 중단
						UE_LOG_WARNING("UpdateOctree: Component '%s'가 옥트리 영역 밖으로 벗어나 추적을 중단합니다.",
							Component->GetName().ToString().data());

						DynamicPrimitiveMap.erase(It);
						OctreeInsertRetryCount.erase(Component);
					}
				}
				// TODO: 오브젝트의 유일성을 보장하기 위해 StaticOctree->Remove(Component)가 필요한가?
				++Count;
			}
			else
			{
				// 큐에 기록된 오브젝트의 마지막 변경 이후 새로운 변경이 존재했다면 다시 큐에 삽입한다.
				DynamicPrimitiveQueue.push({Component, It->second});
			}
		}
	}
	
	DynamicPrimitiveQueue = NotInsertedQueue;
	if (Count != 0)
	{
		// UE_LOG("UpdateOctree: %d개의 컴포넌트가 업데이트 되었습니다.", Count);
	}
}

void ULevel::OnPrimitiveUpdated(UPrimitiveComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	if (!StaticOctree)
	{
		return;  // 옥트리가 없으면 추적하지 않음
	}
	float GameTime = UTimeManager::GetInstance().GetGameTime();
	if (auto It = DynamicPrimitiveMap.find(InComponent); It != DynamicPrimitiveMap.end())
	{
		It->second = GameTime;
	}
	else
	{
		DynamicPrimitiveMap[InComponent] = UTimeManager::GetInstance().GetGameTime();

		DynamicPrimitiveQueue.push({InComponent, GameTime});
	}
}

void ULevel::OnPrimitiveUnregistered(UPrimitiveComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	if (auto It = DynamicPrimitiveMap.find(InComponent); It != DynamicPrimitiveMap.end())
	{
		DynamicPrimitiveMap.erase(It);
	}
	OctreeInsertRetryCount.erase(InComponent);
}
