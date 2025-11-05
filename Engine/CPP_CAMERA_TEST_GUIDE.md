# C++ 카메라 트랜지션 테스트 가이드

C++ 코드에서 직접 카메라 트랜지션을 테스트하는 방법입니다.

---

## 🎯 빠른 시작

### 1. Level.cpp에서 테스트하기

가장 쉬운 방법은 `Level.cpp`의 `Tick()` 함수에서 직접 테스트하는 것입니다.

**파일**: `Source/Level/Private/Level.cpp`

```cpp
#include "Level/Public/CameraTransitionTestHelper.h"

void ULevel::Tick(float DeltaTime)
{
    // ... 기존 코드 ...

    // PIE 모드에서만 테스트
    if (GWorld && GWorld->GetGameMode())
    {
        APlayerCameraManager* CamMgr = GWorld->GetPlayerCameraManager();
        UInputManager* Input = UInputManager::Get();

        if (CamMgr && Input)
        {
            // T 키: 기본 전환 테스트
            if (Input->IsKeyPressed(EKeyInput::T))
            {
                CameraTransitionTestHelper::TestBasicTransition(CamMgr);
            }

            // Y 키: 모든 Easing 타입 테스트 (순환)
            static int easingIndex = 0;
            if (Input->IsKeyPressed(EKeyInput::Y))
            {
                CameraTransitionTestHelper::TestAllEasingTypes(CamMgr, easingIndex);
                easingIndex = (easingIndex + 1) % 6;
            }

            // U 키: 순차 전환 테스트
            if (Input->IsKeyPressed(EKeyInput::U))
            {
                static int seqStep = 0;
                CameraTransitionTestHelper::TestSequentialTransitions(CamMgr, seqStep);
                seqStep = (seqStep + 1) % 5;
            }

            // I 키: 즉시 전환 (보간 없음)
            if (Input->IsKeyPressed(EKeyInput::I))
            {
                CameraTransitionTestHelper::TestInstantTransition(CamMgr);
            }

            // O 키: 트랜지션 중단
            if (Input->IsKeyPressed(EKeyInput::O))
            {
                CameraTransitionTestHelper::TestStopTransition(CamMgr);
            }

            // P 키: 선택된 액터 추적
            if (Input->IsKeyPressed(EKeyInput::P))
            {
                // 에디터에서 선택된 액터를 추적
                UEditor* Editor = UEditor::Get();
                if (Editor && Editor->GetSelectedActors().size() > 0)
                {
                    AActor* SelectedActor = Editor->GetSelectedActors()[0];
                    CameraTransitionTestHelper::TestFollowActor(CamMgr, SelectedActor);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[CameraTest] No actor selected. Select an actor first!"));
                }
            }

            // 진행도 출력 (트랜지션 중일 때만)
            if (CamMgr->IsCameraTransitioning())
            {
                // 프레임마다 출력하면 너무 많으니 간헐적으로만 출력
                static float printTimer = 0.0f;
                printTimer += DeltaTime;
                if (printTimer >= 0.5f)  // 0.5초마다
                {
                    CameraTransitionTestHelper::PrintTransitionStatus(CamMgr);
                    printTimer = 0.0f;
                }
            }
        }
    }
}
```

---

## 🎮 컨트롤 (Level.cpp에 위 코드 추가 시)

| 키 | 기능 |
|----|------|
| **T** | 기본 전환 테스트 (정면 뷰) |
| **Y** | Easing 타입 순환 테스트 (Linear → EaseIn → ... → SmootherStep) |
| **U** | 순차 전환 테스트 (Front → Top → Side → Low → High) |
| **I** | 즉시 전환 (보간 없음) |
| **O** | 트랜지션 중단 |
| **P** | 선택된 액터 추적 (에디터에서 액터 선택 후 P 누르기) |

---

## 📝 직접 API 호출하기

헬퍼 함수 없이 직접 API를 호출할 수도 있습니다:

```cpp
// 1. PlayerCameraManager 가져오기
APlayerCameraManager* CamMgr = GWorld->GetPlayerCameraManager();
if (!CamMgr) return;

// 2. 특정 위치/회전으로 전환
FVector targetLocation(500.0f, 0.0f, 200.0f);
FRotator targetRotation(-20.0f, 180.0f, 0.0f);
float duration = 2.0f;
ECameraEaseType easeType = ECameraEaseType::EaseInOut;

CamMgr->StartTransitionToLocation(targetLocation, targetRotation, duration, easeType);

// 3. FOV도 함께 변경하려면 (옵션)
float targetFOV = 60.0f;
CamMgr->StartTransitionToLocation(targetLocation, targetRotation, duration, easeType, targetFOV);

// 4. 액터 추적
AActor* targetActor = /* 추적할 액터 */;
FVector offset(-300.0f, 0.0f, 100.0f);  // 3인칭 뷰 오프셋
CamMgr->StartTransitionToActor(targetActor, 1.5f, ECameraEaseType::SmoothStep, offset);

// 5. 트랜지션 중단
CamMgr->StopCameraTransition();

// 6. 상태 확인
if (CamMgr->IsCameraTransitioning())
{
    float progress = CamMgr->GetTransitionProgress();  // 0.0 ~ 1.0
    UE_LOG(LogTemp, Log, TEXT("Transition progress: %.1f%%"), progress * 100.0f);
}
```

---

## 🔧 GameMode에서 테스트하기

GameMode의 Tick에서도 동일하게 사용할 수 있습니다:

**파일**: `Source/Level/Private/GameMode.cpp`

```cpp
#include "Level/Public/CameraTransitionTestHelper.h"

void AGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APlayerCameraManager* CamMgr = GetPlayerCameraManager();
    UInputManager* Input = UInputManager::Get();

    if (CamMgr && Input)
    {
        // 위와 동일한 키 바인딩 코드
        if (Input->IsKeyPressed(EKeyInput::T))
        {
            CameraTransitionTestHelper::TestBasicTransition(CamMgr);
        }
        // ... 등등
    }
}
```

---

## 🎬 고급 사용 예제

### 360도 회전 카메라 (타임랩스 스타일)

```cpp
void ULevel::CreateRotatingCameraSequence(APlayerCameraManager* CamMgr)
{
    // 이 코드는 Tick에서 상태 머신으로 구현해야 합니다
    static int currentAngle = 0;
    static bool isRotating = false;

    if (!isRotating && Input->IsKeyPressed(EKeyInput::K))
    {
        isRotating = true;
        currentAngle = 0;
    }

    if (isRotating && !CamMgr->IsCameraTransitioning())
    {
        float radius = 500.0f;
        float height = 200.0f;
        float angleRad = currentAngle * (3.14159f / 180.0f);

        FVector location(
            cos(angleRad) * radius,
            sin(angleRad) * radius,
            height
        );
        FRotator rotation(0.0f, currentAngle + 180.0f, 0.0f);

        CamMgr->StartTransitionToLocation(location, rotation, 0.5f, ECameraEaseType::Linear);

        currentAngle += 15;  // 15도씩 회전
        if (currentAngle >= 360)
        {
            isRotating = false;
            currentAngle = 0;
            UE_LOG(LogTemp, Log, TEXT("[CameraTest] 360 rotation complete!"));
        }
    }
}
```

### 컷신 시퀀스 (대화 씬)

```cpp
void CreateDialogueSequence(APlayerCameraManager* CamMgr, AActor* Speaker1, AActor* Speaker2)
{
    struct Shot {
        AActor* Target;
        FVector Offset;
        float Duration;
        ECameraEaseType Ease;
    };

    static Shot shots[] = {
        { Speaker1, FVector(-200, 100, 80), 1.5f, ECameraEaseType::EaseInOut },  // 화자 1 클로즈업
        { Speaker2, FVector(-200, -100, 80), 1.5f, ECameraEaseType::EaseInOut }, // 화자 2 클로즈업
        { Speaker1, FVector(-300, 0, 150), 2.0f, ECameraEaseType::SmoothStep },  // 화자 1 미드샷
        { nullptr, FVector(-500, 0, 200), 2.5f, ECameraEaseType::SmootherStep }  // 와이드 샷
    };

    static int currentShot = 0;
    static bool sequenceActive = false;

    if (!sequenceActive && Input->IsKeyPressed(EKeyInput::L))
    {
        sequenceActive = true;
        currentShot = 0;
    }

    if (sequenceActive && !CamMgr->IsCameraTransitioning())
    {
        if (currentShot < sizeof(shots) / sizeof(Shot))
        {
            Shot& shot = shots[currentShot];

            if (shot.Target)
            {
                CamMgr->StartTransitionToActor(shot.Target, shot.Duration, shot.Ease, shot.Offset);
            }
            else
            {
                // 고정 위치 샷
                FVector midPoint = (Speaker1->GetActorLocation() + Speaker2->GetActorLocation()) * 0.5f;
                CamMgr->StartTransitionToLocation(midPoint + shot.Offset,
                    FRotator(-20, 0, 0), shot.Duration, shot.Ease);
            }

            currentShot++;
        }
        else
        {
            sequenceActive = false;
            currentShot = 0;
            UE_LOG(LogTemp, Log, TEXT("[CameraTest] Dialogue sequence complete!"));
        }
    }
}
```

### 줌 인/아웃 효과

```cpp
void TestZoomEffect(APlayerCameraManager* CamMgr)
{
    static bool isZoomed = false;

    if (Input->IsKeyPressed(EKeyInput::Z))
    {
        FVector currentLoc = CamMgr->GetCameraLocation();
        FRotator currentRot = CamMgr->GetCameraRotation();

        if (!isZoomed)
        {
            // 줌 인: FOV 30도
            CamMgr->StartTransitionToLocation(currentLoc, currentRot, 0.5f,
                ECameraEaseType::EaseOut, 30.0f);
            isZoomed = true;
        }
        else
        {
            // 줌 아웃: FOV 90도
            CamMgr->StartTransitionToLocation(currentLoc, currentRot, 0.5f,
                ECameraEaseType::EaseIn, 90.0f);
            isZoomed = false;
        }
    }
}
```

---

## 💡 팁 & 주의사항

### 1. PIE 모드에서만 동작
- `PlayerCameraManager`는 PIE (Play In Editor) 모드에서만 생성됩니다
- 에디터 모드에서는 `UCamera` 사용 (다른 API)

### 2. 트랜지션 체이닝
- 트랜지션이 진행 중일 때 새로운 전환을 시작하면 이전 것을 덮어씁니다
- 순차 전환을 하려면 `IsCameraTransitioning()`으로 완료 여부 확인 후 시작

```cpp
// ❌ 이렇게 하면 마지막 것만 실행됨
CamMgr->StartTransitionToLocation(loc1, rot1, 1.0f, ECameraEaseType::EaseIn);
CamMgr->StartTransitionToLocation(loc2, rot2, 1.0f, ECameraEaseType::EaseOut);

// ✅ 이렇게 해야 순차 실행
if (!CamMgr->IsCameraTransitioning())
{
    CamMgr->StartTransitionToLocation(nextLoc, nextRot, 1.0f, easeType);
}
```

### 3. Duration = 0 → 즉시 전환
- `duration = 0.0f`로 설정하면 보간 없이 즉시 이동
- 텔레포트 효과에 사용

### 4. Easing 타입 선택 가이드
- **Linear**: 일정한 속도 (기계적, 로봇틱)
- **EaseIn**: 천천히 시작 (가속)
- **EaseOut**: 천천히 끝 (감속)
- **EaseInOut**: 부드러운 시작과 끝 (가장 자연스러움) ⭐ 추천
- **SmoothStep**: S곡선 (영화 같은 느낌)
- **SmootherStep**: 더 부드러운 S곡선 (고급스러운 느낌)

### 5. 액터 추적 시 오프셋
```cpp
// 다양한 카메라 앵글
FVector behindOffset(-300, 0, 100);      // 3인칭 뒤
FVector sideOffset(0, -400, 80);         // 측면
FVector topOffset(0, 0, 500);            // 탑다운
FVector closeupOffset(-100, 50, 60);     // 클로즈업
```

---

## 📊 디버깅

로그 확인:
```cpp
// 트랜지션 시작 시 로그가 자동 출력됨
// Log/[날짜].log 파일에서 "[CameraTest]" 검색

// 추가 디버그 정보
if (CamMgr->IsCameraTransitioning())
{
    UE_LOG(LogTemp, Log, TEXT("Camera transitioning: %.1f%%"),
        CamMgr->GetTransitionProgress() * 100.0f);
}
```

---

**이제 테스트할 준비가 되었습니다!** 🎥

1. `Level.cpp`에 위 코드 추가
2. 빌드: `MSBuild Engine.vcxproj /p:Configuration=Debug /p:Platform=x64`
3. 실행: `Build\Debug\Engine.exe`
4. PIE 모드 시작
5. T/Y/U/I/O/P 키로 테스트!
