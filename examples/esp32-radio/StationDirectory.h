/**
 * @file StationDirectory.h
 * @brief Fetches and holds the radio-browser.info station list for a
 *        genre/country query, on a background task.
 *
 * Owns the HTTP client used to talk to the radio-browser API and the CSV
 * parsing needed to turn a response into a small in-memory list of
 * name/stream-URL pairs. The actual network fetch runs on its own
 * audio_tools::Task so the UI thread never blocks on it; the caller kicks
 * off a fetch with requestFetch() and polls consumeReady() from the UI loop
 * to know when to redraw. Purely data - no UI/screen code.
 */
#pragma once

#include "AudioTools/Communication/AudioHttp.h"
#include "AudioTools/Concurrency.h"
#include <WiFi.h>
#include <atomic>

template <int Capacity>
class StationDirectory {
 public:
  StationDirectory() : task_("station-fetch-task", 8192, 1, 0) {}

  void begin() {
    instance_ = this;
    task_.begin(&StationDirectory::taskTrampoline);
  }

  static String queryUrl(const char* field, const char* value) {
    String url = "http://de1.api.radio-browser.info/csv/stations/search?";
    url += field;
    url += "=";
    url += value;
    url += "&codec=MP3&hidebroken=true&order=clickcount&reverse=true&limit=";
    url += String(Capacity);
    return url;
  }

  // Queues `url` to be fetched by the background task and returns
  // immediately. A later fetch request always supersedes one still pending.
  void requestFetch(const String& url) {
    audio_tools::LockGuard guard(mutex_);
    pendingUrl_ = url;
    hasPendingRequest_ = true;
    loading_ = true;
  }

  bool isLoading() const { return loading_; }

  // True exactly once right after a fetch completes, so the UI thread knows
  // to pull the new results and redraw. Clears on read.
  bool consumeReady() { return ready_.exchange(false); }

  int count() {
    audio_tools::LockGuard guard(mutex_);
    return count_;
  }
  String name(int index) {
    audio_tools::LockGuard guard(mutex_);
    return entries_[index].name;
  }
  String url(int index) {
    audio_tools::LockGuard guard(mutex_);
    return entries_[index].url;
  }

 private:
#ifdef ESP32
  using AppMutex = audio_tools::Mutex;
#else
  using AppMutex = audio_tools::StdMutex;
#endif

  struct StationEntry {
    String name;
    String url;
  };

  static void taskTrampoline() {
    if (instance_ != nullptr) instance_->loop();
  }

  void loop() {
    String url;
    bool hasRequest = false;
    {
      audio_tools::LockGuard guard(mutex_);
      if (hasPendingRequest_) {
        url = pendingUrl_;
        hasPendingRequest_ = false;
        hasRequest = true;
      }
    }
    if (!hasRequest) {
      delay(50);
      return;
    }

    // Fetched into a local buffer first so partially-parsed results are
    // never visible to the UI thread while a fetch is in flight.
    StationEntry results[Capacity];
    const int found = fetch(url, results);

    {
      audio_tools::LockGuard guard(mutex_);
      for (int i = 0; i < found; ++i) entries_[i] = results[i];
      count_ = found;
    }
    loading_ = false;
    ready_ = true;
  }

  int fetch(const String& url, StationEntry* out) {
    http_.setTimeout(8000);
    http_.addRequestHeader("User-Agent", "TinyMaterialDesign-ESP32Radio/1.0");
    http_.setConnectionClose(true);
    if (!http_.begin(url.c_str())) return 0;

    constexpr int kNameField = 3;
    constexpr int kUrlField = 4;
    constexpr int kUrlResolvedField = 5;
    constexpr int kCsvFieldsNeeded = kUrlResolvedField + 1;

    const int expectedLength = http_.contentLength();
    http_.readStringUntil('\n');  // header row

    int found = 0;
    while (expectedLength < 0 || (int)http_.totalRead() < expectedLength) {
      if (!http_.available()) {
        if (!http_.httpRequest().connected()) break;
        delay(1);  // wait for more data without starving the watchdog
        continue;
      }
      String line = http_.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;
      if (found >= Capacity) continue;  // keep draining to EOF

      String fields[kCsvFieldsNeeded];
      const int fieldCount = parseCsvLine(line, fields, kCsvFieldsNeeded);
      if (fieldCount <= kNameField) continue;
      const String& name = fields[kNameField];
      String streamUrl = (fieldCount > kUrlResolvedField) ? fields[kUrlResolvedField] : "";
      if (streamUrl.length() == 0 && fieldCount > kUrlField) streamUrl = fields[kUrlField];
      if (name.length() == 0 || streamUrl.length() == 0) continue;

      out[found].name = name;
      out[found].url = streamUrl;
      ++found;
    }
    return found;
  }

  static int parseCsvLine(const String& line, String* fields, int maxFields) {
    int count = 0;
    int i = 0;
    const int len = line.length();
    while (count < maxFields && i <= len) {
      String field;
      if (i < len && line[i] == '"') {
        ++i;
        while (i < len) {
          if (line[i] == '"') {
            if (i + 1 < len && line[i + 1] == '"') {
              field += '"';
              i += 2;
            } else {
              ++i;
              break;
            }
          } else {
            field += line[i];
            ++i;
          }
        }
        while (i < len && line[i] != ',') ++i;
      } else {
        while (i < len && line[i] != ',') {
          field += line[i];
          ++i;
        }
      }
      fields[count++] = field;
      if (i < len && line[i] == ',') {
        ++i;
      } else {
        break;
      }
    }
    return count;
  }

  static StationDirectory* instance_;

  audio_tools::Task task_;
  AppMutex mutex_;
  String pendingUrl_;
  bool hasPendingRequest_ = false;
  std::atomic<bool> loading_{false};
  std::atomic<bool> ready_{false};

  WiFiClient client_;
  URLStream http_{client_};
  StationEntry entries_[Capacity];
  int count_ = 0;
};

template <int Capacity>
StationDirectory<Capacity>* StationDirectory<Capacity>::instance_ = nullptr;
