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
#define k_firstNonGameplaySong 12
#define k_maxPlaylistEntries 64

MBoolean                musicOn = true;
int                     musicSelection = k_noMusic;

static MBoolean         s_musicFast = false;
static MBoolean         s_musicIsOgg = false;
static MBoolean         s_musicFading = false;
int                     s_musicPaused = 0;
static int              s_musicFadeDuration = 0;
static double           s_musicBaseGain = 1.0;
static MTicks           s_musicFadeStart = 0;
static int              s_playlistCount = 0;
static int              s_playlistIndex = -1;

static struct CMVoice* s_musicChannel = NULL;
static char*           s_playlistEntries[k_maxPlaylistEntries];

static void ClearMusicPlaylist(void)
{
    for (int i = 0; i < s_playlistCount; i++)
    {
        SDL_free(s_playlistEntries[i]);
        s_playlistEntries[i] = NULL;
    }

    s_playlistCount = 0;
    s_playlistIndex = -1;
}

static void ReleaseMusicChannel(void)
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

static MBoolean IsTrackerExtension(const char* extension)
{
    return !SDL_strcasecmp(extension, ".mod")
        || !SDL_strcasecmp(extension, ".s3m")
        || !SDL_strcasecmp(extension, ".xm")
        || !SDL_strcasecmp(extension, ".it");
}

static CMVoicePtr LoadMusicPath(const char* path, MBoolean* isOggOut)
{
    const char* extension = SDL_strrchr(path, '.');

    if (isOggOut)
    {
        *isOggOut = false;
    }

    if (!extension)
    {
        return NULL;
    }

    if (!SDL_strcasecmp(extension, ".ogg"))
    {
        if (isOggOut)
        {
            *isOggOut = true;
        }
        return CMVoice_LoadOGG(path);
    }

    if (IsTrackerExtension(extension))
    {
        return CMVoice_LoadMOD(path);
    }

    return NULL;
}

static MBoolean StartMusicChannel(CMVoicePtr voice, MBoolean isOgg, MBoolean loop)
{
    if (!voice)
    {
        return false;
    }

    s_musicChannel = voice;
    s_musicIsOgg = isOgg;
    s_musicFading = false;
    s_musicFadeDuration = 0;
    s_musicFadeStart = 0;

    CMVoice_SetLoop(s_musicChannel, loop);
    CMVoice_SetMusicPlaybackSpeed(s_musicChannel, s_musicFast ? k_fastMusicSpeed : 1.0);
    s_musicBaseGain = musicOn ? 1.0 : 0.0;
    EnableMusic(musicOn);
    CMVoice_Play(s_musicChannel);
    s_musicPaused = 0;
    return true;
}

static MBoolean StartMusicPath(const char* path, MBoolean loop)
{
    MBoolean isOgg = false;
    CMVoicePtr voice = LoadMusicPath(path, &isOgg);
    return StartMusicChannel(voice, isOgg, loop);
}

static void TrimWhitespace(char* text)
{
    if (!text)
    {
        return;
    }

    while (*text && SDL_isspace((unsigned char) *text))
    {
        SDL_memmove(text, text + 1, SDL_strlen(text));
    }

    size_t len = SDL_strlen(text);
    while (len > 0 && SDL_isspace((unsigned char) text[len - 1]))
    {
        text[--len] = '\0';
    }
}

static MBoolean ResolvePlaylistEntryPath(const char* playlistPath, const char* entry, char* resolvedPath, size_t resolvedPathSize)
{
    char playlistDir[1024];

    if (!entry || !entry[0])
    {
        return false;
    }

    if ((SDL_isalpha((unsigned char) entry[0]) && entry[1] == ':') || entry[0] == '\\' || entry[0] == '/')
    {
        SDL_strlcpy(resolvedPath, entry, resolvedPathSize);
        return FileExists(resolvedPath);
    }

    SDL_strlcpy(playlistDir, playlistPath, sizeof(playlistDir));
    char* slash = SDL_strrchr(playlistDir, '\\');
    char* slash2 = SDL_strrchr(playlistDir, '/');
    if (slash2 && (!slash || slash2 > slash))
    {
        slash = slash2;
    }

    if (!slash)
    {
        SDL_strlcpy(resolvedPath, entry, resolvedPathSize);
        return FileExists(resolvedPath);
    }

    slash[1] = '\0';
    SDL_snprintf(resolvedPath, resolvedPathSize, "%s%s", playlistDir, entry);
    return FileExists(resolvedPath);
}

static MBoolean TryLoadPlaylistFile(const char* playlistPath)
{
    SDL_IOStream* stream = SDL_IOFromFile(playlistPath, "rb");
    if (!stream)
    {
        return false;
    }

    SDL_SeekIO(stream, 0, SDL_IO_SEEK_END);
    Sint64 fileSize = SDL_TellIO(stream);
    SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET);

    if (fileSize <= 0)
    {
        SDL_CloseIO(stream);
        return false;
    }

    char* contents = SDL_malloc((size_t) fileSize + 1);
    if (!contents)
    {
        SDL_CloseIO(stream);
        return false;
    }

    SDL_ReadIO(stream, contents, (size_t) fileSize);
    contents[fileSize] = '\0';
    SDL_CloseIO(stream);

    ClearMusicPlaylist();

    char* scan = contents;
    while (*scan && s_playlistCount < k_maxPlaylistEntries)
    {
        char* lineEnd = scan;
        char resolvedPath[1024];

        while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
        {
            lineEnd++;
        }

        char saved = *lineEnd;
        *lineEnd = '\0';
        TrimWhitespace(scan);

        if (scan[0] && scan[0] != '#' && scan[0] != ';' && ResolvePlaylistEntryPath(playlistPath, scan, resolvedPath, sizeof(resolvedPath)))
        {
            CMVoicePtr probe;
            MBoolean isOgg = false;

            probe = LoadMusicPath(resolvedPath, &isOgg);
            if (probe)
            {
                CMVoice_Free(probe);
                s_playlistEntries[s_playlistCount++] = SDL_strdup(resolvedPath);
            }
        }

        *lineEnd = saved;
        scan = lineEnd;
        while (*scan == '\n' || *scan == '\r')
        {
            scan++;
        }
    }

    SDL_free(contents);

    if (s_playlistCount == 0)
    {
        ClearMusicPlaylist();
        return false;
    }

    s_playlistIndex = 0;
    return true;
}

static MBoolean TryLoadPlaylist(short which)
{
    const char* path = QuickResourceName("mod", which + 128, ".m3u");
    if (FileExists(path) && TryLoadPlaylistFile(path))
    {
        return true;
    }

    path = QuickResourceName("mod", which + 128, ".M3U");
    if (FileExists(path) && TryLoadPlaylistFile(path))
    {
        return true;
    }

    return false;
}

static MBoolean StartPlaylistEntry(int index)
{
    if (index < 0 || index >= s_playlistCount)
    {
        return false;
    }

    return StartMusicPath(s_playlistEntries[index], false);
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
        { ".s3m", CMVoice_LoadMOD, false },
        { ".S3M", CMVoice_LoadMOD, false },
        { ".it", CMVoice_LoadMOD, false },
        { ".IT", CMVoice_LoadMOD, false },
        { ".xm", CMVoice_LoadMOD, false },
        { ".XM", CMVoice_LoadMOD, false },
        { ".mod", CMVoice_LoadMOD, false },
        { ".MOD", CMVoice_LoadMOD, false },
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
    if (s_musicChannel && s_musicFading)
    {
        if (MTickCount() - s_musicFadeStart >= s_musicFadeDuration)
        {
            ReleaseMusicChannel();
            return;
        }

        ApplyMusicGain();
    }

    if (s_playlistCount > 0 && s_musicChannel && !s_musicFading && CMVoice_GetState(s_musicChannel) == CM_STATE_STOPPED)
    {
        int nextIndex = s_playlistIndex;
        for (int tries = 0; tries < s_playlistCount; tries++)
        {
            nextIndex = (nextIndex + 1) % s_playlistCount;

            ReleaseMusicChannel();
            if (StartPlaylistEntry(nextIndex))
            {
                s_playlistIndex = nextIndex;
                return;
            }
        }

        ClearMusicPlaylist();
    }
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
        if (which >= k_firstNonGameplaySong)
        {
            s_musicFast = false;
        }

        if (TryLoadPlaylist(which))
        {
            if (StartPlaylistEntry(0))
            {
                musicSelection = which;
                return;
            }

            ClearMusicPlaylist();
        }

        s_musicChannel = LoadMusicTrack(which, &s_musicIsOgg);
        if (StartMusicChannel(s_musicChannel, s_musicIsOgg, true))
        {
            musicSelection = which;
        }
    }
}

void ShutdownMusic()
{
    ReleaseMusicChannel();
    ClearMusicPlaylist();
}
