# 카메라 트랜지션 테스트 가이드

언리얼 엔진 스타일의 카메라 트랜지션 시스템 테스트 방법

---

## 📋 테스트 스크립트 목록

### 1. CameraTransitionTest.lua
**고정 위치 카메라 전환 테스트**

5개의 미리 정의된 카메라 뷰포인트 사이를 부드럽게 전환하는 테스트입니다.

#### 사용 방법:
1. 엔진에서 새 Actor를 생성 (StaticMeshActor 등)
2. Actor의 `ScriptName`을 `CameraTransitionTest`로 설정
3. `UseScript`를 true로 설정
4. Play 시작

#### 컨트롤:
- **숫자 1-5 키** (키보드 상단 숫자 키): 특정 카메라 뷰로 즉시 전환
  - 1: Front View (정면)
  - 2: Top View (위에서)
  - 3: Side View (측면)
  - 4: Low Angle (낮은 각도)
  - 5: High Angle (높은 각도)
- **SPACE**: 모든 뷰를 순환하며 전환
- **A**: 자동 재생 모드 토글 (3초마다 자동 전환)
- **S**: 현재 트랜지션 중단
- **Q/E**: 트랜지션 속도 조절 (느리게/빠르게)
- **R**: 첫 번째 뷰로 리셋

#### 특징:
- 각 뷰마다 다른 Easing 함수 적용
- 실시간 트랜지션 진행도 표시
- 자동 재생 모드로 데모 가능

---

### 2. CameraFollowTest.lua
**액터 추적 카메라 테스트**

움직이는 액터를 부드럽게 추적하는 카메라 시스템 테스트입니다.

#### 사용 방법:
1. 엔진에서 새 Actor를 생성
2. Actor의 `ScriptName`을 `CameraFollowTest`로 설정
3. `UseScript`를 true로 설정
4. Play 시작

#### 컨트롤:
- **F**: 카메라 추적 시작/중단
- **숫자 1-5 키** (키보드 상단 숫자 키): 카메라 오프셋 프리셋 전환
  - 1: Behind (3인칭 뒤)
  - 2: Side View (측면)
  - 3: Top Down (위에서)
  - 4: Low Angle (낮은 각도)
  - 5: Far Behind (멀리 뒤)
- **SPACE**: 프리셋 순환
- **Q/E**: 트랜지션 속도 조절
- **X**: 추적 중단

#### 추적 중 액터 이동:
- **우클릭 + WASD**: 액터를 움직여서 카메라 추적 테스트
  - W/S: 앞/뒤
  - A/D: 왼쪽/오른쪽
  - Q/E: 아래/위

#### 특징:
- 5가지 카메라 오프셋 프리셋
- 실시간 오프셋 전환
- 움직이는 액터를 부드럽게 추적

---

## 💻 C++ 테스트 코드

### CameraTransitionTestHelper.h
C++ 환경에서 카메라 트랜지션을 테스트할 수 있는 헬퍼 클래스입니다.

#### 사용 예제:

```cpp
#include "Level/Public/CameraTransitionTestHelper.h"

// GameMode나 Level의 BeginPlay() 또는 Tick()에서 사용

void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // PlayerCameraManager 가져오기
    APlayerCameraManager* CamMgr = GetWorld()->GetPlayerCameraManager();

    // 1. 기본 전환 테스트
    CameraTransitionTestHelper::TestBasicTransition(CamMgr);
}

void AMyGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APlayerCameraManager* CamMgr = GetWorld()->GetPlayerCameraManager();

    // 입력에 따라 다양한 테스트 실행
    if (Input->IsKeyPressed(EKeyInput::T))
    {
        // 모든 Easing 타입 테스트
        CameraTransitionTestHelper::TestAllEasingTypes(CamMgr);
    }

    if (Input->IsKeyPressed(EKeyInput.Y))
    {
        // 액터 추적 테스트
        AActor* TargetActor = /* 추적할 액터 */;
        CameraTransitionTestHelper::TestFollowActor(CamMgr, TargetActor);
    }

    // 트랜지션 진행도 출력
    CameraTransitionTestHelper::PrintTransitionStatus(CamMgr);
}
```

#### 제공 함수:
- `TestBasicTransition()` - 기본 위치 전환 테스트
- `TestSequentialTransitions()` - 순차 전환 테스트
- `TestFollowActor()` - 액터 추적 테스트
- `TestInstantTransition()` - 즉시 전환 테스트 (보간 없음)
- `TestAllEasingTypes()` - 모든 Easing 타입 순환 테스트
- `TestStopTransition()` - 트랜지션 중단 테스트
- `PrintTransitionStatus()` - 진행도 프로그레스 바 출력

---

## 🎮 빠른 시작 가이드

### Lua 스크립트 테스트 (추천)

1. **엔진 실행**
   ```bash
   Build/Debug/Engine.exe
   ```

2. **액터 생성**
   - 에디터에서 `StaticMeshActor` 생성
   - 또는 빈 `Actor` 생성

3. **스크립트 설정**
   - Details 패널에서:
     - `UseScript`: ✓ (체크)
     - `ScriptName`: `CameraTransitionTest` 또는 `CameraFollowTest`

4. **Play 시작**
   - PIE 모드 실행
   - 키보드 컨트롤로 테스트

### C++ 테스트

1. **헤더 포함**
   ```cpp
   #include "Level/Public/CameraTransitionTestHelper.h"
   ```

2. **게임 모드나 레벨에서 호출**
   ```cpp
   APlayerCameraManager* CamMgr = World->GetPlayerCameraManager();
   CameraTransitionTestHelper::TestBasicTransition(CamMgr);
   ```

3. **빌드 및 실행**
   ```bash
   MSBuild Engine.vcxproj /p:Configuration=Debug /p:Platform=x64
   Build/Debug/Engine.exe
   ```

---

## 📊 Easing 함수 설명

| Easing Type | 설명 | 사용 상황 |
|------------|------|----------|
| **Linear** | 선형 보간 (일정한 속도) | 로봇처럼 기계적인 움직임 |
| **EaseIn** | 천천히 시작 | 카메라가 부드럽게 움직이기 시작 |
| **EaseOut** | 천천히 끝 | 목표 지점에 부드럽게 도착 |
| **EaseInOut** | 천천히 시작하고 끝 | 가장 자연스러운 전환 (추천) |
| **SmoothStep** | 부드러운 S 곡선 | 영화 같은 부드러운 카메라 워크 |
| **SmootherStep** | 더 부드러운 S 곡선 | 매우 부드럽고 고급스러운 전환 |

---

## 🎬 추천 테스트 시나리오

### 시나리오 1: 시네마틱 카메라 워크
1. `CameraTransitionTest.lua` 사용
2. `A` 키로 자동 재생 시작
3. 각 뷰가 3초간 지속되며 순환
4. 트레일러나 인트로 연출 테스트에 활용

### 시나리오 2: 3인칭 캐릭터 카메라
1. `CameraFollowTest.lua` 사용
2. `F` 키로 추적 시작
3. `1` 키로 Behind 프리셋 선택
4. 우클릭 + WASD로 캐릭터 이동
5. 카메라가 부드럽게 따라가는지 확인

### 시나리오 3: 다양한 Easing 비교
1. `CameraTransitionTest.lua` 사용
2. 같은 뷰를 다른 키로 여러 번 전환
3. `Q`/`E`로 속도 조절하며 차이 관찰
4. 프로젝트에 맞는 최적의 Easing 선택

### 시나리오 4: 컷신 카메라 전환
1. C++ `TestSequentialTransitions()` 사용
2. 여러 카메라 앵글을 순차적으로 전환
3. 컷신이나 대화 씬 연출 테스트

---

## 🐛 트러블슈팅

### "CameraManager is null" 오류
- PIE 모드에서만 PlayerCameraManager가 생성됩니다
- Play를 시작한 후 테스트하세요

### 카메라가 움직이지 않음
- 스크립트가 올바르게 로드되었는지 확인
- Console 창에서 Print 메시지 확인
- `UseScript`가 체크되어 있는지 확인

### Lua 스크립트 찾을 수 없음
- 스크립트 경로: `Scripts/CameraTransitionTest.lua`
- 파일명 대소문자 확인 (대소문자 구분함)
- ScriptName에 `.lua` 확장자는 제외

### 트랜지션이 너무 빠르거나 느림
- `Q`/`E` 키로 실시간 조절
- 또는 코드에서 `transitionDuration` 값 수정
- 추천값: 1.0 ~ 3.0초

---

## 💡 고급 사용 팁

### 1. 커스텀 카메라 위치 추가
`CameraTransitionTest.lua`의 `testPoints` 테이블에 새 항목 추가:

```lua
{
    name = "My Custom View",
    location = FVector(1000, 500, 300),
    rotation = FRotator(-30, 90, 0),
    ease = ECameraEaseType.EaseInOut
}
```

### 2. 오프셋 프리셋 추가
`CameraFollowTest.lua`의 `offsetPresets` 테이블에 추가:

```lua
{
    name = "First Person",
    offset = FVector(0, 0, 60),
    ease = ECameraEaseType.Linear
}
```

### 3. C++에서 커스텀 전환 만들기
```cpp
// 360도 회전 뷰
for (int i = 0; i < 360; i += 45) {
    float angle = i * (3.14159f / 180.0f);
    FVector loc(cos(angle) * 500, sin(angle) * 500, 200);
    FRotator rot(0, i, 0);

    CamMgr->StartTransitionToLocation(loc, rot, 1.0f,
        ECameraEaseType::SmoothStep);

    // Wait for transition to complete...
}
```

---

## 📝 참고 사항

- 모든 트랜지션은 **Slerp**(구면 선형 보간)을 사용하여 회전을 부드럽게 처리합니다
- 위치와 FOV는 **Lerp**(선형 보간)을 사용합니다
- 트랜지션 중에는 새로운 트랜지션이 이전 것을 덮어씁니다
- `StopCameraTransition()`으로 언제든지 중단 가능합니다
- 에디터 카메라(`UCamera`)와 게임 카메라(`UCameraComponent`) 모두 지원합니다

---

**즐거운 테스트 되세요!** 🎥✨
