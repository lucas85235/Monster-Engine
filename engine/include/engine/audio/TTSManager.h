#pragma once

#include <string>
#include <atomic>

// Forward declarations for SAPI
struct ISpVoice;

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
    bool IsSpeaking() const;
    bool IsPaused() const { return is_paused_; }
    bool IsInitialized() const { return initialized_; }

private:
    TTSManager() = default;
    ~TTSManager();
    
    TTSManager(const TTSManager&) = delete;
    TTSManager& operator=(const TTSManager&) = delete;
    
    ISpVoice* voice_ = nullptr;
    bool initialized_ = false;
    std::atomic<bool> is_paused_{false};
};

} // namespace se
