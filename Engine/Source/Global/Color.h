#pragma once
#include <cstdint>

struct FArchive;

/**
 * @brief 8비트 정수 기반 색상 (채널 순서 RGBA)
 */
struct FColor
{
    uint8 R;
    uint8 G;
    uint8 B;
    uint8 A;

    // 기본: 불투명 블랙
    constexpr FColor()
        : R(0), G(0), B(0), A(255)
    {
    }

    // 표준 RGBA 생성자
    constexpr FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255)
        : R(InR), G(InG), B(InB), A(InA)
    {
    }

    // 동일성 비교
    bool operator==(const FColor& Other) const;
    bool operator!=(const FColor& Other) const;

    // 값 설정
    void Set(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255);

    // 32비트 패킹
    std::uint32_t ToPackedRGBA() const; // R | G<<8 | B<<16 | A<<24
    std::uint32_t ToPackedBGRA() const; // B | G<<8 | R<<16 | A<<24 (D3D BGRA와 호환)

    // 헬퍼: 네임드 컬러
    static FColor Black();
    static FColor White();
    static FColor Red();
    static FColor Green();
    static FColor Blue();
    static FColor Transparent();

    // 헬퍼: "#RRGGBB" 또는 "#RRGGBBAA" 파싱
    static FColor FromHex(const char* InHex);
};