#include "engine/audio/AudioManager.h"

#include "alc.h"
#include "engine/Log.h"
#include "engine/audio/AudioSystem.h"
#include "engine/audio/AudioEmitter.h"

map<string, Audio*> AudioManager::audios;

void AudioManager::AddAudio(string name)
{
    if (!GetAudio(name))
    {
        Audio* s = new Audio();

        string extension = name.substr(name.length() - 3, 3);

        if (extension == "wav")
        {
            s->LoadFromWAV(name);
            alGenBuffers(1, &s->buffer);

            ALenum err = alGetError();
            if (err != AL_NO_ERROR)
            {
                SE_LOG_ERROR("alGenBuffers error: {}", err);
                delete s;
                return;
            }

            // Obter informações do arquivo WAV carregado
            ALenum fmt = s->GetOALFormat();
            const void* dataptr = s->GetData();
            int size = s->GetSize();
            int freq = (int)s->GetFrequency();
            int channels = s->GetChannels();
            int bitrate = s->GetBitRate();

            SE_LOG_INFO("Loading WAV file: {}", name);
            SE_LOG_INFO("  Format: {}", fmt);
            SE_LOG_INFO("  Size: {} bytes", size);
            SE_LOG_INFO("  Frequency: {} Hz", freq);
            SE_LOG_INFO("  Channels: {}", channels);
            SE_LOG_INFO("  Bit Rate: {} bits", bitrate);

            // Validações básicas
            if (!dataptr)
            {
                SE_LOG_ERROR("WAV data is NULL for file: {}", name);
                alDeleteBuffers(1, &s->buffer);
                delete s;
                return;
            }

            if (size <= 0)
            {
                SE_LOG_ERROR("Invalid WAV size: {} for file: {}", size, name);
                alDeleteBuffers(1, &s->buffer);
                delete s;
                return;
            }

            if (freq <= 0)
            {
                SE_LOG_ERROR("Invalid sample rate: {} for file: {}", freq, name);
                alDeleteBuffers(1, &s->buffer);
                delete s;
                return;
            }

            if (!(fmt == AL_FORMAT_MONO8 || fmt == AL_FORMAT_MONO16 ||
                  fmt == AL_FORMAT_STEREO8 || fmt == AL_FORMAT_STEREO16))
            {
                SE_LOG_ERROR("Invalid AL format: {} for file: {}", fmt, name);
                alDeleteBuffers(1, &s->buffer);
                delete s;
                return;
            }

            // Chamar alBufferData UMA ÚNICA VEZ com os valores corretos do arquivo WAV
            alBufferData(s->buffer, fmt, dataptr, (ALsizei)size, (ALsizei)freq);

            err = alGetError();
            if (err != AL_NO_ERROR)
            {
                SE_LOG_ERROR("alBufferData error: {} (file={})", err, name);
                alDeleteBuffers(1, &s->buffer);
                delete s;
                return;
            }
            else
            {
                SE_LOG_INFO("Audio loaded successfully: {}", name);
            }
        }
        else
        {
            SE_LOG_WARN("OpenAL: Invalid extension for file: {}", name);
            delete s;
            return;
        }

        audios.insert(make_pair(name, s));
    }
}

Audio* AudioManager::GetAudio(string name)
{
    map<string, Audio*>::iterator s = audios.find(name);
    return (s != audios.end() ? s->second : NULL);
}

void AudioManager::DeleteAudio()
{
    for (map<string, Audio*>::iterator i = audios.begin(); i != audios.end(); ++i)
    {
        if (i->second)
        {
            alDeleteBuffers(1, &i->second->buffer);
            delete i->second;
        }
    }
    audios.clear();
}

void AudioManager::PrepareAndPlay(AudioSource a) {
    AddAudio(a.file);
    Audio* audio = GetAudio(a.file);

    AudioEmitter* emitter = new AudioEmitter(audio);
    // emitter->SetTarget(entity);
    emitter->SetVolume(a.volume);
    emitter->SetRadius(a.radius);
    emitter->SetLooping(a.isLooping);
    emitter->SetPriority(AUDIOPRIORITY_MEDIUM);
    emitter->SetPosition(glm::vec3{0.0f, 0.0f, 0.0f});

    AudioSystem::GetAudioSystem()->AddAudioEmitter(emitter);
}