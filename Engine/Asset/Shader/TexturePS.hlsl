#include "TextureVS.hlsl"

cbuffer MaterialConstants : register(b2)
{
    float4 Ka;		// Ambient color
    float4 Kd;		// Diffuse color
    float4 Ks;		// Specular color
    float Ns;		// Specular exponent
    float Ni;		// Index of refraction
    float D;		// Dissolve factor
    uint MaterialFlags;	// Which textures are available (bitfield)
    float Time;
    uint SubUVGridColumns;
    uint SubUVGridRows;
    float SubUVAnimationSpeed;
};

Texture2D DiffuseTexture : register(t0);	// map_Kd
Texture2D AmbientTexture : register(t1);	// map_Ka
Texture2D SpecularTexture : register(t2);	// map_Ks
Texture2D NormalTexture : register(t3);		// map_Ns
Texture2D AlphaTexture : register(t4);		// map_d
Texture2D BumpTexture : register(t5);		// map_Bump

SamplerState SamplerWrap : register(s0);

// Material flags
#define HAS_DIFFUSE_MAP	 (1 << 0)
#define HAS_AMBIENT_MAP	 (1 << 1)
#define HAS_SPECULAR_MAP (1 << 2)
#define HAS_NORMAL_MAP	 (1 << 3)
#define HAS_ALPHA_MAP	 (1 << 4)
#define HAS_BUMP_MAP	 (1 << 5)
#define HAS_SUBUV_ANIMATION (1 << 6)

struct PS_OUTPUT
{
    float4 SceneColor : SV_Target0;
    float4 NormalData : SV_Target1;
};

PS_OUTPUT mainPS(PS_INPUT Input) : SV_TARGET
{
    PS_OUTPUT Output;

    float4 FinalColor = float4(0.f, 0.f, 0.f, 1.f);
    float2 UV = Input.Tex;

    // SubUV Animation
    if (MaterialFlags & HAS_SUBUV_ANIMATION)
    {
        uint TotalFrames = SubUVGridColumns * SubUVGridRows;

        // Calculate current frame index based on time and animation speed
        float FrameTime = Time * SubUVAnimationSpeed;
        uint CurrentFrame = min(uint(FrameTime), TotalFrames - 1); // 마지막 프레임에서 멈춤 (반복 안함)

        // Calculate grid position (row and column)
        uint Row = CurrentFrame / SubUVGridColumns;
        uint Col = CurrentFrame % SubUVGridColumns;

        // Calculate cell size
        float CellWidth = 1.0f / float(SubUVGridColumns);
        float CellHeight = 1.0f / float(SubUVGridRows);

        // Apply UV offset and scale
        UV.x = (UV.x * CellWidth) + (float(Col) * CellWidth);
        UV.y = (UV.y * CellHeight) + (float(Row) * CellHeight);
    }

    // Base diffuse color
    float4 DiffuseColor = Kd;
    if (MaterialFlags & HAS_DIFFUSE_MAP)
    {
        DiffuseColor *= DiffuseTexture.Sample(SamplerWrap, UV);
        FinalColor.a = DiffuseColor.a;
    }

    // Ambient contribution
    float4 AmbientColor = Ka;
    if (MaterialFlags & HAS_AMBIENT_MAP)
    {
        AmbientColor *= AmbientTexture.Sample(SamplerWrap, UV);
    }

    FinalColor.rgb = DiffuseColor.rgb;

    // Alpha handling
    if (MaterialFlags & HAS_ALPHA_MAP)
    {
        float alpha = AlphaTexture.Sample(SamplerWrap, UV).r;
        FinalColor.a = D;
        FinalColor.a *= alpha;
    }

    // SubUV 애니메이션일 때 어두운 픽셀 제거 (검은 배경 제거)
    if (MaterialFlags & HAS_SUBUV_ANIMATION)
    {
        // RGB 밝기 계산 (luminance)
        float luminance = dot(FinalColor.rgb, float3(0.299f, 0.587f, 0.114f));

        // 밝기가 일정 threshold 이하면 discard (어두운 배경 제거)
        if (luminance < 0.1f)
        {
            discard;
        }
    }

    // Discard fully transparent pixels to prevent depth write
    if (FinalColor.a < 0.01f)
    {
        discard;
    }

    Output.SceneColor = FinalColor;
    float3 EncodedNormal = normalize(Input.WorldNormal) * 0.5f + 0.5f;
    Output.NormalData = float4(EncodedNormal, 1.0f);
	
    return Output;
}
