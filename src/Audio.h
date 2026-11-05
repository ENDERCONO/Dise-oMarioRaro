#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <vector>
#include <string>

#define DEFAULT_MUSIC_FADE_TIME 2.0f

struct _Mix_Music;

class Audio : public Module
{
public:

	Audio();

	// Destructor
	virtual ~Audio();

	// Called before render is available
	bool Awake();

	// Called before quitting
	bool CleanUp();

	// Play a music file
	bool PlayMusic(const char* path, float fadeTime = DEFAULT_MUSIC_FADE_TIME);

	// Load a WAV in memory
	int LoadFx(const char* path);

	// Play a previously loaded WAV
	bool PlayFx(int fx, int repeat = 0);

    bool  PlayFxTracked(int fx);
    void  StopFxTracked(float fadeTime = 0.05f);   // seconds
    void  SetFxTrackedSpeed(float ratio);          // 1.0 = normal
    float GetFxDuration(int fx) const;             // seconds

    bool Update(float dt);

	// Volume control
	void SetMusicVolume(float volume); // 0.0f - 1.0f
	void SetSFXVolume(float volume);   // 0.0f - 1.0f

private:

    struct SoundData {
        SDL_AudioSpec spec{};  // source format
        Uint8* buf{ nullptr };
        Uint32 len{ 0 };  // bytes
    };

    // Device and default output format
    SDL_AudioDeviceID device_{ 0 };
    SDL_AudioSpec     device_spec_{};

    // Streams
    SDL_AudioStream* music_stream_{ nullptr }; // for background music (single)
    SDL_AudioStream* sfx_stream_{ nullptr };   // simple shared SFX stream
    SDL_AudioStream* tracked_stream_{ nullptr }; // controllable SFX (jump)
    float tracked_fade_total_ = 0.0f;
    float tracked_fade_left_ = 0.0f;

    // Loaded sounds
    SoundData music_data_{};
    std::vector<SoundData> sfx_; // 1-based indexing outwardly

    // Volume control
    float music_volume_ = 1.0f; // 0.0 = mute, 1.0 = full
    float sfx_volume_ = 1.0f;

    // helpers
    bool LoadWavFile(const char* path, SoundData& out);
    void FreeSound(SoundData& s);
    bool EnsureDeviceOpen();
    bool EnsureStreams();
};
