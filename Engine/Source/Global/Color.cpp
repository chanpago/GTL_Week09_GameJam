#include "pch.h"
#include "Color.h"
#include "Core/Public/Archive.h"

// 내부 헬퍼
static uint8 HexCharToNibble(char C)
{
    if (C >= '0' && C <= '9') { return static_cast<uint8>(C - '0'); }
    if (C >= 'a' && C <= 'f') { return static_cast<uint8>(10 + (C - 'a')); }
    if (C >= 'A' && C <= 'F') { return static_cast<uint8>(10 + (C - 'A')); }
    return 0;
}

static uint8 HexPairToByte(char Hi, char Lo)
{
    return static_cast<uint8>((HexCharToNibble(Hi) << 4) | HexCharToNibble(Lo));
}

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

std::uint32_t FColor::ToPackedRGBA() const
{
    return (static_cast<std::uint32_t>(R)) |
        (static_cast<std::uint32_t>(G) << 8) |
        (static_cast<std::uint32_t>(B) << 16) |
        (static_cast<std::uint32_t>(A) << 24);
}

std::uint32_t FColor::ToPackedBGRA() const
{
    return (static_cast<std::uint32_t>(B)) |
        (static_cast<std::uint32_t>(G) << 8) |
        (static_cast<std::uint32_t>(R) << 16) |
        (static_cast<std::uint32_t>(A) << 24);
}

FColor FColor::Black() { return FColor(0, 0, 0, 255); }
FColor FColor::White() { return FColor(255, 255, 255, 255); }
FColor FColor::Red() { return FColor(255, 0, 0, 255); }
FColor FColor::Green() { return FColor(0, 255, 0, 255); }
FColor FColor::Blue() { return FColor(0, 0, 255, 255); }
FColor FColor::Transparent() { return FColor(0, 0, 0, 0); }

FColor FColor::FromHex(const char* InHex)
{
    if (InHex == nullptr)
    {
        return FColor::Black();
    }

    // # 스킵
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