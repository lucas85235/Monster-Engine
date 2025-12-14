#include "engine/audio/TTSManager.h"
#include "engine/Log.h"

#include <fstream>
#include <cstdlib>
#include <filesystem>

// miniaudio single-header audio library
#define MINIAUDIO_IMPLEMENTATION
#include "../../third_party/llama/vendor/miniaudio/miniaudio.h"

namespace se {

namespace fs = std::filesystem;

TTSManager::~TTSManager() {
    Shutdown();
}

bool TTSManager::Initialize() {
    if (initialized_) {
        SE_LOG_WARN("[TTSManager] Already initialized");
        return true;
    }
    
    SE_LOG_INFO("[TTSManager] Initializing Piper TTS...");
    
    // Get project directory from current file path
    // __FILE__ = .../engine/src/audio/TTSManager.cpp
    fs::path this_file(__FILE__);
    fs::path engine_dir = this_file.parent_path().parent_path().parent_path();  // Up to engine/
    
    piper_exe_path_ = (engine_dir / "piper_tts" / "piper" / "piper.exe").string();
    
    if (!fs::exists(piper_exe_path_)) {
        SE_LOG_ERROR("[TTSManager] Piper executable not found at: {}", piper_exe_path_);
        return false;
    }
    
    // Find voice model
    model_path_ = (engine_dir / "piper_tts" / "piper" / "en_US-lessac-medium.onnx").string();
    
    if (!fs::exists(model_path_)) {
        SE_LOG_ERROR("[TTSManager] Voice model not found at: {}", model_path_);
        return false;
    }
    
    // Temp file for audio
    temp_wav_path_ = (engine_dir / "piper_tts" / "temp_speech.wav").string();
    
    initialized_ = true;
    SE_LOG_INFO("[TTSManager] Piper TTS initialized successfully");
    SE_LOG_INFO("[TTSManager] Using voice: {}", model_path_);
    
    return true;
}

void TTSManager::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("[TTSManager] Shutting down...");
    
    Stop();
    
    // Wait for speak thread
    if (speak_thread_.joinable()) {
        speak_thread_.join();
    }
    
    // Cleanup temp file
    if (fs::exists(temp_wav_path_)) {
        fs::remove(temp_wav_path_);
    }
    
    initialized_ = false;
}

void TTSManager::SetVoiceModel(const std::string& model_path) {
    if (fs::exists(model_path)) {
        model_path_ = model_path;
        SE_LOG_INFO("[TTSManager] Voice model changed to: {}", model_path);
    } else {
        SE_LOG_ERROR("[TTSManager] Voice model not found: {}", model_path);
    }
}

void TTSManager::Speak(const std::string& text) {
    if (!initialized_) {
        SE_LOG_WARN("[TTSManager] Not initialized");
        return;
    }
    
    if (text.empty()) return;
    
    // Stop any current speech
    Stop();
    
    // Wait for previous thread
    if (speak_thread_.joinable()) {
        speak_thread_.join();
    }
    
    // Start new speech thread
    stop_requested_ = false;
    speak_thread_ = std::thread(&TTSManager::SpeakThread, this, text);
}

void TTSManager::SpeakThread(const std::string& text) {
    std::lock_guard<std::mutex> lock(speak_mutex_);
    
    is_speaking_ = true;
    SE_LOG_INFO("[TTSManager] Speaking: {} chars", text.size());
    
    // Generate audio with Piper
    if (GenerateAudio(text, temp_wav_path_)) {
        if (!stop_requested_) {
            // Play the generated audio
            PlayAudio(temp_wav_path_);
        }
    }
    
    is_speaking_ = false;
    is_paused_ = false;
}

bool TTSManager::GenerateAudio(const std::string& text, const std::string& output_file) {
    // Build command: echo "text" | piper --model model.onnx --output_file output.wav
    
    // Escape quotes and special characters in text
    std::string escaped_text = text;
    for (size_t i = 0; i < escaped_text.size(); ++i) {
        if (escaped_text[i] == '"') {
            escaped_text.replace(i, 1, "\\\"");
            i++;
        } else if (escaped_text[i] == '\n') {
            escaped_text.replace(i, 1, " ");
        }
    }
    
    // Use PowerShell to pipe text to piper
    std::string cmd = "powershell -Command \"echo '\"" + escaped_text + "\"' | "
                      "& '" + piper_exe_path_ + "' "
                      "--model '" + model_path_ + "' "
                      "--output_file '" + output_file + "' --quiet\"";
    
    SE_LOG_INFO("[TTSManager] Running Piper...");
    int result = std::system(cmd.c_str());
    
    if (result != 0) {
        SE_LOG_ERROR("[TTSManager] Piper failed with code: {}", result);
        return false;
    }
    
    if (!fs::exists(output_file)) {
        SE_LOG_ERROR("[TTSManager] Output file not created");
        return false;
    }
    
    SE_LOG_INFO("[TTSManager] Audio generated: {}", output_file);
    return true;
}

void TTSManager::PlayAudio(const std::string& wav_file) {
    ma_result result;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_device device;
    
    // Initialize decoder
    result = ma_decoder_init_file(wav_file.c_str(), nullptr, &decoder);
    if (result != MA_SUCCESS) {
        SE_LOG_ERROR("[TTSManager] Failed to open WAV file: {}", wav_file);
        return;
    }
    
    // Configure device for playback
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate;
    deviceConfig.pUserData = &decoder;
    
    // Data callback for playback
    deviceConfig.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
        if (pDecoder == nullptr) return;
        
        ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, nullptr);
        (void)pInput;
    };
    
    result = ma_device_init(nullptr, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        SE_LOG_ERROR("[TTSManager] Failed to initialize audio device");
        ma_decoder_uninit(&decoder);
        return;
    }
    
    // Store for pause/stop control
    audio_device_ = &device;
    audio_decoder_ = &decoder;
    
    // Start playback
    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        SE_LOG_ERROR("[TTSManager] Failed to start audio playback");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        audio_device_ = nullptr;
        audio_decoder_ = nullptr;
        return;
    }
    
    SE_LOG_INFO("[TTSManager] Playing audio...");
    
    // Wait for playback to finish or stop requested
    while (ma_device_is_started(&device) && !stop_requested_) {
        // Check if decoder reached end
        ma_uint64 cursor, length;
        if (ma_decoder_get_cursor_in_pcm_frames(&decoder, &cursor) == MA_SUCCESS &&
            ma_decoder_get_length_in_pcm_frames(&decoder, &length) == MA_SUCCESS) {
            if (cursor >= length) {
                break;
            }
        }
        
        // Handle pause
        while (is_paused_ && !stop_requested_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Cleanup
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
    audio_device_ = nullptr;
    audio_decoder_ = nullptr;
    
    SE_LOG_INFO("[TTSManager] Audio playback finished");
}

void TTSManager::Pause() {
    if (is_speaking_ && !is_paused_) {
        is_paused_ = true;
        SE_LOG_INFO("[TTSManager] Paused");
    }
}

void TTSManager::Resume() {
    if (is_speaking_ && is_paused_) {
        is_paused_ = false;
        SE_LOG_INFO("[TTSManager] Resumed");
    }
}

void TTSManager::Stop() {
    if (is_speaking_) {
        stop_requested_ = true;
        is_paused_ = false;
        SE_LOG_INFO("[TTSManager] Stop requested");
    }
}

void TTSManager::StopAudioPlayback() {
    if (audio_device_) {
        ma_device_stop((ma_device*)audio_device_);
    }
}

} // namespace se
