#pragma once
/**
 * @file WifiEsp32.h
 * @brief The WiFi of the ESP32 for WifiLoginPage. Not part of
 * TinyMaterialDesign.h: include it in ESP32 sketches.
 */
#include <Preferences.h>
#include <WiFi.h>

#include "TinyMaterialDesign/Wifi/WifiControl.h"

namespace tinymd {

/**
 * @brief WifiControl for the ESP32: asynchronous scan and connection, the
 * credentials are stored in the flash (Preferences, namespace "wifi") when a
 * connection works, and used by begin().
 *
 *   Esp32Wifi wifi;
 *   void setup() {
 *     wifi.begin();             // connects with the stored credentials
 *     loginPage.setWifi(&wifi);
 *   }
 */
class Esp32Wifi : public WifiControl {
 public:
  /// Time after which a connection attempt counts as failed.
  uint32_t connectTimeoutMs = 20000;

  /// Station mode; connects with the stored credentials (if any).
  void begin() {
    WiFi.persistent(false);  // the credentials are stored here, not by the SDK
    WiFi.mode(WIFI_STA);
    Preferences p;
    p.begin("wifi", true);
    savedSsid_ = p.getString("ssid", "").c_str();
    savedPassword_ = p.getString("password", "").c_str();
    p.end();
    if (!savedSsid_.empty()) start(savedSsid_.c_str(), savedPassword_.c_str(), false);
  }

  bool startScan() override {
    // a connection attempt blocks the scan: stop it
    if (connecting_ && WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      connecting_ = false;
    }
    return WiFi.scanNetworks(true /* async */, false /* hidden */) != WIFI_SCAN_FAILED;
  }

  bool scanDone(std::vector<WifiNetwork>& out) override {
    const int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return false;
    out.clear();
    for (int i = 0; i < n; ++i) {
      WifiNetwork net;
      net.ssid = WiFi.SSID(i).c_str();
      net.rssi = WiFi.RSSI(i);
      net.secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
      out.push_back(net);
    }
    if (n >= 0) WiFi.scanDelete();
    normalizeNetworks(out);
    return true;
  }

  void connect(const char* ssid, const char* password) override { start(ssid, password, true); }

  void disconnect() override {
    WiFi.disconnect();
    connecting_ = false;
    failed_ = false;
  }

  State state() override {
    const wl_status_t s = WiFi.status();
    if (s == WL_CONNECTED) {
      if (connecting_ && store_) save();
      connecting_ = false;
      failed_ = false;
      return State::kConnected;
    }
    if (connecting_) {
      const bool timeout = millis() - startMs_ > connectTimeoutMs;
      if (s == WL_CONNECT_FAILED || s == WL_NO_SSID_AVAIL || timeout) {
        connecting_ = false;
        failed_ = true;
        WiFi.disconnect();
      }
    }
    return connecting_ ? State::kConnecting : failed_ ? State::kFailed : State::kDisconnected;
  }

  std::string ssid() override {
    if (WiFi.status() == WL_CONNECTED) return WiFi.SSID().c_str();
    return ssid_;
  }

  std::string ip() override { return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : ""; }

  bool hasCredentials() override { return !savedSsid_.empty(); }

  void forget() override {
    Preferences p;
    p.begin("wifi", false);
    p.clear();
    p.end();
    savedSsid_.clear();
    savedPassword_.clear();
  }

 private:
  std::string ssid_, password_, savedSsid_, savedPassword_;
  bool connecting_ = false;
  bool failed_ = false;
  bool store_ = false;
  uint32_t startMs_ = 0;

  void start(const char* ssid, const char* password, bool store) {
    ssid_ = ssid != nullptr ? ssid : "";
    password_ = password != nullptr ? password : "";
    store_ = store;
    WiFi.disconnect();
    WiFi.begin(ssid_.c_str(), password_.empty() ? nullptr : password_.c_str());
    connecting_ = true;
    failed_ = false;
    startMs_ = millis();
  }

  void save() {
    store_ = false;
    if (ssid_ == savedSsid_ && password_ == savedPassword_) return;
    Preferences p;
    p.begin("wifi", false);
    p.putString("ssid", ssid_.c_str());
    p.putString("password", password_.c_str());
    p.end();
    savedSsid_ = ssid_;
    savedPassword_ = password_;
  }
};

}  // namespace tinymd
