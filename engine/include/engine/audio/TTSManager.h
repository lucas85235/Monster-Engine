#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace se {

class TTSManager {
public:
    static TTSManager& Get() {
        static TTSManager instance;
        return instance;
    }
    
    bool Initialize();
    void Shutdown();
    
    // Speech control
    void Speak(const std::string& text);
    void Pause();
    void Resume();
    void Stop();
    
    // State queries
    bool IsSpeaking() const { return is_speaking_.load(); }
    bool IsPaused() const { return is_paused_.load(); }
    bool IsInitialized() const { return initialized_; }
    
    // Configuration
    void SetVoiceModel(const std::string& model_path);
    void SetSpeechRate(float rate) { speech_rate_ = rate; }
    float GetSpeechRate() const { return speech_rate_; }

private:
    TTSManager() = default;
    ~TTSManager();
    
    TTSManager(const TTSManager&) = delete;
    TTSManager& operator=(const TTSManager&) = delete;
    
    void SpeakThread(const std::string& text);
    bool GenerateAudio(const std::string& text, const std::string& output_file);
    void PlayAudio(const std::string& wav_file);
    void StopAudioPlayback();
    
    bool initialized_ = false;
    std::string piper_exe_path_;
    std::string model_path_;
    std::string temp_wav_path_;
    float speech_rate_ = 1.0f;
    
    std::atomic<bool> is_speaking_{false};
    std::atomic<bool> is_paused_{false};
    std::atomic<bool> stop_requested_{false};
    
    std::thread speak_thread_;
    std::mutex speak_mutex_;
    
    // Audio playback handle
    void* audio_device_ = nullptr;
    void* audio_decoder_ = nullptr;
};

} // namespace se
