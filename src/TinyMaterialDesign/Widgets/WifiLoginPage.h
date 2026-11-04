#pragma once
#include <stdio.h>

#include <functional>
#include <string>
#include <vector>

#include "TinyMaterialDesign/Core/Page.h"
#include "TinyMaterialDesign/Draw/Icons.h"
#include "TinyMaterialDesign/Widgets/Button.h"
#include "TinyMaterialDesign/Widgets/Keyboard.h"
#include "TinyMaterialDesign/Widgets/Label.h"
#include "TinyMaterialDesign/Widgets/SettingRows.h"
#include "TinyMaterialDesign/Widgets/TextField.h"
#include "TinyMaterialDesign/Wifi/WifiControl.h"

namespace tinymd {

/**
 * @brief One network in a list: name, a lock for a secured network and the
 * signal bars; tapping the row calls onSelect. The connected network is
 * highlighted.
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class WifiNetworkRow : public Widget<RGB_T> {
 public:
  static constexpr int32_t kHeight = 30;
  std::function<void()> onSelect;

  void setNetwork(const char* ssid, int bars, bool secure, bool current) {
    ssid_ = ssid;
    bars_ = bars;
    secure_ = secure;
    current_ = current;
  }

  void draw(tinygpu::ISurface<RGB_T>& target) override {
    const MaterialTheme<RGB_T>& t = this->theme();
    const Bounds& b = this->bounds;
    const RGB_T bg = current_ ? t.colors.secondaryContainer : t.colors.surface;
    target.fillRoundRect(toPx(b.x), toPx(b.y), toPx(b.w), toPx(b.h - 2), toPx(t.shape.small), bg);
    tinygpu::IFont<RGB_T>& body = *t.typography.body;
    const RGB_T fg = current_ ? t.colors.onSecondaryContainer : t.colors.onSurface;
    std::string name = ssid_;
    const int32_t maxW = b.w - 60;
    while (name.size() > 3 && static_cast<int32_t>(body.measureTextWidth(name.c_str())) > maxW) name.pop_back();
    body.drawText(target, static_cast<int16_t>(b.x + 8),
                  static_cast<int16_t>(b.centerY() - 1 - static_cast<int32_t>(body.getHeight(1)) / 2), name.c_str(),
                  fg, bg, false);
    if (secure_) drawLock(target, Bounds(b.right() - 44, b.centerY() - 7, 9, 12), t.colors.onSurfaceVariant);
    drawSignalBars(target, Bounds(b.right() - 26, b.centerY() - 8, 18, 14), bars_, t.colors.primary, t.colors.outline);
  }

  bool onGesture(const tinygpu::GestureEvent& event) override {
    if (!isTapGesture(event.type)) return false;
    if (onSelect) onSelect();
    return true;
  }

 private:
  std::string ssid_;
  int bars_ = 0;
  bool secure_ = false;
  bool current_ = false;
};

/**
 * @brief WiFi selection and login as a Page: the state of the connection,
 * SSID and password (masked, "Show password"), Connect / Scan, the networks
 * of the scan (tap one to select it: open networks connect at once, for
 * secured ones the keyboard opens for the password) and "Continue without
 * WiFi".
 *
 * The WiFi itself is a WifiControl: Esp32Wifi (Wifi/WifiEsp32.h) on the
 * ESP32, SimulatedWifi on the desktop. Show the page on its own, or add it
 * to a PageNavigator; call tick() regularly while it is shown (the
 * PageNavigator does that) - it polls the scan and the connection and
 * returns true when the page must be redrawn.
 *
 *   Keyboard<RGB565> keyboard(Bounds(0, 150, 240, 170));
 *   WifiLoginPage<RGB565> login(keyboard);
 *   Esp32Wifi wifi;
 *
 *   void setup() {
 *     app.begin();
 *     wifi.begin();
 *     login.setWifi(&wifi);
 *     login.bounds = Bounds(0, 40, 240, 280);
 *     app.screen().addWidget(login);
 *     login.build();
 *     login.onShow();
 *     app.screen().addFixedWidget(keyboard);
 *     login.onDone = []() { ... };  // connected (startup) or skipped
 *   }
 *   void loop() {
 *     if (login.tick(millis())) app.screen().invalidate();
 *     app.update();
 *   }
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class WifiLoginPage : public Page<RGB_T> {
 public:
  static constexpr int kMaxNetworks = 12;

  explicit WifiLoginPage(Keyboard<RGB_T>& keyboard, const char* title = "WiFi")
      : Page<RGB_T>(title, drawWifi<RGB_T>), keyboard_(keyboard), showPassword_("Show password") {}

  /// The user is done: connected (on startup, see setStartup()) or skipped.
  std::function<void()> onDone;

  void setWifi(WifiControl* wifi) { wifi_ = wifi; }

  /// On startup the page calls onDone by itself once a connection works.
  void setStartup(bool startup) { startup_ = startup; }

  /// Text of the button that leaves the page without WiFi; nullptr hides it.
  void setSkipText(const char* text) {
    skipText_ = text != nullptr ? text : "";
    if (built_) {
      skip_.setLabel(skipText_.c_str());
      layout();
    }
  }

  /// Fills in the fields and connects (as the Connect button).
  void connect(const char* ssid, const char* password) {
    ssid_.setText(ssid);
    password_.setText(password);
    connectFromFields();
  }

  /// Starts a new scan.
  void startScan() {
    if (wifi_ == nullptr || scanning_) return;
    scanning_ = wifi_->startScan();
    refreshStatus();
  }

  /// The networks of the last scan.
  const std::vector<WifiNetwork>& networks() const { return networks_; }

  void build() override {
    title_ = Label<RGB_T>(Bounds(), this->title(), TypographyRole::kTitle);
    title_.setOnBackground(true);
    this->addChild(title_);
    status_ = Label<RGB_T>(Bounds(), "", TypographyRole::kLabel);
    status_.setOnBackground(true);
    this->addChild(status_);

    ssid_ = TextField<RGB_T>(Bounds(), "Network (SSID)", "select below or type");
    ssid_.setMaxLength(32);
    ssid_.setOnBackground(true);
    keyboard_.manage(ssid_);
    this->addChild(ssid_);
    password_ = TextField<RGB_T>(Bounds(), "Password", "");
    password_.setMaxLength(63);
    password_.setPasswordMode(true);
    password_.setOnBackground(true);
    password_.onSubmit = [this]() { connectFromFields(); };
    keyboard_.manage(password_);
    this->addChild(password_);
    showPassword_.onChange = [this](bool v) { password_.setPasswordMode(!v); };
    this->addChild(showPassword_);

    connect_ = Button<RGB_T>(Bounds(), "Connect");
    connect_.onClick = [this]() { connectFromFields(); };
    this->addChild(connect_);
    scan_ = Button<RGB_T>(Bounds(), "Scan", ButtonVariant::kOutlined);
    scan_.onClick = [this]() { startScan(); };
    this->addChild(scan_);

    listCaption_ = Label<RGB_T>(Bounds(), "Networks", TypographyRole::kLabel);
    listCaption_.setOnBackground(true);
    listCaption_.setVariantColor(true);
    this->addChild(listCaption_);
    for (int i = 0; i < kMaxNetworks; ++i) {
      rows_[i].onSelect = [this, i]() { select(i); };
      this->addChild(rows_[i]);
    }
    skip_ = Button<RGB_T>(Bounds(), skipText_.c_str(), ButtonVariant::kText);
    skip_.onClick = [this]() {
      keyboard_.hide();
      if (onDone) onDone();
    };
    this->addChild(skip_);
    this->addChild(spacer_);  // room to scroll the fields above the keyboard
    built_ = true;
    layout();
  }

  void onShow() override {
    if (wifi_ != nullptr && ssid_.text().empty()) ssid_.setText(wifi_->ssid().c_str());
    if (networks_.empty()) startScan();
    refreshStatus();
  }

  void onHide() override {
    keyboard_.hide();
    startup_ = false;
  }

  /// Polls the scan and the connection: true if the page changed.
  bool tick(uint32_t nowMs) override {
    if (wifi_ == nullptr || nowMs - lastTickMs_ < 250) return false;
    lastTickMs_ = nowMs;
    bool changed = false;
    if (scanning_ && wifi_->scanDone(networks_)) {
      scanning_ = false;
      if (networks_.size() > kMaxNetworks) networks_.resize(kMaxNetworks);
      changed = true;
    }
    const WifiControl::State state = wifi_->state();
    if (state != lastState_) {
      if (state == WifiControl::State::kConnected && startup_ && lastState_ == WifiControl::State::kConnecting) {
        doneAtMs_ = nowMs + 1500;  // show "connected" briefly
      }
      lastState_ = state;
      changed = true;
    }
    if (doneAtMs_ != 0 && static_cast<int32_t>(nowMs - doneAtMs_) >= 0) {
      doneAtMs_ = 0;
      startup_ = false;
      if (onDone) onDone();
    }
    if (changed) {
      layout();  // highlights the connected network
      refreshStatus();
    }
    return changed;
  }

 private:
  Keyboard<RGB_T>& keyboard_;
  WifiControl* wifi_ = nullptr;
  bool built_ = false;
  bool startup_ = false;
  bool scanning_ = false;
  WifiControl::State lastState_ = WifiControl::State::kDisconnected;
  uint32_t lastTickMs_ = 0;
  uint32_t doneAtMs_ = 0;
  std::vector<WifiNetwork> networks_;
  std::string skipText_ = "Continue without WiFi";

  Label<RGB_T> title_, status_, listCaption_, spacer_;
  TextField<RGB_T> ssid_, password_;
  SwitchRow<RGB_T> showPassword_;
  Button<RGB_T> connect_, scan_, skip_;
  WifiNetworkRow<RGB_T> rows_[kMaxNetworks];

  void select(int index) {
    if (index >= static_cast<int>(networks_.size())) return;
    ssid_.setText(networks_[index].ssid.c_str());
    password_.setText("");
    if (networks_[index].secure) {
      password_.focus();  // shows the keyboard
    } else {
      connectFromFields();
    }
  }

  void connectFromFields() {
    if (wifi_ == nullptr || ssid_.text().empty()) return;
    keyboard_.hide();
    ssid_.blur();
    password_.blur();
    wifi_->connect(ssid_.text().c_str(), password_.text().c_str());
    lastState_ = WifiControl::State::kConnecting;
    refreshStatus();
  }

  void refreshStatus() {
    if (!built_) return;
    if (wifi_ == nullptr) {
      status_.setText("No WiFi available");
      return;
    }
    const ColorScheme<RGB_T>& c = this->theme().colors;
    char buf[96];
    switch (wifi_->state()) {
      case WifiControl::State::kConnected:
        snprintf(buf, sizeof(buf), "Connected to %s (%s)", wifi_->ssid().c_str(), wifi_->ip().c_str());
        status_.setColor(c.primary);
        break;
      case WifiControl::State::kConnecting:
        snprintf(buf, sizeof(buf), "Connecting to %s ...", wifi_->ssid().c_str());
        status_.setColor(c.onSurfaceVariant);
        break;
      case WifiControl::State::kFailed:
        snprintf(buf, sizeof(buf), "Could not connect to %s: check the password", wifi_->ssid().c_str());
        status_.setColor(c.error);
        break;
      default:
        snprintf(buf, sizeof(buf), "Not connected: select a network");
        status_.setColor(c.onSurfaceVariant);
        break;
    }
    status_.setText(buf);
    listCaption_.setText(scanning_ ? "Networks (scanning ...)" : "Networks");
  }

  /// Places a widget below the previous one; hidden ones get empty bounds.
  struct Placer {
    int32_t x, y, w, top;
    void place(Widget<RGB_T>& widget, int32_t h, bool show = true, int32_t gap = 6) {
      widget.visible = show;
      if (!show) {
        widget.bounds = Bounds(x, top, 0, 0);
        return;
      }
      widget.bounds = Bounds(x, y, w, h);
      y += h + gap;
    }
  };

  void layout() {
    const Bounds& b = this->bounds;
    constexpr int32_t margin = 8;
    const int32_t w = b.w - margin - Container<RGB_T>::kScrollbarTouchWidth;
    Placer p{b.x + margin, b.y + 6, w, b.y};
    p.place(title_, 18, true, 2);
    p.place(status_, 12, true, 8);
    p.place(ssid_, 44);
    p.place(password_, 44, true, 2);
    p.place(showPassword_, SwitchRow<RGB_T>::kHeight, true, 4);
    const int32_t half = (w - 8) / 2;
    connect_.bounds = Bounds(p.x, p.y, half, 32);
    scan_.bounds = Bounds(p.x + half + 8, p.y, half, 32);
    p.y += 32 + 12;
    p.place(listCaption_, 12, true, 4);
    const std::string current =
        wifi_ != nullptr && wifi_->state() == WifiControl::State::kConnected ? wifi_->ssid() : "";
    for (int i = 0; i < kMaxNetworks; ++i) {
      const bool used = i < static_cast<int>(networks_.size());
      if (used) {
        const WifiNetwork& n = networks_[i];
        rows_[i].setNetwork(n.ssid.c_str(), wifiBars(n.rssi), n.secure, n.ssid == current);
      }
      p.place(rows_[i], WifiNetworkRow<RGB_T>::kHeight, used, 2);
    }
    p.y += 8;
    p.place(skip_, 32, !skipText_.empty());
    p.place(spacer_, 120);
  }
};

}  // namespace tinymd
