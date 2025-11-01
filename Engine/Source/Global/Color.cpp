#include "pch.h"
#include "Color.h"
#include "Core/Public/Archive.h"

/*  ========================
    내부 헬퍼
    ========================*/

// 16진수 문자 하나를 0 ~ 15 숫자로 변환 (Nibble은 4비트짜리 값(0~15)을 의미)
static uint8 HexCharToNibble(char C)
{
    if (C >= '0' && C <= '9') { return static_cast<uint8>(C - '0'); }
    if (C >= 'a' && C <= 'f') { return static_cast<uint8>(10 + (C - 'a')); }
    if (C >= 'A' && C <= 'F') { return static_cast<uint8>(10 + (C - 'A')); }
    return 0;
}

/* 
    16진수 문자 두 개를 합쳐서 1바이트(0~255)로 변환하는 함수
	- 예: "AF" -> 0xAF (175), "FF" -> 0xFF (255)
*/
static uint8 HexPairToByte(char Hi, char Lo)
{
	// 실제로는 Hi * 16 + Lo 와 동일 (Hi를 4비트 밀면 16을 곱한것과 같음)
    return static_cast<uint8>((HexCharToNibble(Hi) << 4) | HexCharToNibble(Lo));
}
/*  ========================
    헬퍼 끝
    ========================*/

bool FColor::operator==(const FColor& Other) const
{
    return R == Other.R && G == Other.G && B == Other.B && A == Other.A;
}

bool FColor::operator!=(const FColor& Other) const
{
    return !(*this == Other);
}

void FColor::Set(uint8 InR, uint8 InG, uint8 InB, uint8 InA)
{
    R = InR;
    G = InG;
    B = InB;
    A = InA;
}
/* 
    R / G / B / A 네 개의 1바이트 값을 32비트 정수로 합치는 함수
    - 메모리나 GPU에 넘길 때 한 덩어리로 넘기기 좋게 만듬
*/
uint32 FColor::ToPackedRGBA() const
{
    return (static_cast<uint32>(R)) |
        (static_cast<uint32>(G) << 8) |
        (static_cast<uint32>(B) << 16) |
        (static_cast<uint32>(A) << 24);
}

uint32 FColor::ToPackedBGRA() const
{
    return (static_cast<uint32>(B)) |
        (static_cast<uint32>(G) << 8) |
        (static_cast<uint32>(R) << 16) |
        (static_cast<uint32>(A) << 24);
}

FColor FColor::Black() { return FColor(0, 0, 0, 255); }
FColor FColor::White() { return FColor(255, 255, 255, 255); }
FColor FColor::Red() { return FColor(255, 0, 0, 255); }
FColor FColor::Green() { return FColor(0, 255, 0, 255); }
FColor FColor::Blue() { return FColor(0, 0, 255, 255); }
FColor FColor::Transparent() { return FColor(0, 0, 0, 0); }

FColor FColor::FromHex(const char* InHex)
{
	// 들어온 문자가 null 포인터면 Black 반환
    if (InHex == nullptr)
    {
        return FColor::Black();
    }

	// #은 스킵, 포인터를 한 칸 미뤄서 진짜 16진수 부분부터 읽게 함.
    if (*InHex == '#')
    {
        ++InHex;
    }

    // 길이 계산
    size_t Length = 0;
	while (InHex[Length] != '\0')
    {
        ++Length;
    }

    if (Length == 6)
    {
        // RRGGBB
        uint8 R8 = HexPairToByte(InHex[0], InHex[1]);
        uint8 G8 = HexPairToByte(InHex[2], InHex[3]);
        uint8 B8 = HexPairToByte(InHex[4], InHex[5]);
        return FColor(R8, G8, B8, 255);
    }
    else if (Length == 8)
    {
        // RRGGBBAA
        uint8 R8 = HexPairToByte(InHex[0], InHex[1]);
        uint8 G8 = HexPairToByte(InHex[2], InHex[3]);
        uint8 B8 = HexPairToByte(InHex[4], InHex[5]);
        uint8 A8 = HexPairToByte(InHex[6], InHex[7]);
        return FColor(R8, G8, B8, A8);
    }

    // 지원하지 않는 형식이면 Black 반환
    return FColor::Black();
}

FArchive& operator<<(FArchive& Ar, FColor& Color)
{
    Ar << Color.R;
    Ar << Color.G;
    Ar << Color.B;
    Ar << Color.A;
    return Ar;
}