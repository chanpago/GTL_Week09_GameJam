#include "pch.h"
#include "Core/Public/AudioEngine.h"

#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/Viewport.h"

#ifndef FOURCC
#define FOURCC(c1, c2, c3, c4) ((c1) | ((c2) << 8) | ((c3) << 16) | ((c4) << 24))
#endif
#define FOURCC_WAVE FOURCC('W', 'A', 'V', 'E')
#define FOURCC_FMT FOURCC('f', 'm', 't', ' ')
#define FOURCC_DATA FOURCC('d', 'a', 't', 'a')

FAudioEngine& FAudioEngine::GetInstance()
{
    static FAudioEngine Instance;
    return Instance;
}

void FAudioEngine::Initialize()
{
    HRESULT hr = XAudio2Create(&Audio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("[FAudioEngine] Audio device create fail");
        return;
    }

    hr = Audio->CreateMasteringVoice(&MasterVoice);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("[FAudioEngine] MasterVoice create fail");
        Shutdown();
        return;
    }

    DWORD ChannelMask;
    hr = MasterVoice->GetChannelMask(&ChannelMask);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("[FAudioEngine] Get channerMask fail");
        Shutdown();
        return;
    }

    hr = X3DAudioInitialize(ChannelMask, X3DAUDIO_SPEED_OF_SOUND, X3DAudioHandle);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("[FAudioEngine] X3Daudio Init fail");
        Shutdown();
        return;
    }

    ZeroMemory(&Listener, sizeof(X3DAUDIO_LISTENER));
    // dx11은 z-forward, y-up이지만 우리 엔진은 x-forward, z-up 
    Listener.OrientFront = {1.0f, 0.0f, 0.0f};
    Listener.OrientTop = {0.0f, 0.0f, 1.0f};
    Listener.Position = {0.0f, 0.0f, 0.0f};

    PreLoadSound();

    UE_LOG_SUCCESS("[FAudioEngine] Initialize success");
}

void FAudioEngine::Shutdown()
{
    StopBGM();

    // 모든 SFX Voice 정리
    for (IXAudio2SourceVoice* Voice : SFXVoices)
    {
        if (Voice)
        {
            Voice->DestroyVoice();
        }
    }
    SFXVoices.clear();

    ActiveComponents.clear();
    SoundMap.clear();

    if (MasterVoice)
    {
        MasterVoice->DestroyVoice();
        MasterVoice = nullptr;
    }

    if (Audio)
    {
        Audio->Release();
        Audio = nullptr;
    }
}

FSoundData* FAudioEngine::GetSoundData(const path& FilePath)
{
    auto It = SoundMap.find(FilePath.wstring());
    if (It != SoundMap.end())
    {
        return &It->second;
    }

    if(LoadSound(FilePath))
    {
        return &SoundMap[FilePath.wstring()];
    }

    return nullptr;
}

void FAudioEngine::Tick(float DeltaTime, UCamera* Camera)
{
    const FVector& ListenerPosition = Camera->GetLocation();
    const FVector& ListenerForward = Camera->GetForward();
    const FVector& ListenerUp = Camera->GetUp();
    Listener.Position = {ListenerPosition.X, ListenerPosition.Y, ListenerPosition.Z};
    Listener.OrientFront = {ListenerForward.X, ListenerForward.Y, ListenerForward.Z};
    Listener.OrientTop = {ListenerUp.X, ListenerUp.Y, ListenerUp.Z};

    // 재생 완료된 SFX Voice 정리
    for (int i = SFXVoices.size() - 1; i >= 0; --i)
    {
        XAUDIO2_VOICE_STATE state;
        SFXVoices[i]->GetState(&state);

        // BuffersQueued가 0이면 재생 완료
        if (state.BuffersQueued == 0)
        {
            SFXVoices[i]->DestroyVoice();
            SFXVoices.erase(SFXVoices.begin() + i);
        }
    }

    // TODO ActiveComponent 순회하며 재생하는 로직 추가
}

void FAudioEngine::PlayBGM(const path& FilePath)
{
    if (BGMSourceVoice != nullptr && CurrentBGM->CurrentBGMPath == FilePath)
    {
        return;
    }

    StopBGM();

    CurrentBGM = GetSoundData(FilePath);
    if (!CurrentBGM)
    {
        UE_LOG_ERROR("[FAudioEngine] BGM not found");
        return;
    }

    HRESULT hr = Audio->CreateSourceVoice(&BGMSourceVoice, &CurrentBGM->WaveFormat);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("[FAudioEngine] BGM create fail");
        CurrentBGM = nullptr;
        return;
    }

    XAUDIO2_BUFFER buffer = {};
    CurrentBGM->FillXAudio2Buffer(buffer);
    // bgm 무한 반복
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    hr = BGMSourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        StopBGM();
        return;
    }

    hr = BGMSourceVoice->Start(0);
    if (FAILED(hr))
    {
        StopBGM();
    }

    CurrentBGM->CurrentBGMPath = FilePath;
}

void FAudioEngine::StopBGM()
{
    if (BGMSourceVoice)
    {
        BGMSourceVoice->Stop(0);
        BGMSourceVoice->FlushSourceBuffers();
        BGMSourceVoice->DestroyVoice();
        BGMSourceVoice = nullptr;
        CurrentBGM = nullptr;
    }
}

void FAudioEngine::PlaySFX(const path& FilePath, float Volume)
{
    FSoundData* SFXData = GetSoundData(FilePath);
    if (!SFXData)
    {
        UE_LOG_ERROR("[FAudioEngine] SFX not found: %s", FilePath.string().c_str());
        return;
    }

    IXAudio2SourceVoice* SFXVoice = nullptr;
    HRESULT hr = Audio->CreateSourceVoice(&SFXVoice, &SFXData->WaveFormat);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("[FAudioEngine] SFX voice create fail");
        return;
    }

    // 볼륨 설정
    SFXVoice->SetVolume(Volume);

    // 버퍼 제출 (loop 없음)
    XAUDIO2_BUFFER buffer = {};
    SFXData->FillXAudio2Buffer(buffer);
    buffer.LoopCount = 0; // 반복 없음

    hr = SFXVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        SFXVoice->DestroyVoice();
        return;
    }

    // 재생 시작
    hr = SFXVoice->Start(0);
    if (FAILED(hr))
    {
        SFXVoice->DestroyVoice();
        return;
    }

    // 리스트에 추가 (나중에 정리)
    SFXVoices.push_back(SFXVoice);
}

void FAudioEngine::PlaySoundComponent()
{
}

void FAudioEngine::StopSoundComponent()
{
}

void FAudioEngine::RegisterAudioComponent(UAudioComponent* Component)
{
    if (Component)
    {
        ActiveComponents.push_back(Component);
    }
}

void FAudioEngine::UnregisterAudioComponent(UAudioComponent* Component)
{
    auto It = std::find(ActiveComponents.begin(), ActiveComponents.end(), Component);
    if (It != ActiveComponents.end())
    {
        ActiveComponents.erase(It);
    }
}

bool FAudioEngine::LoadSound(const path& FilePath)
{
    std::ifstream FileStream(FilePath, std::ios::binary);

    if (!FileStream.is_open())
    {
        UE_LOG_ERROR("[FAudioEngine] Can't open file %s", FilePath.string().c_str());
        return false;
    }

    try
    {
        DWORD ChunkType;
        DWORD ChunkDataSize;
        DWORD FileType;
        FileStream.read(reinterpret_cast<char*>(&ChunkType), sizeof(DWORD));
        FileStream.read(reinterpret_cast<char*>(&ChunkDataSize), sizeof(DWORD));
        FileStream.read(reinterpret_cast<char*>(&FileType), sizeof(DWORD));

        if (ChunkType != FOURCC_RIFF || FileType != FOURCC_WAVE)
        {
            UE_LOG_ERROR("[FAudioEngine] Invalid .wav format");
            FileStream.close();
            return false;
        }
        
        FSoundData NewSoundData;
        DWORD ChunkSize = 0;
        while (FileStream.good())
        {
            FileStream.read(reinterpret_cast<char*>(&ChunkType), sizeof(DWORD));
            FileStream.read(reinterpret_cast<char*>(&ChunkSize), sizeof(DWORD));

            if (FileStream.eof()) break;

            if (ChunkType == FOURCC_FMT)
            {
                // 3-1. "fmt " 청크 (WAVEFORMATEX) 읽기
                // FileStream.read(reinterpret_cast<char*>(&NewSoundData.WaveFormat), sizeof(WAVEFORMATEX));
                // if (ChunkSize > sizeof(WAVEFORMATEX))
                // {
                //     FileStream.seekg(ChunkSize - sizeof(WAVEFORMATEX), std::ios::cur);
                // }
                if (ChunkSize <= sizeof(NewSoundData.WaveFormat))
                {
                    FileStream.read(reinterpret_cast<char*>(&NewSoundData.WaveFormat), ChunkSize);
                }
                else
                {
                    FileStream.read(reinterpret_cast<char*>(&NewSoundData.WaveFormat), sizeof(NewSoundData.WaveFormat));
                    FileStream.seekg(ChunkSize - sizeof(NewSoundData.WaveFormat), std::ios::cur);
                }
            }
            else if (ChunkType == FOURCC_DATA)
            {
                // 3-2. "data" 청크 (오디오 샘플) 읽기
                NewSoundData.DataSize = ChunkSize;
                NewSoundData.DataBuffer = std::make_unique<BYTE[]>(ChunkSize);
                FileStream.read(reinterpret_cast<char*>(NewSoundData.DataBuffer.get()), ChunkSize);

                // 필요한 청크를 모두 찾았으므로 루프 종료
                break;
            }
            else
            {
                // 모르는 청크는 건너뛰기
                FileStream.seekg(ChunkSize, std::ios::cur);
            }
        }
        
        // 4. 파일 핸들 닫기 (RAII - InFile 소멸자가 자동으로 호출)
        FileStream.close();

        // 5. 유효성 검사
        if (NewSoundData.DataSize == 0 || NewSoundData.DataBuffer == nullptr)
        {
            // TODO: "data" 청크를 찾지 못함 로깅
            return false;
        }

        // 6. 캐시에 저장
        SoundMap[FilePath.wstring()] = std::move(NewSoundData);
    }
    catch (const std::exception& e)
    {
        if (FileStream.is_open())
        {
            FileStream.close();            
        }
        return false;
    }

    return true;
}

void FAudioEngine::PreLoadSound()
{
    // 해당 경로의 모든 audio 로드
    const path Directory = "Data/Audio/";
    if (filesystem::exists(Directory) && filesystem::is_directory(Directory))
    {
        for (const auto& Entry : filesystem::recursive_directory_iterator(Directory))
        {
            if (Entry.is_regular_file()&& Entry.path().extension() == ".wav")
            {
                path Path = Entry.path();
                LoadSound(Path);
            }
        }
    }
}
