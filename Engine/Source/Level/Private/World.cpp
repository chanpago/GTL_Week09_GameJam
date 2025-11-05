#include "pch.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Actor/Public/AmbientLight.h"
#include "Component/Public/ULuaScriptComponent.h"
#include "GameMode/Public/GameMode.h"
#include "GameMode/Public/GameModeBase.h"
#include "GamePlay/Public/PlayerController.h"
#include "Utility/Public/JsonSerializer.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Manager/Camera/Public/PlayerCameraManager.h"

IMPLEMENT_CLASS(UWorld, UObject)

UWorld::UWorld()
	: WorldType(EWorldType::Editor)
	, bBegunPlay(false)
{
}

UWorld::UWorld(EWorldType InWorldType)
	: WorldType(InWorldType)
	, bBegunPlay(false)
{
}

UWorld::~UWorld()
{
	EndPlay();
	if (Level)
	{
		ULevel* CurrentLevel = Level;
		SafeDelete(CurrentLevel); // 내부 Clean up은 Level의 소멸자에서 수행
		Level = nullptr;
	}
}

void UWorld::BeginPlay()
{
	if (bBegunPlay)
	{
		return;
	}

	if (!Level)
	{
		UE_LOG_ERROR("World: BeginPlay 호출 전에 로드된 Level이 없습니다.");
		return;
	}

	// Spawn GameMode for Game/PIE worlds
	if (WorldType == EWorldType::Game || WorldType == EWorldType::PIE)
	{
		AGameMode* NewGameMode = SpawnGameMode();
		if (NewGameMode)
		{
			NewGameMode->InitGame();
			UE_LOG_SUCCESS("[World] GameMode initialized");
		}
	}

	Level->Init();
	bBegunPlay = true;
}

bool UWorld::EndPlay()
{
	if (!Level || !bBegunPlay)
	{
		bBegunPlay = false;
		return false;
	}

	FlushPendingDestroy();
	// Level EndPlay
	bBegunPlay = false;
	return true;
}

void UWorld::Tick(float DeltaTimes)
{
	if (!Level || !bBegunPlay)
	{
		return;
	}

	// 스폰 / 삭제 처리
	FlushPendingDestroy();

	// TODO: 현재 임시로 OCtree 업데이트 처리
	Level->UpdateOctree();

	if (WorldType == EWorldType::Editor )
	{
		// Make a copy of the actor list to avoid iterator invalidation
		TArray<AActor*> ActorsCopy = Level->GetLevelActors();

		// First pass: Tick all actors
		for (AActor* Actor : ActorsCopy)
		{
			if(Actor->CanTickInEditor() && Actor->CanTick())
			{
				Actor->Tick(DeltaTimes);
			}
		}

		// Second pass: Collect pending destroy actors (don't modify array during iteration!)
		for (AActor* Actor : ActorsCopy)
		{
			if (Actor->IsPendingDestroy())
			{
				// Add to pending list instead of destroying immediately
				if (std::find(PendingDestroyActors.begin(), PendingDestroyActors.end(), Actor) == PendingDestroyActors.end())
				{
					PendingDestroyActors.push_back(Actor);
				}
			}
		}
	}

	if (WorldType == EWorldType::Game || WorldType == EWorldType::PIE)
	{
		// PIE 모드일 때 PlayerCameraManager 업데이트
		if (WorldType == EWorldType::PIE)
		{
			// Get PlayerCameraManager from PlayerController
			AGameMode* GameMode = Cast<AGameMode>(GetGameMode());
			if (GameMode)
			{
				APlayerController* PC = GameMode->GetPlayerController();
				if (PC)
				{
					APlayerCameraManager* CamMgr = PC->GetPlayerCameraManager();
					if (CamMgr)
					{
						CamMgr->Tick(DeltaTimes);
					}
				}
			}
		}

		// Make a copy of the actor list to avoid iterator invalidation
		TArray<AActor*> ActorsCopy = Level->GetLevelActors();

		// First pass: Tick all actors
		for (AActor* Actor : ActorsCopy)
		{
			if(Actor->CanTick())
			{
				Actor->Tick(DeltaTimes);
			}
		}

		// Second pass: Collect pending destroy actors (don't modify array during iteration!)
		for (AActor* Actor : ActorsCopy)
		{
			if (Actor->IsPendingDestroy())
			{
				// Add to pending list instead of destroying immediately
				if (std::find(PendingDestroyActors.begin(), PendingDestroyActors.end(), Actor) == PendingDestroyActors.end())
				{
					PendingDestroyActors.push_back(Actor);
				}
			}
		}
	}
}

ULevel* UWorld::GetLevel() const
{
	return Level;
}

/**
* @brief 지정된 경로에서 Level을 로드하고 현재 Level로 전환합니다.
* @param InLevelFilePath 로드할 Level 파일 경로
* @return 로드 성공 여부
* @note FilePath는 최종 확정된 경로여야 합니다. EditorEngine을 통해 호출됩니다.
*/
bool UWorld::LoadLevel(path InLevelFilePath)
{
	JSON LevelJson;
	ULevel* NewLevel = nullptr;

	try
	{
		FString LevelNameString = InLevelFilePath.stem().string();
		NewLevel = NewObject<ULevel>(this);
		NewLevel->SetName(LevelNameString);

		if (!FJsonSerializer::LoadJsonFromFile(LevelJson, InLevelFilePath.string()))
		{
			UE_LOG_ERROR("World: Level JSON 로드에 실패했습니다: %s", InLevelFilePath.string().c_str());
			SafeDelete(NewLevel);
			return false;
		}

		NewLevel->SetOuter(this);
		SwitchToLevel(NewLevel);
		NewLevel->Serialize(true, LevelJson);

		UConfigManager::GetInstance().SetLastUsedLevelPath(InLevelFilePath.string());
		BeginPlay();
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("World: Level 로드 중 예외 발생: %s", Exception.what());
		SafeDelete(NewLevel);
		CreateNewLevel();
		BeginPlay();
		return false;
	}


	return true;
}

/**
* @brief 현재 Level을 지정된 경로에 저장합니다.
* @param InLevelFilePath 저장할 파일 경로
* @return 저장 성공 여부
* @note FilePath는 최종 확정된 경로여야 합니다. EditorEngine을 통해 호출됩니다.
*/
bool UWorld::SaveCurrentLevel(path InLevelFilePath) const
{
	if (!Level)
	{
		UE_LOG_ERROR("World: 저장할 Level이 없습니다.");
		return false;
	}

	if(WorldType != EWorldType::Editor && WorldType != EWorldType::EditorPreview)
	{
		UE_LOG_ERROR("World: 게임 또는 PIE 모드에서는 Level 저장이 허용되지 않습니다.");
		return false;
	}

	try
	{
		JSON LevelJson;
		Level->Serialize(false, LevelJson);

		if (!FJsonSerializer::SaveJsonToFile(LevelJson, InLevelFilePath.string()))
		{
			UE_LOG_ERROR("World: Level 저장에 실패했습니다: %s", InLevelFilePath.string().c_str());
			return false;
		}

	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("World: Level 저장 중 예외 발생: %s", Exception.what());
		return false;
	}

	return true;
}

AActor* UWorld::SpawnActor(UClass* InActorClass, JSON* ActorJsonData)
{
	if (!Level)
	{
		UE_LOG_ERROR("World: Actor를 Spawn할 수 있는 Level이 없습니다.");
		return nullptr;
	}

	return Level->SpawnActorToLevel(InActorClass, ActorJsonData);
}

/**
* @brief 지정된 Actor를 월드에서 삭제합니다. 실제 삭제는 안전한 시점에 이루어집니다.
* @param InActor 삭제할 Actor
* @return 삭제 요청이 성공적으로 접수되었는지 여부
*/
bool UWorld::DestroyActor(AActor* InActor)
{
	if (!Level)
	{
		UE_LOG_ERROR("World: Level이 없어 Actor 삭제를 수행할 수 없습니다.");
		return false;
	}

	if (!InActor)
	{
		UE_LOG_ERROR("World: DestroyActor에 null 포인터가 전달되었습니다.");
		return false;
	}

	if (std::find(PendingDestroyActors.begin(), PendingDestroyActors.end(), InActor) != PendingDestroyActors.end())
	{
		UE_LOG_ERROR("World: 이미 삭제 대기 중인 액터에 대한 중복 삭제 요청입니다.");
		return false; // 이미 삭제 대기 중인 액터
	}

	PendingDestroyActors.push_back(InActor);
	return true;
}

EWorldType UWorld::GetWorldType() const
{
	return WorldType;
}

void UWorld::SetWorldType(EWorldType InWorldType)
{
	WorldType = InWorldType;
}

/**
 * @brief 삭제 대기 중인 Actor들을 실제로 삭제합니다.
 * @note 이 함수는 Tick 루프 내에서 안전한 시점에 호출되어야 합니다.
 */
void UWorld::FlushPendingDestroy()
{
	if (PendingDestroyActors.empty() || !Level)
	{
		return;
	}

	TArray<AActor*> ActorsToProcess = PendingDestroyActors;
	PendingDestroyActors.clear();
	UE_LOG("World: %zu개의 Actor를 삭제합니다.", ActorsToProcess.size());
	for (AActor* ActorToDestroy : ActorsToProcess)
	{
		if (!Level->DestroyActor(ActorToDestroy))
		{
			UE_LOG_ERROR("World: Actor 삭제에 실패했습니다: %s", ActorToDestroy->GetName().ToString().c_str());
		}
	}
}

/**
 * @brief 현재 Level을 새 Level로 전환합니다. 기존 Level은 소멸됩니다.
 * @param InNewLevel 새로 전환할 Level
 * @note 이전 Level의 안전한 종료 및 메모리 해제를 여기에서 책입집니다.
 */
void UWorld::SwitchToLevel(ULevel* InNewLevel)
{
	EndPlay();
	if (Level)
	{
		ULevel* OldLevel = Level;
		SafeDelete(OldLevel);
		Level = nullptr;
	}

	Level = InNewLevel;
	PendingDestroyActors.clear();
	bBegunPlay = false;
}

UObject* UWorld::Duplicate()
{
	UWorld* World = Cast<UWorld>(Super::Duplicate());
	return World;
}

void UWorld::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	UWorld* World = Cast<UWorld>(DuplicatedObject);
	World->Level = Cast<ULevel>(Level->Duplicate());

	// Set Level's Outer to the duplicated World (critical for PIE!)
	if (World->Level)
	{
		World->Level->SetOuter(World);
		UE_LOG("[World] DuplicateSubObjects: Level outer set to duplicated World (%p)", World);
	}
}

void UWorld::CreateNewLevel(const FName& InLevelName)
{
	ULevel* NewLevel = NewObject<ULevel>();
	NewLevel->SetName(InLevelName);
	NewLevel->SetOuter(this);
	SwitchToLevel(NewLevel);

	// 기본 AmbientLight 추가
	AActor* SpawnedActor = SpawnActor(AAmbientLight::StaticClass());
	if (AAmbientLight* AmbientLight = Cast<AAmbientLight>(SpawnedActor))
	{
		AmbientLight->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		AmbientLight->SetName("AmbientLight");
	}

	BeginPlay();
}

void UWorld::SetGameModeClass(UClass* InGameModeClass)
{
	if (InGameModeClass && InGameModeClass->IsChildOf(AGameMode::StaticClass()))
	{
		GameModeClass = InGameModeClass;
		UE_LOG("[World] GameMode class set to: %s", InGameModeClass->GetName().ToString().c_str());
	}
	else
	{
		UE_LOG_ERROR("[World] Invalid GameMode class - must inherit from AGameMode");
	}
}

AGameMode* UWorld::SpawnGameMode()
{
	// Only spawn GameMode in Game or PIE worlds
	if (WorldType != EWorldType::Game && WorldType != EWorldType::PIE)
	{
		return nullptr;
	}

	// Use default GameModeBase if no custom class is set
	if (!GameModeClass)
	{
		GameModeClass = AGameMode::StaticClass();
	}

	AGameMode* NewGameMode = Cast<AGameMode>(SpawnActor(GameModeClass));
	if (NewGameMode)
	{
		NewGameMode->SetOwningWorld(this);
		GameMode.Set(NewGameMode);
		UE_LOG("[World] GameMode spawned: %s", NewGameMode->GetName().ToString().c_str());

		// Lua에 바인딩
		NewGameMode->InitializeLuaScript();
	}
	else
	{
		UE_LOG_ERROR("[World] Failed to spawn GameMode");
	}	

	return NewGameMode;
}

APlayerCameraManager* UWorld::GetPlayerCameraManager() const
{
	// Get PlayerCameraManager through GameMode -> PlayerController
	AGameMode* CurrentGameMode = GetGameMode();
	if (!CurrentGameMode)
	{
		return nullptr;
	}

	APlayerController* PC = CurrentGameMode->GetPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	return PC->GetPlayerCameraManager();
}