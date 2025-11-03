#pragma once

#include "engine/ecs/Components.h"
#include "engine/ecs/Entity.h"
#include "Audio.h"
#include "AudioSystem.h"
#include <glm.hpp>

enum AudioPriority {
  AUDIOPRIORTY_LOW,
  AUDIOPRIORITY_MEDIUM,
  AUDIOPRIORITY_HIGH,
  AUDIOPRIORITY_ALWAYS
};

struct OALSource;


class AudioEmitter {

public:
  AudioEmitter(void);
  AudioEmitter(Audio* s);
  ~AudioEmitter(void);
  void Reset();
  void SetAudio(Audio* s);
  inline Audio* GetAudio() { return audio; }
  inline void SetPriority(AudioPriority p) { priority = p; }
  inline AudioPriority GetPriority() { return priority; }
  inline void SetVolume(float vol) {
    volume = min(1.0f, max(0.0f, vol));
  }
  inline float GetVolume() { return volume; }
  inline void SetLooping(bool state) { isLooping = state; }
  inline bool GetLooping() { return isLooping; }
  inline void SetRadius(float value) {
    radius = max(0.0f, value);
  }
  inline float GetRadius() { return radius; }
  inline float GetTimeLeft() { return timeLeft; }
  inline OALSource* GetSource() { return currentSource; }
  void SetTarget(const se::Entity& e) { target = e; }
  void SetPosition(glm::vec3 p) { position = p;}
  void AttachSource(OALSource* s);
  void DetachSource();
  static bool CompareNodesByPriority(AudioEmitter* a, AudioEmitter* b);
  virtual void Update(float msec);

  se::Entity target;
  glm::vec3 position;

protected:
  Audio* audio;
  OALSource* currentSource;
  AudioPriority priority;
  float volume;
  float radius;
  bool isLooping;
  float timeLeft;
};