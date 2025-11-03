#pragma once

#include "engine/audio/Audio.h"
#include "engine/ecs/Entity.h"
#include <map>

using std::map;

struct AudioSource {
  string file;
  se::Entity target;

  float volume;
  float radius;
  bool isLooping;
};

class AudioManager {

public:
  static void AddAudio(string name);
  static Audio* GetAudio(string name);
  static void DeleteAudio();
  static void PrepareAndPlay(AudioSource a);

protected:
  AudioManager(void);
  ~AudioManager(void);

  static map<string, Audio*> audios;
};