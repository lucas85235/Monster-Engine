#include "engine/audio/TTSManager.h"
#include "engine/Log.h"

#ifdef _WIN32
#include <windows.h>
#include <sapi.h>
#include <sphelper.h>
#pragma comment(lib, "sapi.lib")
#endif

namespace se {

TTSManager::~TTSManager() {
    Shutdown();
}

bool TTSManager::Initialize() {
    if (initialized_) {
        SE_LOG_WARN("[TTSManager] Already initialized");
        return true;
    }
    
#ifdef _WIN32
    SE_LOG_INFO("[TTSManager] Initializing Windows SAPI...");
    
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        SE_LOG_ERROR("[TTSManager] Failed to initialize COM: 0x{:08X}", static_cast<unsigned int>(hr));
        return false;
    }
    
    // Create SAPI voice
    hr = CoCreateInstance(
        CLSID_SpVoice,
        nullptr,
        CLSCTX_ALL,
        IID_ISpVoice,
        reinterpret_cast<void**>(&voice_)
    );
    
    if (FAILED(hr) || !voice_) {
        SE_LOG_ERROR("[TTSManager] Failed to create SAPI voice: 0x{:08X}", static_cast<unsigned int>(hr));
        CoUninitialize();
        return false;
    }
    
    initialized_ = true;
    SE_LOG_INFO("[TTSManager] Windows SAPI initialized successfully");
    return true;
#else
    SE_LOG_WARN("[TTSManager] TTS only supported on Windows");
    return false;
#endif
}

void TTSManager::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("[TTSManager] Shutting down...");
    
#ifdef _WIN32
    if (voice_) {
        voice_->Release();
        voice_ = nullptr;
    }
    CoUninitialize();
#endif
    
    initialized_ = false;
}

void TTSManager::Speak(const std::string& text) {
    if (!initialized_ || !voice_) {
        SE_LOG_WARN("[TTSManager] Not initialized, cannot speak");
        return;
    }
    
#ifdef _WIN32
    // Convert UTF-8 to wide string
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring wtext(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wtext[0], size_needed);
    
    // Stop any current speech and speak new text asynchronously
    HRESULT hr = voice_->Speak(wtext.c_str(), SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
    
    if (FAILED(hr)) {
        SE_LOG_ERROR("[TTSManager] Failed to speak: 0x{:08X}", static_cast<unsigned int>(hr));
        return;
    }
    
    is_paused_ = false;
    SE_LOG_INFO("[TTSManager] Speaking: {} chars", text.size());
#endif
}

void TTSManager::Pause() {
    if (!initialized_ || !voice_) return;
    
#ifdef _WIN32
    HRESULT hr = voice_->Pause();
    if (SUCCEEDED(hr)) {
        is_paused_ = true;
        SE_LOG_INFO("[TTSManager] Paused");
    }
#endif
}

void TTSManager::Resume() {
    if (!initialized_ || !voice_) return;
    
#ifdef _WIN32
    HRESULT hr = voice_->Resume();
    if (SUCCEEDED(hr)) {
        is_paused_ = false;
        SE_LOG_INFO("[TTSManager] Resumed");
    }
#endif
}

void TTSManager::Stop() {
    if (!initialized_ || !voice_) return;
    
#ifdef _WIN32
    // Speak empty string with purge to stop
    voice_->Speak(L"", SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
    is_paused_ = false;
    SE_LOG_INFO("[TTSManager] Stopped");
#endif
}

bool TTSManager::IsSpeaking() const {
    if (!initialized_ || !voice_) return false;
    
#ifdef _WIN32
    SPVOICESTATUS status;
    HRESULT hr = voice_->GetStatus(&status, nullptr);
    if (SUCCEEDED(hr)) {
        return status.dwRunningState == SPRS_IS_SPEAKING;
    }
#endif
    return false;
}

} // namespace se
