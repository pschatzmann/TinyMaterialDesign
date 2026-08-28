/**
 * @file RadioOutput.h
 * @brief Streams an internet radio station to the DAC in a background task.
 *
 * Owns the whole audio pipeline (network stream -> MP3 decode -> AGC -> I2S/
 * PortAudio) plus the stall-detection/backoff logic, and runs it all on its
 * own audio_tools::Task so the UI thread never blocks on network I/O.
 */
#pragma once

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Communication/AudioHttp.h"
#include "AudioTools/Concurrency.h"
#include "AudioTools/CoreAudio/Analysis/AutomaticGainControlStream.h"
#ifdef ESP32
#include "AudioTools/AudioLibs/I2SCodecStream.h"
#include "AudioBoards/ESP32S3HosyondDisplay.h"
#else
#include "AudioTools/AudioLibs/PortAudioStream.h"
#endif
#include <atomic>

class RadioOutput {
 public:
  RadioOutput(const char* wifiSsid, const char* wifiPassword)
      : wifiSsid_(wifiSsid),
        wifiPassword_(wifiPassword),
        audioTask_("audio-task", 8192, 2, 0) {}

  void begin() {
    instance_ = this;
    audioTask_.begin(&RadioOutput::taskTrampoline);
  }

  // Records the pick - the background task notices the desired URL changed
  // and does the actual connect/stop, so this returns immediately instead of
  // blocking the caller (typically the UI thread) on a network call.
  void setStation(const String& name, const String& url) {
    audio_tools::LockGuard guard(mutex_);
    desiredName_ = name;
    desiredUrl_ = url;
  }

  String currentStationName() {
    audio_tools::LockGuard guard(mutex_);
    return desiredName_;
  }

  String currentStationUrl() {
    audio_tools::LockGuard guard(mutex_);
    return desiredUrl_;
  }

  bool isPlaying() const { return playing_; }

 private:
#ifdef ESP32
  using AppMutex = audio_tools::Mutex;
#else
  using AppMutex = audio_tools::StdMutex;
#endif

  static void taskTrampoline() {
    if (instance_ != nullptr) instance_->loop();
  }

  void loop() {
    if (!wifiConnected_) {
#ifdef ESP32
      WiFi.mode(WIFI_STA);
#endif
      WiFi.begin(wifiSsid_, wifiPassword_);
      while (WiFi.status() != WL_CONNECTED) {
        delay(300);
      }
      copier_.resize(8192);
      wifiConnected_ = true;
      return;
    }

    String desired;
    {
      audio_tools::LockGuard guard(mutex_);
      desired = desiredUrl_;
    }
    if (desired != activeStationUrl_) {
      // A newly-picked station always cuts in immediately, even if the
      // previous one is mid-backoff below - only a stall retry of the same
      // URL waits out retryAtMs_.
      activeStationUrl_ = desired;
      stallRetryDelayMs_ = 0;
      startStreaming(activeStationUrl_);
      return;
    }

    if (playing_) {
      if (copier_.copy() > 0) {
        lastProgressMs_ = millis();
        if (millis() - streamStartedMs_ > kStableAfterMs) {
          stallRetryDelayMs_ =
              0;  // been solid for a while - forget past trouble
        }
      } else if (millis() - lastProgressMs_ > kStallTimeoutMs) {
        stopStreaming();
        stallRetryDelayMs_ =
            stallRetryDelayMs_ == 0
                ? 2000
                : min(stallRetryDelayMs_ * 2, kMaxStallRetryDelayMs);
        retryAtMs_ = millis() + stallRetryDelayMs_;
        Serial.print("Stream stalled - retrying in ");
        Serial.print(stallRetryDelayMs_);
        Serial.println("ms");
      }
    } else if (activeStationUrl_.length() > 0 && millis() >= retryAtMs_) {
      startStreaming(activeStationUrl_);
    } else {
      delay(5);
    }
  }

  void startStreaming(const String& url) {
    if (url.length() == 0) return;
    if (playing_) stopStreaming();

    decoder_.begin();
    auto dacConfig = dacOut_.defaultConfig(TX_MODE);
    dacOut_.begin(dacConfig);
#ifdef ESP32
    dacOut_.setVolume(0.5f);  // fixed - no on-screen volume control; ESP32
                              // only - PortAudioStream has no
                              // VolumeSupport, use the host's own volume
                              // control instead.
#endif

    Serial.print("Connecting to: ");
    Serial.println(url.c_str());
    if (!urlStream_.begin(url.c_str(), "audio/mp3")) {
      // Same backoff as a stall (see loop()) - an immediate connect failure
      // would otherwise retry just as fast next iteration.
      stallRetryDelayMs_ = stallRetryDelayMs_ == 0 ? 2000
                                                   : min(stallRetryDelayMs_ * 2,
                                                         kMaxStallRetryDelayMs);
      retryAtMs_ = millis() + stallRetryDelayMs_;
      Serial.print("  connect failed - retrying in ");
      Serial.print(stallRetryDelayMs_);
      Serial.println("ms");
      return;
    }
    playing_ = true;
    lastProgressMs_ = millis();
    streamStartedMs_ = lastProgressMs_;
  }

  void stopStreaming() {
    if (!playing_) return;
    urlStream_.end();
    decoder_.end();
    playing_ = false;
  }

  static RadioOutput* instance_;

  const char* wifiSsid_;
  const char* wifiPassword_;

  AppMutex mutex_;
  String desiredName_;
  String desiredUrl_;
  String activeStationUrl_;

  ICYStream urlStream_;
#ifdef ESP32
  I2SCodecStream dacOut_{ESP32S3HosyondDisplay};
#else
  PortAudioStream dacOut_;
#endif
  AutomaticGainControlStream agcStream_{dacOut_};
  MP3DecoderHelix mp3Decoder_;
  EncodedAudioStream decoder_{&agcStream_, &mp3Decoder_};
  StreamCopy copier_{decoder_, urlStream_};
  std::atomic<bool> playing_{false};

  bool wifiConnected_ = false;
  uint32_t lastProgressMs_ = 0;
  uint32_t streamStartedMs_ = 0;
  uint32_t stallRetryDelayMs_ = 0;
  uint32_t retryAtMs_ = 0;
  static constexpr uint32_t kStallTimeoutMs = 15000;
  static constexpr uint32_t kMaxStallRetryDelayMs = 32000;
  static constexpr uint32_t kStableAfterMs = 20000;

  audio_tools::Task audioTask_;
};

RadioOutput* RadioOutput::instance_ = nullptr;
