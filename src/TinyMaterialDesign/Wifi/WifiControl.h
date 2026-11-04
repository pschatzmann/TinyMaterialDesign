#pragma once
#include <stdint.h>

#include <algorithm>
#include <string>
#include <vector>

namespace tinymd {

/// A WiFi network found by a scan.
struct WifiNetwork {
  std::string ssid;
  /// Signal strength in dBm (e.g. -45 strong, -85 weak).
  int rssi = -100;
  /// Needs a password.
  bool secure = true;
};

/// Signal strength as 0-4 bars (see drawSignalBars()).
inline int wifiBars(int rssi) {
  return rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : rssi >= -85 ? 1 : 0;
}

/// Keeps the strongest entry of each SSID (hidden networks are dropped),
/// sorted by signal strength.
inline void normalizeNetworks(std::vector<WifiNetwork>& list) {
  std::vector<WifiNetwork> out;
  for (const WifiNetwork& n : list) {
    if (n.ssid.empty()) continue;
    auto it = std::find_if(out.begin(), out.end(), [&](const WifiNetwork& o) { return o.ssid == n.ssid; });
    if (it == out.end()) {
      out.push_back(n);
    } else if (n.rssi > it->rssi) {
      *it = n;
    }
  }
  std::sort(out.begin(), out.end(), [](const WifiNetwork& a, const WifiNetwork& b) { return a.rssi > b.rssi; });
  list = out;
}

/**
 * @brief The WiFi of a device as WifiLoginPage uses it: scan, connect and
 * the stored credentials. All calls must return quickly - the scan and the
 * connection run in the background and are polled.
 *
 * Implementations: Esp32Wifi (TinyMaterialDesign/Wifi/WifiEsp32.h, not
 * included by TinyMaterialDesign.h) and SimulatedWifi (desktop, tests); for
 * another board derive from this class.
 */
class WifiControl {
 public:
  enum class State { kDisconnected, kConnecting, kConnected, kFailed };

  virtual ~WifiControl() = default;

  /// Starts a scan in the background.
  virtual bool startScan() = 0;
  /// True when the scan has finished; `out` then holds the networks
  /// (strongest first, each SSID once - see normalizeNetworks()).
  virtual bool scanDone(std::vector<WifiNetwork>& out) = 0;
  /// Connects in the background; the credentials are stored when it works.
  virtual void connect(const char* ssid, const char* password) = 0;
  virtual void disconnect() = 0;
  virtual State state() = 0;
  /// The network that is connected or that is being connected to.
  virtual std::string ssid() = 0;
  /// The IP address while connected.
  virtual std::string ip() { return ""; }
  /// Credentials are stored (they are used at the start).
  virtual bool hasCredentials() = 0;
  /// Removes the stored credentials.
  virtual void forget() {}
};

/**
 * @brief WiFi without hardware, for the desktop and tests: a fixed list of
 * networks; connecting succeeds after `connectMs` if the network is open or
 * the password is `password` ("demo").
 */
class SimulatedWifi : public WifiControl {
 public:
  SimulatedWifi() {
    networks = {{"HomeNetwork", -48, true}, {"Office", -63, true}, {"CoffeeShop", -71, false},
                {"Neighbour", -84, true}, {"Office", -70, true}};
  }

  std::vector<WifiNetwork> networks;
  /// The password that works for the secure networks.
  std::string password = "demo";
  uint32_t scanMs = 800;
  uint32_t connectMs = 1500;

  /// Starts connected (with stored credentials).
  void setConnected(const char* ssid) {
    ssid_ = ssid;
    saved_ = true;
    state_ = State::kConnected;
  }

  bool startScan() override {
    scanning_ = true;
    scanStart_ = millis();
    return true;
  }

  bool scanDone(std::vector<WifiNetwork>& out) override {
    if (!scanning_) return true;
    if (millis() - scanStart_ < scanMs) return false;
    scanning_ = false;
    out = networks;
    normalizeNetworks(out);
    return true;
  }

  void connect(const char* ssid, const char* pw) override {
    ssid_ = ssid != nullptr ? ssid : "";
    pending_ = pw != nullptr ? pw : "";
    state_ = State::kConnecting;
    connectStart_ = millis();
  }

  void disconnect() override { state_ = State::kDisconnected; }

  State state() override {
    if (state_ == State::kConnecting && millis() - connectStart_ >= connectMs) {
      auto it = std::find_if(networks.begin(), networks.end(), [&](const WifiNetwork& n) { return n.ssid == ssid_; });
      const bool ok = it != networks.end() && (!it->secure || pending_ == password);
      state_ = ok ? State::kConnected : State::kFailed;
      if (ok) saved_ = true;
    }
    return state_;
  }

  std::string ssid() override { return ssid_; }
  std::string ip() override { return state_ == State::kConnected ? "192.168.1.42" : ""; }
  bool hasCredentials() override { return saved_; }
  void forget() override { saved_ = false; }

 private:
  State state_ = State::kDisconnected;
  std::string ssid_, pending_;
  bool saved_ = false;
  bool scanning_ = false;
  uint32_t scanStart_ = 0, connectStart_ = 0;
};

}  // namespace tinymd
