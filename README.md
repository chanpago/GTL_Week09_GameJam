# FutureEngine - Recent Features & Improvements

## 📅 업데이트 히스토리 (2025.10.23 ~ 2025.10.30)

본 문서는 2025년 10월 23일 ~ 10월 30일 주간 (WEEK08) 동안 FutureEngine에 추가된 주요 기능 및 개선사항을 기술합니다.

**주간 주제**: Shadow Mapping & Multiple Light Sources

---

## 🎯 주요 기능

### 1. Shadow Mapping System (PSM - Perspective Shadow Mapping)

**구현 날짜**: 2025.10.23 ~ 10.30
**핵심 기술**: Light Perspective Rendering, Depth Map, Bias Handling

#### 개요
Light 관점에서 scene을 렌더링하여 depth map을 생성하고, 이를 활용해 실시간 그림자를 렌더링하는 시스템입니다. Directional Light, Point Light, Spot Light 총 3가지 광원 타입을 지원하며, 각 광원이 동시에 존재하는 multi-light 환경에 대응합니다.

#### Light Types & Shadow Map Architecture

**1. Directional Light (Orthographic Projection)**
```cpp
// 직교 투영 행렬 생성 (태양광 등 평행광)
FMatrix LightViewMatrix = FMatrix::LookAtLH(LightPosition, LightPosition + LightDirection, FVector(0, 0, 1));
FMatrix LightProjMatrix = FMatrix::OrthographicLH(OrthoWidth, OrthoHeight, NearZ, FarZ);

// Shadow Map: Single 2D Texture (1024×1024 ~ 4096×4096)
ID3D11Texture2D* DirectionalLightShadowMap;
```

**2. Point Light (Cube Map)**
```cpp
// 6방향 투영 (±X, ±Y, ±Z)
FMatrix CubeFaceViewMatrices[6];
CubeFaceViewMatrices[0] = FMatrix::LookAtLH(LightPos, LightPos + FVector(1, 0, 0), FVector(0, 1, 0));  // +X
CubeFaceViewMatrices[1] = FMatrix::LookAtLH(LightPos, LightPos + FVector(-1, 0, 0), FVector(0, 1, 0)); // -X
// ... (±Y, ±Z)

// Shadow Map: Cube Texture (TextureCube)
ID3D11Texture2D* PointLightShadowCubeMap;
```

**3. Spot Light (Perspective Projection)**
```cpp
// 원뿔형 투영 행렬
FMatrix LightViewMatrix = FMatrix::LookAtLH(LightPosition, LightPosition + LightDirection, FVector(0, 0, 1));
FMatrix LightProjMatrix = FMatrix::PerspectiveFovLH(OuterConeAngle, 1.0f, NearZ, FarZ);

// Shadow Map: Single 2D Texture
ID3D11Texture2D* SpotLightShadowMap;
```

#### Shadow Map Pass Pipeline

**Pass 1: Depth Map Generation (Light Perspective)**
```cpp
void URenderer::RenderShadowMapPass(ULightComponent* Light)
{
    // Render Target: Shadow Map (Depth만 기록)
    ID3D11DepthStencilView* ShadowDSV = Light->GetShadowDepthStencilView();
    DeviceContext->ClearDepthStencilView(ShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    DeviceContext->OMSetRenderTargets(0, nullptr, ShadowDSV);  // Color 출력 없음

    // Viewport: Shadow Map 해상도
    D3D11_VIEWPORT ShadowViewport = {};
    ShadowViewport.Width = static_cast<float>(ShadowResolution);
    ShadowViewport.Height = static_cast<float>(ShadowResolution);
    ShadowViewport.MinDepth = 0.0f;
    ShadowViewport.MaxDepth = 1.0f;
    DeviceContext->RSSetViewports(1, &ShadowViewport);

    // Light View/Proj Matrix
    FMatrix LightViewMatrix = Light->GetLightViewMatrix();
    FMatrix LightProjMatrix = Light->GetLightProjectionMatrix();

    // 모든 CastShadow Actor 렌더링
    for (AActor* Actor : World->GetAllActors())
    {
        if (!Actor->bCastShadows) continue;

        for (UStaticMeshComponent* MeshComp : Actor->GetComponents<UStaticMeshComponent>())
        {
            // Simple Depth-Only Shader (VertexShader만 사용)
            FMatrix WorldMatrix = MeshComp->GetWorldMatrix();
            FMatrix WVP = WorldMatrix * LightViewMatrix * LightProjMatrix;

            // Constant Buffer 업데이트
            DepthPassCB.WVP = WVP;
            UpdateConstantBuffer(DepthPassConstantBuffer, &DepthPassCB);

            // Draw Call
            DrawIndexed(MeshComp->GetIndexCount());
        }
    }
}
```

**Pass 2: Scene Rendering with Shadow Sampling**
```hlsl
// UberLit.hlsl - Pixel Shader
float4 mainPS(PSInput input) : SV_Target
{
    // Light-Space Position 계산
    float4 LightSpacePos = mul(float4(input.WorldPos, 1.0f), LightViewProj);
    LightSpacePos.xyz /= LightSpacePos.w;  // Perspective Division (NDC)

    // NDC → UV 변환 ([-1, 1] → [0, 1])
    float2 ShadowUV = float2(LightSpacePos.x, -LightSpacePos.y) * 0.5f + 0.5f;
    float PixelDepth = LightSpacePos.z;

    // Shadow Map 샘플링
    float ShadowMapDepth = ShadowMapTexture.Sample(ShadowSampler, ShadowUV).r;

    // Shadow Test (Bias 적용)
    float ShadowBias = ShadowBiasConstant + ShadowBiasSlopeScale * max(abs(ddx(PixelDepth)), abs(ddy(PixelDepth)));
    float ShadowFactor = (PixelDepth - ShadowBias > ShadowMapDepth) ? 0.0f : 1.0f;

    // Lighting 계산
    float3 Lighting = DiffuseLighting + SpecularLighting;
    Lighting *= ShadowFactor;  // 그림자 영역은 조명 차단

    return float4(Lighting, 1.0f);
}
```

#### Shadow Artifacts & Solutions

**1. Shadow Acne (Self-Shadowing)**
- **원인**: Depth precision 부족으로 인한 false-positive shadow
- **해결**: Constant Bias + Slope-Scaled Bias
```cpp
class ULightComponent
{
    float ShadowBias = 0.001f;         // Constant Bias (깊이 오프셋)
    float ShadowSlopeBias = 2.0f;      // Slope-Scaled Bias (경사 보정)
};

// Shader
float ShadowBias = ShadowBiasConstant + ShadowBiasSlopeScale * max(abs(ddx(PixelDepth)), abs(ddy(PixelDepth)));
```

**2. Peter Panning (Light Leaking)**
- **원인**: Bias가 너무 커서 그림자가 물체에서 분리됨
- **해결**: Bias 값 최소화, Back-face culling 사용
```cpp
// Shadow Map Pass에서 Front-face만 렌더링
RasterizerState.CullMode = D3D11_CULL_BACK;  // Back-face cull
```

**3. Shadow Map Resolution 부족**
- **원인**: Shadow Map 해상도가 낮아 계단 현상 발생
- **해결**: 해상도 증가, PCF 필터링
```cpp
float ShadowResolutionScale = 1.0f;  // UI에서 조절 가능 (0.5 ~ 2.0)
int ShadowMapSize = static_cast<int>(BaseShadowMapSize * ShadowResolutionScale);
```

#### Multiple Light Shadows Handling

**문제점**: n개의 광원이 동시에 존재할 때 그림자 중첩 이슈
```cpp
// 잘못된 접근 - 마지막 광원의 그림자만 적용됨
for (Light in Lights)
{
    ShadowFactor = CalculateShadow(Light);
    FinalColor *= ShadowFactor;  // ❌ 덮어쓰기
}
```

**올바른 접근**: Per-Light Shadow 누적
```hlsl
// Pixel Shader - Multiple Lights
float3 FinalLighting = AmbientLighting;

for (int i = 0; i < NumLights; ++i)
{
    // Light 개별 조명 계산
    float3 LightContribution = CalculateLighting(Lights[i], WorldPos, Normal);

    // Light 개별 그림자 계산
    if (Lights[i].bCastShadows)
    {
        float ShadowFactor = CalculateShadow(Lights[i], WorldPos);
        LightContribution *= ShadowFactor;
    }

    FinalLighting += LightContribution;
}

return float4(FinalLighting, 1.0f);
```

---

### 2. PCF (Percentage Closer Filtering)

**구현 날짜**: 2025.10.25
**목적**: Shadow edge의 계단 현상(aliasing) 완화

#### 원리
Shadow Map의 단일 샘플 대신 주변 픽셀들을 샘플링하여 평균을 계산함으로써 부드러운 그림자 경계 생성

#### 구현
```hlsl
// PCF 3×3 Kernel
float PCF_ShadowFactor(Texture2D ShadowMap, SamplerState Sampler, float2 UV, float PixelDepth, float Bias)
{
    float ShadowSum = 0.0f;
    float TexelSize = 1.0f / ShadowMapResolution;

    // 3×3 샘플링 (9개 샘플)
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float2 Offset = float2(x, y) * TexelSize;
            float SampleDepth = ShadowMap.Sample(Sampler, UV + Offset).r;
            ShadowSum += (PixelDepth - Bias > SampleDepth) ? 0.0f : 1.0f;
        }
    }

    return ShadowSum / 9.0f;  // 평균
}
```

**비용 분석:**
- 1×1 샘플링: 1회 texture fetch
- 3×3 PCF: 9회 texture fetch (9배 비용)
- 5×5 PCF: 25회 texture fetch (고품질, 높은 비용)

**최적화 기법:**
```hlsl
// Hardware PCF (SamplerComparisonState 사용)
SamplerComparisonState ShadowSampler
{
    Filter = COMPARISON_MIN_MAG_MIP_LINEAR;  // HW-accelerated PCF
    ComparisonFunc = LESS;
};

float ShadowFactor = ShadowMap.SampleCmp(ShadowSampler, UV, PixelDepth - Bias);  // 1회 호출로 자동 PCF
```

---

### 3. VSM (Variance Shadow Map)

**구현 날짜**: 2025.10.26
**목적**: PCF보다 빠른 필터링, 부드러운 그림자

#### 원리
Shadow Map에 Depth와 Depth² 값을 저장하고, Chebyshev's Inequality를 활용해 확률 기반 그림자 계산

**Shadow Map Pass:**
```hlsl
// Depth + Depth² 출력
float2 DepthPS(PSInput input) : SV_Target
{
    float Depth = input.Position.z;
    return float2(Depth, Depth * Depth);  // (μ, μ²)
}
```

**Shadow Sampling:**
```hlsl
float VSM_ShadowFactor(Texture2D VSMTexture, float2 UV, float PixelDepth)
{
    float2 Moments = VSMTexture.Sample(LinearSampler, UV).rg;
    float Mean = Moments.x;       // E[X]
    float MeanSq = Moments.y;     // E[X²]

    // Variance: σ² = E[X²] - E[X]²
    float Variance = MeanSq - Mean * Mean;
    Variance = max(Variance, 0.00001f);  // Numerical stability

    // Chebyshev's Inequality: P(X >= t) <= σ² / (σ² + (t - μ)²)
    float Delta = PixelDepth - Mean;
    float PMax = Variance / (Variance + Delta * Delta);

    // Sharpen (Light Bleeding 완화)
    float ShadowSharpen = 0.5f;  // UI 파라미터
    PMax = smoothstep(ShadowSharpen, 1.0f, PMax);

    return (PixelDepth <= Mean) ? 1.0f : PMax;
}
```

**장점:**
- Linear filtering 가능 (Mipmap, Anisotropic filtering 사용 가능)
- 큰 필터 커널에서도 일정한 비용 (texture fetch 1회)

**단점:**
- Light Bleeding: 두꺼운 occluder 뒤에서 밝은 영역 발생
- Depth² 오버플로우 위험 (float precision 이슈)

---

### 4. Shadow Atlas

**구현 날짜**: 2025.10.27
**목적**: 여러 광원의 Shadow Map을 단일 Texture에 효율적으로 배치하여 리소스 사용량 감소

#### 아키텍처
```
Shadow Atlas Texture (4096×4096)
┌─────────────┬─────────────┬─────────────┬─────────────┐
│ DirLight 0  │ SpotLight 0 │ SpotLight 1 │ SpotLight 2 │
│ (2048×2048) │ (1024×1024) │ (1024×1024) │ (1024×1024) │
├─────────────┼─────────────┼─────────────┼─────────────┤
│ SpotLight 3 │ SpotLight 4 │ (Empty)     │ (Empty)     │
│ (1024×1024) │ (1024×1024) │             │             │
└─────────────┴─────────────┴─────────────┴─────────────┘
```

#### 구현
```cpp
struct FShadowAtlasSlot
{
    FIntRect Region;        // Atlas 내 영역 (x, y, width, height)
    ULightComponent* Owner; // 슬롯 소유 Light
};

class FShadowAtlas
{
public:
    void AllocateSlot(ULightComponent* Light, int32 RequestedSize);
    void RenderShadowToSlot(ULightComponent* Light);

    ID3D11Texture2D* GetAtlasTexture() { return AtlasTexture; }

private:
    ID3D11Texture2D* AtlasTexture;           // 4096×4096 Depth Texture
    TArray<FShadowAtlasSlot> AllocatedSlots; // 할당된 슬롯 목록
    TArray<FIntRect> FreeRegions;            // 남은 공간
};
```

**UV 변환:**
```hlsl
// Light-Space NDC [-1, 1] → Atlas UV [0, 1]
float2 LocalUV = LightSpacePos.xy * 0.5f + 0.5f;
LocalUV.y = 1.0f - LocalUV.y;

// Atlas Slot으로 변환
float2 AtlasUV = AtlasOffset + LocalUV * AtlasScale;
// AtlasOffset: Slot 시작 위치 (예: (0.5, 0.0) for slot [2048, 0])
// AtlasScale: Slot 크기 비율 (예: (0.25, 0.25) for 1024×1024 in 4096×4096)
```

**장점:**
- Shader Resource View 개수 감소 (n개 Light → 1개 Atlas)
- GPU 메모리 효율 증가 (fragmentation 감소)
- Draw Call batching 가능

**단점:**
- Atlas 공간 부족 시 동적 재할당 필요
- 큰 Shadow Map(4096×4096)은 Atlas에 배치 불가

---

### 5. Editor UI & Debugging Tools

**구현 날짜**: 2025.10.28 ~ 10.30

#### Light Property Panel Enhancements
```cpp
// Light Component Inspector
ImGui::Text("Shadow Settings");
ImGui::Checkbox("Cast Shadows", &Light->bCastShadows);
ImGui::SliderFloat("Resolution Scale", &Light->ShadowResolutionScale, 0.5f, 2.0f);
ImGui::SliderFloat("Shadow Bias", &Light->ShadowBias, 0.0f, 0.01f);
ImGui::SliderFloat("Slope Bias", &Light->ShadowSlopeBias, 0.0f, 5.0f);
ImGui::SliderFloat("Shadow Sharpen (VSM)", &Light->ShadowSharpen, 0.0f, 1.0f);

// Shadow Map Visualization
if (Light->bCastShadows && Light->ShadowMapSRV)
{
    ImGui::Text("Shadow Depth Map:");
    ImGui::Image(Light->ShadowMapSRV, ImVec2(256, 256));
}
```

#### Override Camera with Light Perspective
```cpp
// 'O' 키 또는 UI 버튼으로 토글
if (ImGui::Button("View from Light"))
{
    Editor->OverrideCameraWithLight(SelectedLight);
}

void UEditor::OverrideCameraWithLight(ULightComponent* Light)
{
    if (!Light) return;

    UCamera* EditorCamera = GetEditorCamera();
    EditorCamera->SetViewMatrix(Light->GetLightViewMatrix());
    EditorCamera->SetProjectionMatrix(Light->GetLightProjectionMatrix());

    bIsLightViewOverride = true;
    OverriddenLight = Light;
}
```

#### ShowFlag & Stat System
```cpp
// ShowFlags (콘솔 명령어)
show shadows          // 그림자 렌더링 ON/OFF
show shadowmaps       // Shadow Map 오버레이 표시
viewmode unlit        // Unlit 모드 (그림자 없음)

// Stat (통계 정보)
stat shadow
  - Directional Lights: 1
  - Spot Lights: 3
  - Point Lights: 2
  - Shadow Map Memory: 48 MB
  - Shadow Pass Draw Calls: 156
  - PCF Samples: 9 (3×3 kernel)

stat gpu
  - ShadowMapPass: 2.3 ms
  - PCF Filtering: 1.1 ms
  - VSM Generation: 0.8 ms
```

---

### 6. Pilot Mode & UI Enhancements

**구현 날짜**: 2025.10.30
**PR**: #13 (feature/viewport-actor_view_override)

에디터 카메라가 선택된 Actor의 Transform을 실시간으로 따라가는 Pilot Mode 기능과 관련 UI 개선사항이 추가되었습니다. `Alt + G` 단축키 또는 UI 버튼으로 토글 가능하며, ViewType 드롭다운에 조종 중인 Actor 이름 표시 및 Eject 버튼(△ 아이콘)이 추가되었습니다.

**주요 기능:**
- Actor Transform 실시간 동기화
- Pilot Mode 전용 UI (Dynamic button width, Text truncation)
- Dangling pointer 버그 수정 (static FString 캐싱)

---

### 7. Selection Outline System

**구현 날짜**: 2025.10.30

Stencil Buffer 기반 Two-Pass 렌더링으로 선택된 Actor 주변에 Unreal Engine 스타일 주황색 외곽선을 렌더링하는 시스템입니다.

**Pass 1**: Stencil Write (Color 출력 없음, PixelShader = nullptr)
**Pass 2**: Full-screen Quad로 8방향 edge detection 후 외곽선 렌더링

---

### 8. Camera & Utility Improvements

**구현 날짜**: 2025.10.30

- **Camera Movement**: Q/E 키를 camera-local space → world-space Z-axis 이동으로 변경 (Unreal Engine과 동일)
- **Utility Functions**: `Lerp`, `Clamp` 템플릿 함수를 `Global/Function.h`로 이동 (프로젝트 전역 사용)

---

## 🔧 버그 수정 및 개선사항

### Shadow System Improvements

**1. VSM (Variance Shadow Map) 버그 수정**
- Directional Light VSM 계산 오류 수정 (커밋: 351d47c)
- Shadow Sharpen 파라미터 VSM 연동 (커밋: 2250eed)
- SAVSM (Summed Area Variance Shadow Map) 샤프닝 수정 (커밋: 3c23dd3)

**2. Point Light Shadow Cube Map Seam 해결**
- Cube Shadow Map 6면 경계선(seam) 아티팩트 완화 (커밋: 4461944)
- 인접 face 간 depth 샘플링 블렌딩 개선

**3. Shadow Stat Overlay**
- Shadow Map 품질 실시간 모니터링 UI (커밋: 366faaa)
- Cascade별 해상도 표시, GPU 메모리 사용량 추적

### Light System Enhancements

**1. Light Component Icon (6535dba)**
- Directional/Point/Spot Light 고유 아이콘 추가
- 에디터에서 Light 타입 구분 용이

**2. Directional Light Look At (d9be0aa)**
- Directional Light Forward Vector 계산 정확도 향상
- Shadow Map 렌더링 시 올바른 방향 보장

### Editor Improvements

**1. Gizmo System Refactoring**
- Center Gizmo 추가 (3축 동시 이동) (커밋: 7213a52)
- Translation/Rotation/Scale 로직 분리 (커밋: c752e4f)
- Orthographic 뷰 Rotation Gizmo 수정 (커밋: 5a6ab0e)

**2. Picking & Focus**
- HitProxy 기반 선택 정확도 향상 (커밋: 1bc9bcf)
- Orthographic 뷰 Focus(F 키) 동작 개선 (커밋: d0c258a)

---

## 👥 Contributors

**개발 기간 (WEEK08)**: 2025.10.23 ~ 2025.10.30
**주제**: Shadow Mapping & Multiple Light Sources

---

## 📜 핵심 키워드 (Week 08)

**Shadow Techniques**
- PSM (Perspective Shadow Mapping)
- PCF (Percentage Closer Filtering)
- VSM (Variance Shadow Map)
- Shadow Atlas
- Cascade Shadow Map (미구현, 향후 계획)

**Light Types**
- Directional Light (Orthographic Projection)
- Point Light (Cube Map, 6-face rendering)
- Spot Light (Perspective Projection)

**Artifacts & Solutions**
- Shadow Acne → Constant Bias + Slope-Scaled Bias
- Peter Panning → Bias 최소화 + Back-face culling
- Light Bleeding (VSM) → Shadow Sharpen parameter
- Cube Map Seam → Inter-face blending

**Editor Tools**
- Light Perspective Override (카메라를 Light 시점으로 전환)
- Shadow Map Visualization (Depth Map 미리보기)
- ShowFlag & Stat System (성능 모니터링)
