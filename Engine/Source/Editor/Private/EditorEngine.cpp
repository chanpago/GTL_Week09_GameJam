#include "pch.h"
#include "Core/Public/Object.h"
#include "Editor/Public/EditorEngine.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Render/UI/Window/Public/LevelTabBarWindow.h"
#include "Render/UI/Widget/Public/LevelTabBarWidget.h"
#include "Editor/Public/Camera.h"
#include "GameMode/Public/GameModeBase.h"
#include "GameMode/Public/GameMode.h"
#include "GamePlay/Public/PlayerController.h"
#include "GamePlay/Public/PlayerInput.h"
#include "Manager/UI/Public/GameUIManager.h"
#include "Player/Public/PlayerCharacter.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Core/Public/AudioEngine.h"
#include "Manager/Camera/Public/PlayerCameraManager.h"
#include "Manager/Camera/Public/CameraModifier_CameraShake.h"

IMPLEMENT_CLASS(UEditorEngine, UObject)
UEditorEngine* GEditor = nullptr;
UWorld* GWorld = nullptr;
UEditorEngine::UEditorEngine()
{
    GEditor = this;
    UWorld* EditorWorld = NewObject<UWorld>(this);
    EditorWorld->SetWorldType(EWorldType::Editor);

    if (EditorWorld)
    {
        FWorldContext EditorContext;
        EditorContext.SetWorld(EditorWorld);
        WorldContexts.push_back(EditorContext); 

        GWorld = EditorWorld;
    }
    EditorModule = NewObject<UEditor>();

    CreateNewLevel();
    EditorWorld->BeginPlay();
}

UEditorEngine::~UEditorEngine()
{
    if (IsPIESessionActive())
    {
        EndPIE();
    }

    for (auto WorldContext : WorldContexts)
    {
        if (WorldContext.World())
        {
            delete WorldContext.World();
        }
    }
    if (EditorModule)
    {
        delete EditorModule;
    }
    
    GWorld = nullptr;
}

/**
 * @brief WorldContext를 순회하며 World의 Tick을 처리, EditorModule Update
 */
void UEditorEngine::Tick(float DeltaSeconds)
{
    for (FWorldContext& Context : WorldContexts)
    {
        UWorld* World = Context.World();
        if (World)
        {
            if (World->GetWorldType() == EWorldType::Editor)
            {
                World->Tick(DeltaSeconds);
            }
            else if (World->GetWorldType() == EWorldType::PIE)
            {
                // PIE 상태가 Playing일 때만 틱을 실행
                if (PIEState == EPIEState::Playing)
                {
                    World->Tick(DeltaSeconds);
                    //FAudioEngine::GetInstance().Tick(DeltaSeconds, EditorModule->GetCamera());
                }
            }
        }
    }

    if (EditorModule)
    {
        EditorModule->Update();
    }

}

/**
 * @brief PIE가 활성화되어 있는지 확인
 */
bool UEditorEngine::IsPIESessionActive() const
{    
    for (const FWorldContext& Context : WorldContexts)
    {
        if (Context.World() && Context.GetType() == EWorldType::PIE)
        {
            return true;
        }
    }
    return false;
}

/**
 * brief 에디터 월드를 복제해 PIE 시작
 */
void UEditorEngine::StartPIE()
{
    if (PIEState != EPIEState::Stopped)
    {
        return;
    }

    PIEState = EPIEState::Playing;
    UWorld* EditorWorld = GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        return;
    }

    // PIE 시작 시 마지막으로 클릭한 뷰포트를 PIE 전용 뷰포트로 저장
    UViewportManager& ViewportMgr = UViewportManager::GetInstance();
    int32 LastClickedViewport = ViewportMgr.GetLastClickedViewportIndex();
    ViewportMgr.SetPIEActiveViewportIndex(LastClickedViewport);

    UWorld* PIEWorld = Cast<UWorld>(EditorWorld->Duplicate());

    if (PIEWorld)
    {
        PIEWorld->SetWorldType(EWorldType::PIE);
        FWorldContext PIEContext;
        PIEContext.SetWorld(PIEWorld);
        WorldContexts.push_back(PIEContext);

        GWorld = PIEWorld;
        PIEWorld->BeginPlay();

        FAudioEngine::GetInstance().PlayBGM("Data/Audio/TopGunSound.wav");

        // 게임 UI 매니저 초기화 (PlayerController 생성 이후)
        UGameUIManager::GetInstance().Initialize();

        // ========== Setup PIE Camera ==========
        // Get PlayerCameraManager from PlayerController
        AGameMode* GameMode = Cast<AGameMode>(PIEWorld->GetGameMode());
        APlayerController* PC = GameMode ? GameMode->GetPlayerController() : nullptr;
        APlayerCameraManager* CamMgr = PC ? PC->GetPlayerCameraManager() : nullptr;

        if (!CamMgr)
        {
            UE_LOG_ERROR("[EditorEngine] PlayerCameraManager not found in PlayerController!");
            return;
        }

        // Find PlayerCharacter in PIE world
        ULevel* PIELevel = PIEWorld->GetLevel();
        if (PIELevel)
        {
            TArray<AActor*> Actors = PIELevel->GetLevelActors();
            for (AActor* Actor : Actors)
            {
                APlayerCharacter* PlayerChar = dynamic_cast<APlayerCharacter*>(Actor);
                if (PlayerChar)
                {
                    UCameraComponent* CameraComp = PlayerChar->GetCameraComponent();
                    if (CameraComp)
                    {
                        // Check if we should use editor camera or player camera
                        if (bUseEditorCameraInPIE)
                        {
                            // Editor camera mode: Don't attach to player, keep editor camera control
                            // But still connect PlayerCameraManager to editor camera for transition support
                            int32 PIEViewportIdx = ViewportMgr.GetPIEActiveViewportIndex();
                            if (PIEViewportIdx >= 0 && PIEViewportIdx < ViewportMgr.GetViewports().size())
                            {
                                FViewport* PIEViewport = ViewportMgr.GetViewports()[PIEViewportIdx];
                                if (PIEViewport && PIEViewport->GetViewportClient())
                                {
                                    UCamera* Camera = PIEViewport->GetViewportClient()->GetCamera();
                                    if (Camera)
                                    {
                                        // Clear any previous follow target so PlayerCameraManager has full control
                                        Camera->ClearFollowTarget();

                                        // Initialize PlayerCameraManager with editor camera
                                        CamMgr->Initialize(Camera);
                                        UE_LOG("[EditorEngine] Editor camera mode: PlayerCameraManager initialized with editor camera (follow target cleared)");
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Player camera mode: Original behavior
                            // Initialize PlayerCameraManager with CameraComponent (PIE mode)
                            CamMgr->Initialize(CameraComp);
                            CamMgr->SetViewTarget(PlayerChar);
                            UE_LOG("[EditorEngine] PlayerCameraManager initialized with CameraComponent");

                            // Get camera offset from component
                            FVector CameraOffset = CameraComp->GetRelativeLocation();

                            // Set PIE viewport camera to follow player with camera offset
                            int32 PIEViewportIdx = ViewportMgr.GetPIEActiveViewportIndex();
                            if (PIEViewportIdx >= 0 && PIEViewportIdx < ViewportMgr.GetViewports().size())
                            {
                                FViewport* PIEViewport = ViewportMgr.GetViewports()[PIEViewportIdx];
                                if (PIEViewport && PIEViewport->GetViewportClient())
                                {
                                    UCamera* Camera = PIEViewport->GetViewportClient()->GetCamera();
                                    if (Camera)
                                    {
                                        Camera->SetFollowTarget(PlayerChar, CameraOffset);
                                        UE_LOG("[EditorEngine] PIE Camera set to follow PlayerCharacter with offset (%.1f, %.1f, %.1f)",
                                            CameraOffset.X, CameraOffset.Y, CameraOffset.Z);
                                    }
                                }
                            }
                        }
                    }
                    break;  // Found player, exit loop
                }
            }
        }
    }
}

/**
 * @brief PIE 종료하고 에디터 월드로 돌아감
 */
void UEditorEngine::EndPIE()
{
    if (PIEState == EPIEState::Stopped)
    {
        return;
    }
    PIEState = EPIEState::Stopped;

    // 불법증축: PIE 카메라의 FollowTarget 정리
    ClearPIECamera();

    // 게임 UI 매니저 정리 (마우스 잠금 해제 포함)
    UGameUIManager::GetInstance().Shutdown();

    // PlayerCameraManager는 PIE World의 Actor이므로 World 종료 시 자동 정리됨
    UE_LOG("[EditorEngine] PlayerCameraManager will be cleaned up with PIE World");

    bPIEMouseUnlocked = false;  // 상태 리셋

    // LevelTabBarWidget의 PIE UI 상태 리셋
    UUIManager& UIManager = UUIManager::GetInstance();
    if (ULevelTabBarWindow* LevelTabBarWindow = UIManager.GetLevelTabBarWindow())
    {
        if (ULevelTabBarWidget* LevelTabBarWidget = LevelTabBarWindow->GetMainBarWidget())
        {
            LevelTabBarWidget->ResetPIEUIState();
        }
    }

    // PIE 전용 뷰포트 인덱스 리셋
    UViewportManager::GetInstance().SetPIEActiveViewportIndex(-1);

    FWorldContext* PIEContext = GetPIEWorldContext();
    if (PIEContext)
    {
        UWorld* PIEWorld = PIEContext->World();
        PIEWorld->EndPlay();
        delete PIEWorld;

        WorldContexts.erase(std::remove(WorldContexts.begin(), WorldContexts.end(), *PIEContext),WorldContexts.end());
    }

    // GWorld를 다시 Editor World로 복원
    GWorld = GetEditorWorldContext().World();

    FAudioEngine::GetInstance().StopBGM();
}

/**
 * @brief PIE 일시정지
 */
void UEditorEngine::PausePIE()
{
    if (PIEState != EPIEState::Playing)
    {
        return;
    }
    PIEState = EPIEState::Paused;
}

/**
 * @brief PIE 재개
 */
void UEditorEngine::ResumePIE()
{
    if (PIEState != EPIEState::Paused)
    {
        return;
    }
    PIEState = EPIEState::Playing;
}

/**
 * @brief Shift + F1: PIE 중 마우스 잠금 토글 (언리얼 스타일)
 * Playing 상태에서만 작동: 마우스 + 입력 함께 토글
 */
void UEditorEngine::TogglePIEMouseLock()
{
    // PIE 모드가 아니면 무시
    if (PIEState != EPIEState::Playing)
    {
        return;
    }

    // 상태 토글
    bPIEMouseUnlocked = !bPIEMouseUnlocked;

    if (bPIEMouseUnlocked)
    {
        // 마우스 해제: 커서 보이기, 자유 이동, 입력 비활성화
        UInputManager::GetInstance().LockMouseToCenter(false);
        UE_LOG("[EditorEngine] PIE mouse unlocked (Shift+F1) - Input disabled");
    }
    else
    {
        // 마우스 잠금: 커서 숨기기, 중앙 고정, 입력 활성화
        UInputManager::GetInstance().LockMouseToCenter(true);
        UE_LOG("[EditorEngine] PIE mouse locked - Input enabled");
    }

    // PlayerInput도 함께 토글
    FWorldContext* PIEContext = GetPIEWorldContext();
    if (PIEContext && PIEContext->World())
    {
        UWorld* PIEWorld = PIEContext->World();
        AGameModeBase* GameMode = Cast<AGameMode>(PIEWorld->GetGameMode());
        if (GameMode)
        {
            APlayerController* PC = GameMode->GetPlayerController();
            if (PC)
            {
                UPlayerInput* PlayerInput = PC->GetPlayerInput();
                if (PlayerInput)
                {
                    // 마우스 잠금 해제 = 입력 비활성화
                    // 마우스 잠금 = 입력 활성화
                    PlayerInput->SetInputEnabled(!bPIEMouseUnlocked);
                    UE_LOG("[EditorEngine] PlayerInput %s", bPIEMouseUnlocked ? "DISABLED" : "ENABLED");
                }
            }
        }
    }
}

/**
 * @brief 주어진 뷰포트 인덱스에 따라 렌더링할 World 반환
 * @param ViewportIndex 뷰포트 인덱스 (0~3)
 * @return PIE active viewport면 PIE World, 아니면 Editor World
 */
UWorld* UEditorEngine::GetWorldForViewport(int32 ViewportIndex)
{
    // PIE가 활성화되지 않았으면 항상 에디터 월드
    if (!IsPIESessionActive())
    {
        return GetEditorWorldContext().World();
    }

    // PIE 활성 뷰포트 확인
    int32 PIEActiveIndex = UViewportManager::GetInstance().GetPIEActiveViewportIndex();

    // 현재 뷰포트가 PIE 활성 뷰포트면 PIE World, 아니면 Editor World
    if (ViewportIndex == PIEActiveIndex)
    {
        FWorldContext* PIEContext = GetPIEWorldContext();
        return PIEContext ? PIEContext->World() : GetEditorWorldContext().World();
    }
    else
    {
        return GetEditorWorldContext().World();
    }
}

/**
 * @brief 경로의 파일을 불러와서 현재 Editor 월드의 Level 교체 
 */
bool UEditorEngine::LoadLevel(const FString& InFilePath)
{
    UE_LOG("GEditor: Loading Level: %s", InFilePath.data());
    
    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive())
    {
        EndPIE();
    }
    return GetEditorWorldContext().World()->LoadLevel(path(InFilePath));
}

/**
 * @brief 현재 Editor 월드의 레벨을 파일로 저장
 */
bool UEditorEngine::SaveCurrentLevel(const FString& InLevelName)
{
    UE_LOG("GEditor: Saving Level: %s", InLevelName.c_str());
    
    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive())
    {
        EndPIE();
    }

    path FilePath = InLevelName;
    if (FilePath.empty())
    {
        FName CurrentLevelName = GetEditorWorldContext().World()->GetLevel()->GetName();
        FilePath = GenerateLevelFilePath(CurrentLevelName == FName::GetNone()? "Untitled" : CurrentLevelName.ToString());
    }

    UE_LOG("GEditor: 현재 레벨을 다음 경로에 저장합니다: %s", FilePath.string().c_str());

    try
    {
        bool bSuccess = GetEditorWorldContext().World()->SaveCurrentLevel(FilePath);
        if (bSuccess)
        {
            UConfigManager::GetInstance().SetLastUsedLevelPath(InLevelName);

            UE_LOG("GEditor: 레벨이 성공적으로 저장되었습니다");
        }
        else
        {
            UE_LOG("GEditor: 레벨을 저장하는 데에 실패했습니다");
        }

        return bSuccess;
    }
    catch (const exception& Exception)
    {
        UE_LOG("GEditor: 저장 과정에서 Exception 발생: %s", Exception.what());
        return false;
    }
}

/**
 * @brief 현재 Editor 월드에 새 레벨 변경
 */
bool UEditorEngine::CreateNewLevel(const FString& InLevelName)
{
    UE_LOG("GEditor: Create New Level: %s", InLevelName.c_str());
    
    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive()) { EndPIE(); }
    GetEditorWorldContext().World()->CreateNewLevel(InLevelName);
    return true;
}

path UEditorEngine::GenerateLevelFilePath(const FString& InLevelName)
{
    path LevelDirectory = GetLevelDirectory();
    path FileName = InLevelName + ".scene";
    return LevelDirectory / FileName;
}

path UEditorEngine::GetLevelDirectory()
{
    UPathManager& PathManager = UPathManager::GetInstance();
    return PathManager.GetWorldPath();
}

FWorldContext* UEditorEngine::GetPIEWorldContext()
{
    for (FWorldContext& Context : WorldContexts)
    {
        if (Context.World() && Context.GetType() == EWorldType::PIE)
        {
            return &Context; 
        }
    }
    return nullptr;
}

FWorldContext* UEditorEngine::GetActiveWorldContext()
{
    FWorldContext* PIEContext = GetPIEWorldContext();
    if (PIEContext)
    {
        return PIEContext;
    }

    if (!WorldContexts.empty()) { return &WorldContexts[0]; }

    return nullptr;
}

void UEditorEngine::ClearPIECamera()
{
    UViewportManager& ViewportManager = UViewportManager::GetInstance();

    // 모든 뷰포트의 카메라에서 FollowTarget 제거
    for (FViewport* Viewport : ViewportManager.GetViewports())
    {
        if (Viewport && Viewport->GetViewportClient())
        {
            UCamera* Camera = Viewport->GetViewportClient()->GetCamera();
            if (Camera && Camera->HasFollowTarget())
            {
                Camera->ClearFollowTarget();
                Camera->SetInputEnabled(true);  // 에디터 입력 다시 활성화
                Camera->SetFovY(90.0f);  // FOV를 에디터 기본값으로 복구
            }
        }
    }
}
