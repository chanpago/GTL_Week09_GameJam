#pragma once
#include "Core/Public/Object.h"
#include "Level/Public/World.h"

class UEditor;

/**
 * @brief UWorld 인스턴스와 그에 대한 정보를 담는 구조체
 */
struct FWorldContext
{
private:
    UWorld* WorldPtr = nullptr;
public:
    UWorld* World() const { return WorldPtr; }
    EWorldType GetType() const { return WorldPtr ? WorldPtr->GetWorldType() : EWorldType::Game; } 
    void SetWorld(UWorld* InWorld) { WorldPtr = InWorld; }
    bool operator==(const FWorldContext& Other) const { return WorldPtr == Other.WorldPtr; }
};

enum class EPIEState
{
    Stopped,
    Playing,
    Paused
};

/**
 * @brief 에디터의 최상위 제어 객체, World들을 관리
 * ClientApp 이외의 곳에서 별도로 생성하거나, GEditor 교체 절대 금지
 */
UCLASS()
class UEditorEngine final : public UObject
{
    GENERATED_BODY()
    DECLARE_CLASS(UEditorEngine, UObject)

public:
    UEditorEngine();
    ~UEditorEngine() override;

    void Tick(float DeltaSeconds);

    // PIE Management
    void StartPIE();
    void EndPIE();
    void PausePIE();
    void ResumePIE();
    void TogglePIEMouseLock();  // Shift + F1 toggle

    // Level Management
    bool LoadLevel(const FString& InFilePath);
    bool SaveCurrentLevel(const FString& InLevelName = "Untitled");
	bool CreateNewLevel(const FString& InLevelName = "Untitled");
    static std::filesystem::path GetLevelDirectory();
    static std::filesystem::path GenerateLevelFilePath(const FString& InLevelName);

    // Getter
    FWorldContext& GetEditorWorldContext() { return WorldContexts[0]; }
    UWorld* GetWorldForViewport(int32 ViewportIndex);
    EPIEState GetPIEState() const { return PIEState; }
    UEditor* GetEditorModule() const { return EditorModule; }
    bool IsPIESessionActive() const;

    // PIE 월드의 FWorldContext를 찾아서 반환
    FWorldContext* GetPIEWorldContext();
private:

    // 현재 PIE 세션 중인지 확인하고, 그렇다면 현재 WorldContext를 반환
    FWorldContext* GetActiveWorldContext();
    // PIE 카메라의 FollowTarget 정리 (불법증축!)
    void ClearPIECamera();

    EPIEState PIEState = EPIEState::Stopped;
    bool bPIEMouseUnlocked = false;  // Shift + F1 상태
    bool bUseEditorCameraInPIE = false;  // PIE 시작 시 에디터 카메라 사용 여부
    TArray<FWorldContext> WorldContexts;
    UEditor* EditorModule;
};

// UEditorEngine의 전역 인스턴스 포인터
extern UEditorEngine* GEditor;
// 현재 활성화된 UWorld 포인터
extern UWorld* GWorld;
