#pragma once
#include <stdio.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "TinyMaterialDesign/Core/Page.h"
#include "TinyMaterialDesign/Core/Screen.h"
#include "TinyMaterialDesign/Draw/Icons.h"
#include "TinyMaterialDesign/Widgets/Button.h"
#include "TinyMaterialDesign/Widgets/IconButton.h"
#include "TinyMaterialDesign/Widgets/Keyboard.h"
#include "TinyMaterialDesign/Widgets/Label.h"
#include "TinyMaterialDesign/Widgets/ListItem.h"
#include "TinyMaterialDesign/Widgets/Menu.h"
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
  static constexpr int32_t kHeight = 32;
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
    target.fillRect(toPx(b.x + 1), toPx(b.y), toPx(b.w - 2), toPx(b.h), bg);
    tinygpu::IFont<RGB_T>& body = *t.typography.body;
    const RGB_T fg = current_ ? t.colors.onSecondaryContainer : t.colors.onSurface;
    std::string name = ssid_;
    const int32_t maxW = b.w - 64;
    while (name.size() > 3 && static_cast<int32_t>(body.measureTextWidth(name.c_str())) > maxW) name.pop_back();
    body.drawText(target, static_cast<int16_t>(b.x + 10),
                  static_cast<int16_t>(b.centerY() - static_cast<int32_t>(body.getHeight(1)) / 2), name.c_str(), fg,
                  bg, false);
    if (secure_) drawLock(target, Bounds(b.right() - 48, b.centerY() - 6, 9, 12), t.colors.onSurfaceVariant);
    drawSignalBars(target, Bounds(b.right() - 30, b.centerY() - 7, 18, 14), bars_, t.colors.primary, t.colors.outline);
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
 * the SSID with a button that opens the networks of a scan as pop-up list,
 * the password (masked, "Show password"), Connect (Disconnect while
 * connected) and Cancel (leaves the page without connecting).
 *
 * - When a field is tapped, the page scrolls it to the top, so that it stays
 *   visible above the keyboard.
 * - Connect scrolls back to the top: the state of the connection is visible.
 * - Selecting a network in the pop-up connects at once to an open network
 *   and opens the keyboard for the password of a secured one.
 * - Once a connection works, the page calls onDone (after showing
 *   "Connected" briefly).
 *
 * The WiFi itself is a WifiControl: Esp32Wifi (Wifi/WifiEsp32.h) on the
 * ESP32, SimulatedWifi on the desktop. Show the page on its own, or add it
 * to a PageNavigator; call tick() regularly while it is shown (the
 * PageNavigator does that) - it polls the scan and the connection and
 * returns true when the page must be redrawn.
 *
 *   Keyboard<RGB565> keyboard(Bounds(0, 150, 240, 170));
 *   WifiLoginPage<RGB565> login(keyboard, app.screen());
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
 *     login.onDone = []() { ... };  // connected or cancelled
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

  WifiLoginPage(Keyboard<RGB_T>& keyboard, Screen<RGB_T>& screen, const char* title = "WiFi")
      : Page<RGB_T>(title, drawWifi<RGB_T>), keyboard_(keyboard), screen_(screen), showPassword_("Show password") {}

  /// The user is done: connected or cancelled.
  std::function<void()> onDone;

  void setWifi(WifiControl* wifi) { wifi_ = wifi; }

  /// Text of the button that leaves the page without connecting (default
  /// "Cancel"); nullptr hides it.
  void setCancelText(const char* text) {
    cancelText_ = text != nullptr ? text : "";
    if (built_) {
      cancel_.setLabel(cancelText_.c_str());
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
    if (popupOpen_) fillPopup();
  }

  /// Opens the pop-up list of the networks (as the button next to the SSID).
  void showNetworks() {
    if (networks_.empty() && !scanning_) startScan();
    keyboard_.hide();
    ssid_.blur();
    password_.blur();
    fillPopup();
    popupOpen_ = true;
    screen_.presentDialog(popup_);
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

    ssid_ = TextField<RGB_T>(Bounds(), "Network (SSID)", "tap the list or type");
    ssid_.setMaxLength(32);
    ssid_.setOnBackground(true);
    manage(ssid_);
    this->addChild(ssid_);
    list_ = IconButton<RGB_T>(Bounds(), drawChevronDown<RGB_T>);  // opens the list of networks
    list_.onClick = [this]() { showNetworks(); };
    this->addChild(list_);
    password_ = TextField<RGB_T>(Bounds(), "Password", "");
    password_.setMaxLength(63);
    password_.setPasswordMode(true);
    password_.setOnBackground(true);
    password_.onSubmit = [this]() { connectFromFields(); };
    manage(password_);
    this->addChild(password_);
    showPassword_.onChange = [this](bool v) { password_.setPasswordMode(!v); };
    this->addChild(showPassword_);

    connect_ = Button<RGB_T>(Bounds(), "Connect");
    connect_.onClick = [this]() {
      if (connected()) {
        disconnect();
      } else {
        connectFromFields();
      }
    };
    this->addChild(connect_);
    cancel_ = Button<RGB_T>(Bounds(), cancelText_.c_str(), ButtonVariant::kOutlined);
    cancel_.onClick = [this]() {
      keyboard_.hide();
      if (onDone) onDone();
    };
    this->addChild(cancel_);
    this->addChild(spacer_);  // room to scroll each field to the top

    // the pop-up list of the networks
    for (int i = 0; i < kMaxNetworks; ++i) rows_[i].onSelect = [this, i]() { select(i); };
    scanAgain_ = ListItem<RGB_T>(Bounds(), "Scan again");
    scanAgain_.onClick = [this]() { startScan(); };
    popup_.onOutsideTap = [this]() { closePopup(); };
    built_ = true;
    layout();
  }

  void onShow() override {
    if (wifi_ != nullptr && ssid_.text().empty()) ssid_.setText(wifi_->ssid().c_str());
    // only a new connection closes the page (not one that existed already)
    lastState_ = wifi_ != nullptr ? wifi_->state() : WifiControl::State::kDisconnected;
    doneAtMs_ = 0;
    this->setScrollOffset(0);
    if (networks_.empty()) startScan();
    refreshStatus();
  }

  void onHide() override {
    keyboard_.hide();
    if (popupOpen_) closePopup();
    doneAtMs_ = 0;
  }

  /// Polls the scan and the connection: true if the page changed.
  bool tick(uint32_t nowMs) override {
    if (wifi_ == nullptr || nowMs - lastTickMs_ < 250) return false;
    lastTickMs_ = nowMs;
    bool changed = false;
    if (scanning_ && wifi_->scanDone(networks_)) {
      scanning_ = false;
      if (networks_.size() > kMaxNetworks) networks_.resize(kMaxNetworks);
      if (popupOpen_) fillPopup();
      changed = true;
    }
    const WifiControl::State state = wifi_->state();
    if (state != lastState_) {
      // a new connection: show "Connected" briefly, then close
      if (state == WifiControl::State::kConnected) doneAtMs_ = nowMs + 1500;
      lastState_ = state;
      changed = true;
    }
    if (doneAtMs_ != 0 && static_cast<int32_t>(nowMs - doneAtMs_) >= 0) {
      doneAtMs_ = 0;
      if (onDone) onDone();
    }
    if (changed) refreshStatus();
    return changed;
  }

 private:
  Keyboard<RGB_T>& keyboard_;
  Screen<RGB_T>& screen_;
  WifiControl* wifi_ = nullptr;
  bool built_ = false;
  bool scanning_ = false;
  bool popupOpen_ = false;
  WifiControl::State lastState_ = WifiControl::State::kDisconnected;
  uint32_t lastTickMs_ = 0;
  uint32_t doneAtMs_ = 0;
  std::vector<WifiNetwork> networks_;
  std::string cancelText_ = "Cancel";

  Label<RGB_T> title_, status_, spacer_;
  TextField<RGB_T> ssid_, password_;
  IconButton<RGB_T> list_;
  SwitchRow<RGB_T> showPassword_;
  Button<RGB_T> connect_, cancel_;
  Menu<RGB_T> popup_;
  WifiNetworkRow<RGB_T> rows_[kMaxNetworks];
  ListItem<RGB_T> scanAgain_;

  /// The keyboard serves the field, and the page scrolls the field to the
  /// top so that it stays visible above the keyboard.
  void manage(TextField<RGB_T>& field) {
    keyboard_.manage(field);
    std::function<void()> showKeyboard = field.onFocus;
    field.onFocus = [this, showKeyboard, &field]() {
      if (showKeyboard) showKeyboard();
      this->scrollToChild(field);
      screen_.invalidate();
    };
  }

  /// (Re)fills the pop-up with the networks and "Scan again".
  void fillPopup() {
    const Bounds& b = this->bounds;
    const int count = static_cast<int>(networks_.size());
    const int32_t rowH = WifiNetworkRow<RGB_T>::kHeight;
    const int32_t maxH = b.h - 40;
    const int32_t h = std::min<int32_t>((count + 1) * rowH, maxH - maxH % rowH);
    popup_ = Menu<RGB_T>(Bounds(b.x + 12, b.y + 20, b.w - 24, h));
    popup_.onOutsideTap = [this]() { closePopup(); };
    const std::string current =
        wifi_ != nullptr && wifi_->state() == WifiControl::State::kConnected ? wifi_->ssid() : "";
    for (int i = 0; i < count; ++i) {
      const WifiNetwork& n = networks_[i];
      rows_[i].setNetwork(n.ssid.c_str(), wifiBars(n.rssi), n.secure, n.ssid == current);
      rows_[i].bounds = popup_.itemRect(i, rowH);
      popup_.addItem(rows_[i]);
    }
    scanAgain_.setTitle(scanning_ ? "Scanning ..." : count == 0 ? "No networks - scan again" : "Scan again");
    scanAgain_.bounds = popup_.itemRect(count, rowH);
    popup_.addItem(scanAgain_);
    popup_.setTheme(this->theme());
    screen_.invalidate();
  }

  void closePopup() {
    popupOpen_ = false;
    screen_.dismissDialog();
  }

  void select(int index) {
    if (index >= static_cast<int>(networks_.size())) return;
    closePopup();
    ssid_.setText(networks_[index].ssid.c_str());
    password_.setText("");
    if (networks_[index].secure) {
      password_.focus();  // keyboard; scrolls the field to the top
    } else {
      connectFromFields();
    }
  }

  bool connected() { return wifi_ != nullptr && wifi_->state() == WifiControl::State::kConnected; }

  void disconnect() {
    keyboard_.hide();
    wifi_->disconnect();
    lastState_ = wifi_->state();  // not a new connection: the page stays
    doneAtMs_ = 0;
    refreshStatus();
  }

  void connectFromFields() {
    if (wifi_ == nullptr || ssid_.text().empty()) return;
    keyboard_.hide();
    ssid_.blur();
    password_.blur();
    this->setScrollOffset(0);  // the state of the connection is visible
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
        snprintf(buf, sizeof(buf), scanning_ ? "Not connected - scanning for networks ..."
                                             : "Not connected: select a network");
        status_.setColor(c.onSurfaceVariant);
        break;
    }
    status_.setText(buf);
    connect_.setLabel(connected() ? "Disconnect" : "Connect");
    screen_.invalidate();
  }

  void layout() {
    const Bounds& b = this->bounds;
    constexpr int32_t margin = 8;
    const int32_t x = b.x + margin;
    const int32_t w = b.w - margin - Container<RGB_T>::kScrollbarTouchWidth;
    int32_t y = b.y + 6;
    title_.bounds = Bounds(x, y, w, 18);
    y += 20;
    status_.bounds = Bounds(x, y, w, 12);
    y += 20;
    ssid_.bounds = Bounds(x, y, w - 44, 44);
    list_.bounds = Bounds(x + w - 40, y + 12, 40, 32);
    y += 50;
    password_.bounds = Bounds(x, y, w, 44);
    y += 46;
    showPassword_.bounds = Bounds(x, y, w, SwitchRow<RGB_T>::kHeight);
    y += SwitchRow<RGB_T>::kHeight + 6;
    // Connect / Disconnect and Cancel side by side
    cancel_.visible = !cancelText_.empty();
    if (cancel_.visible) {
      const int32_t half = (w - 8) / 2;
      connect_.bounds = Bounds(x, y, half, 32);
      cancel_.bounds = Bounds(x + half + 8, y, half, 32);
    } else {
      connect_.bounds = Bounds(x, y, w, 32);
      cancel_.bounds = Bounds(x, b.y, 0, 0);
    }
    y += 40;
    // enough room below to scroll the password field to the top
    spacer_.bounds = Bounds(x, y, w, b.h);
  }
};

}  // namespace tinymd
