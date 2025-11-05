#pragma once

#include "Component/Public/PrimitiveComponent.h"
#include "Global/Vector.h"

UCLASS()
class UBillBoardComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillBoardComponent, UPrimitiveComponent)

public:
	UBillBoardComponent();
	virtual ~UBillBoardComponent() override;

	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void FaceCamera(const FVector& CameraForward);

	UTexture* GetSprite() const;
	void SetSprite(UTexture* Sprite);

	FVector4 GetSpriteTint() const { return SpriteTint; }
	void SetSpriteTint(const FVector4& InTint);
	void SetSpriteTint(const FVector& InTint, float Alpha = 1.0f);

	UClass* GetSpecificWidgetClass() const override;

	static const FRenderState& GetClassDefaultRenderState();

	UObject* Duplicate() override;

private:
	UTexture* Sprite = nullptr;
	FVector4 SpriteTint = FVector4::OneVector();

// Screen Size Section
public:
	bool IsScreenSizeScaled() const { return bScreenSizeScaled; }
	float GetScreenSize() const { return ScreenSize; }
	void SetScreenSizeScaled(bool bEnable, float InScreenSize = 0.1f)
	{
		bScreenSizeScaled = bEnable;
		ScreenSize = InScreenSize;
	}

private:
	bool bScreenSizeScaled = false;
	float ScreenSize = 0.1f;

// SubUV Animation Section
public:
	bool IsSubUVAnimationEnabled() const { return bSubUVAnimationEnabled; }
	uint32_t GetSubUVGridColumns() const { return SubUVGridColumns; }
	uint32_t GetSubUVGridRows() const { return SubUVGridRows; }
	float GetSubUVAnimationSpeed() const { return SubUVAnimationSpeed; }

	void SetSubUVAnimation(bool bEnable, uint32_t InColumns = 1, uint32_t InRows = 1, float InSpeed = 10.0f)
	{
		bSubUVAnimationEnabled = bEnable;
		SubUVGridColumns = InColumns;
		SubUVGridRows = InRows;
		SubUVAnimationSpeed = InSpeed;
	}

	// 상대 시간 모드 (각 빌보드가 독립적으로 애니메이션)
	void SetUseRelativeTime(bool bEnable) { bUseRelativeTime = bEnable; }
	bool IsUsingRelativeTime() const { return bUseRelativeTime; }
	float GetSpawnTime() const { return SpawnTime; }
	void SetSpawnTime(float InTime) { SpawnTime = InTime; }

private:
	bool bSubUVAnimationEnabled = false;
	uint32_t SubUVGridColumns = 1;
	uint32_t SubUVGridRows = 1;
	float SubUVAnimationSpeed = 10.0f; // Frames per second
	bool bUseRelativeTime = false; // true면 SpawnTime 기준으로 상대 시간 사용
	float SpawnTime = 0.0f; // 빌보드가 생성된 게임 시간
};
