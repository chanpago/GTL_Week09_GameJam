#pragma once
#include "editor/Public/Camera.h"

struct FSoundData
{
    WAVEFORMATEX WaveFormat;
    std::unique_ptr<BYTE[]> DataBuffer;
    uint32 DataSize = 0;
    path CurrentBGMPath;

    FSoundData() = default;
    FSoundData(const FSoundData&) = delete;
    FSoundData& operator=(const FSoundData&) = delete;
    FSoundData(FSoundData&&) noexcept = default;
    FSoundData& operator=(FSoundData&&) noexcept = default;
    void FillXAudio2Buffer(XAUDIO2_BUFFER& OutBuffer) const
    {
        ZeroMemory(&OutBuffer, sizeof(XAUDIO2_BUFFER));
        OutBuffer.AudioBytes = DataSize;
        OutBuffer.pAudioData = DataBuffer.get();
        OutBuffer.Flags = XAUDIO2_END_OF_STREAM;
    }
};

class UAudioComponent;

class FAudioEngine
{
public:
    static FAudioEngine& GetInstance();

    void Initialize();
    void Shutdown();    
    FSoundData* GetSoundData(const path& SoundName);

    void Tick(float DeltaTime, UCamera* Camera);

    // Background music 재생
    void PlayBGM(const path& FilePath);
    void StopBGM();

    // 효과음 재생 (one-shot, 중첩 가능)
    void PlaySFX(const path& FilePath, float Volume = 1.0f);

    // 사운드컴포넌트 재생
    void PlaySoundComponent();
    void StopSoundComponent();

    // 컴포넌트 등록
    void RegisterAudioComponent(UAudioComponent* Component);
    void UnregisterAudioComponent(UAudioComponent* Component);

    IXAudio2* GetXAudio2Device() const { return Audio; }

private:
   
    bool LoadSound(const path& FilePath);
    void PreLoadSound();

private:
    IXAudio2* Audio = nullptr;
    IXAudio2MasteringVoice* MasterVoice = nullptr;
    TMap<std::wstring, FSoundData> SoundMap;

    IXAudio2SourceVoice* BGMSourceVoice = nullptr;
    FSoundData* CurrentBGM = nullptr;

    // 효과음용 SourceVoice 리스트 (재생 완료 후 정리)
    TArray<IXAudio2SourceVoice*> SFXVoices;

    TArray<UAudioComponent*> ActiveComponents;
    X3DAUDIO_HANDLE X3DAudioHandle;
    X3DAUDIO_LISTENER Listener;
    
private:
    FAudioEngine() {};
    ~FAudioEngine() {};
    FAudioEngine(const FAudioEngine&) = delete;
    FAudioEngine& operator=(const FAudioEngine&) = delete;
    FAudioEngine(FAudioEngine&&) = delete;
    FAudioEngine& operator=(FAudioEngine&&) = delete;
    
};