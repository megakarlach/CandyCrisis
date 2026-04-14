// music.c

#include "main.h"
#include "music.h"
#include "gworld.h"
#include "gameticks.h"
#include "soundfx.h"
#include "graphics.h"
#include "support/cmixer.h"

#define k_noMusic (-1)
#define k_songs 14
#define k_fastMusicSpeed 1.3

MBoolean                musicOn = true;
int                     musicSelection = k_noMusic;

static MBoolean         s_musicFast = false;
static MBoolean         s_musicIsOgg = false;
static MBoolean         s_musicFading = false;
int                     s_musicPaused = 0;
static int              s_musicFadeDuration = 0;
static double           s_musicBaseGain = 1.0;
static MTicks           s_musicFadeStart = 0;

static struct CMVoice* s_musicChannel = NULL;

static void ApplyMusicGain(void)
{
    if (!s_musicChannel)
    {
        return;
    }

    double gain = s_musicBaseGain;

    if (s_musicFading && s_musicFadeDuration > 0)
    {
        MTicks elapsed = MTickCount() - s_musicFadeStart;
        if (elapsed >= s_musicFadeDuration)
        {
            gain = 0.0;
        }
        else if (elapsed > 0)
        {
            gain *= 1.0 - elapsed / (double) s_musicFadeDuration;
        }
    }

    CMVoice_SetGain(s_musicChannel, gain);
}

static CMVoicePtr LoadMusicTrack(short which, MBoolean* isOggOut)
{
    static const struct
    {
        const char* extension;
        CMVoicePtr (*loader)(const char* path);
        MBoolean isOgg;
    } candidates[] =
    {
        { ".ogg", CMVoice_LoadOGG, true },
        { ".OGG", CMVoice_LoadOGG, true },
        { ".mod", CMVoice_LoadMOD, false },
        { ".MOD", CMVoice_LoadMOD, false },
        { ".s3m", CMVoice_LoadMOD, false },
        { ".S3M", CMVoice_LoadMOD, false },
        { ".xm", CMVoice_LoadMOD, false },
        { ".XM", CMVoice_LoadMOD, false },
    };

    if (isOggOut)
    {
        *isOggOut = false;
    }

    for (size_t i = 0; i < SDL_arraysize(candidates); i++)
    {
        const char* path = QuickResourceName("mod", which + 128, candidates[i].extension);
        if (!FileExists(path))
        {
            continue;
        }

        CMVoicePtr voice = candidates[i].loader(path);
        if (voice)
        {
            if (isOggOut)
            {
                *isOggOut = candidates[i].isOgg;
            }
            return voice;
        }
    }

    return NULL;
}

void EnableMusic( MBoolean on )
{
    musicOn = on;
    s_musicBaseGain = on ? 1.0 : 0.0;
    ApplyMusicGain();
}

void FastMusic( void )
{
    if (s_musicChannel && !s_musicFast)
    {
        CMVoice_SetMusicPlaybackSpeed(s_musicChannel, k_fastMusicSpeed);
        s_musicFast = true;
    }
}

void SlowMusic( void )
{
    if (s_musicChannel && s_musicFast)
    {
        CMVoice_SetMusicPlaybackSpeed(s_musicChannel, 1.0);
        s_musicFast = false;
    }
}

void PauseMusic( void )
{
    if (s_musicChannel)
    {
        CMVoice_Pause(s_musicChannel);
        s_musicPaused++;
    }
}

void ResumeMusic( void )
{
    if (s_musicChannel)
    {
        CMVoice_Play(s_musicChannel);
        s_musicPaused--;
    }
}

int GetCurrentMusic( void )
{
	return musicSelection;
}

void UpdateMusic( void )
{
    if (!s_musicChannel || !s_musicFading)
    {
        return;
    }

    if (MTickCount() - s_musicFadeStart >= s_musicFadeDuration)
    {
        ShutdownMusic();
        return;
    }

    ApplyMusicGain();
}

void FadeOutMusic( int durationTicks )
{
    if (!s_musicChannel)
    {
        return;
    }

    if (!s_musicIsOgg || durationTicks <= 0)
    {
        ShutdownMusic();
        return;
    }

    if (!s_musicFading)
    {
        s_musicFading = true;
        s_musicFadeDuration = durationTicks;
        s_musicFadeStart = MTickCount();
    }

    ApplyMusicGain();
}

void ChooseMusic( short which )
{
    // Kill existing song first, if any
    ShutdownMusic();

    musicSelection = -1;

    if (which >= 0 && which <= k_songs)
    {
        s_musicChannel = LoadMusicTrack(which, &s_musicIsOgg);
        if (!s_musicChannel)
        {
            return;
        }

        s_musicFading = false;
        s_musicFadeDuration = 0;
        s_musicFadeStart = 0;

        CMVoice_SetLoop(s_musicChannel, true);
        CMVoice_SetMusicPlaybackSpeed(s_musicChannel, s_musicFast ? k_fastMusicSpeed : 1.0);
        s_musicBaseGain = musicOn ? 1.0 : 0.0;
        EnableMusic(musicOn);
        CMVoice_Play(s_musicChannel);
    
        musicSelection = which;
        s_musicPaused  = 0;
    }
}

void ShutdownMusic()
{
    if (s_musicChannel)
    {
        CMVoice_Free(s_musicChannel);
        s_musicChannel = NULL;
    }

    s_musicIsOgg = false;
    s_musicFading = false;
    s_musicFadeDuration = 0;
    s_musicFadeStart = 0;
}
