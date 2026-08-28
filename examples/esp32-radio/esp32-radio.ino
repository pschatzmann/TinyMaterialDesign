/**
 * @file esp32-radio.ino
 * @brief DRAFT Internet radio player for ESP32 with a 2.8" LCD display (or
 * desktop debug build).
 */
#define URL_CLIENT_TIMEOUT 8000

#include <TinyGPU/Boards/LCDBoards.h>
#include <TinyGPU/IO/JPEGParser.h>
#include <TinyJPEGDecoder.h>
#include <TinyMaterialDesign.h>
#include <WiFi.h>
#ifdef ESP32
#include <Wire.h>
#endif
#include <cstdio>

#include "RadioImages.h"
#include "RadioOutput.h"
#include "StationDirectory.h"

constexpr const char* kWifiSsid = "your ssid";
constexpr const char* kWifiPassword = "your password";
constexpr size_t kWidth = 240;
constexpr size_t kHeight = 320;

#ifdef ESP32
LCDBoardESP32S3_2_8Display board;
#else
LCDBoardDesktopSDL board(kWidth, kHeight);
#endif

Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
DeviceOutput<RGB565> display(board.display());
GestureDetector gestures;
Screen<RGB565> screen;
MaterialTheme<RGB565> theme = defaultTheme<RGB565>();

struct TagEntry {
  const char* label;
  const char* value;  // radio-browser tag or countrycode
};
constexpr TagEntry kGenres[] = {
    {"Jazz", "jazz"},
    {"Rock", "rock"},
    {"Pop", "pop"},
    {"Classical", "classical"},
    {"Electronic", "electronic"},
    {"Hip-Hop", "hiphop"},
    {"Country", "country"},
    {"Latin", "latin"},
    {"Blues", "blues"},
    {"Folk", "folk"},
    {"Soul", "soul"},
    {"African", "african"},
    {"Caribbean", "caribbean"},
};
constexpr int kGenreCount = sizeof(kGenres) / sizeof(kGenres[0]);

constexpr TagEntry kCountries[] = {
    {"Switzerland", "ch"},   {"Germany", "de"}, {"Austria", "at"},
    {"France", "fr"},        {"Italy", "it"},   {"United Kingdom", "gb"},
    {"United States", "us"}, {"Spain", "es"},   {"Netherlands", "nl"},
    {"Sweden", "se"},        {"Belgium", "be"}, {"Portugal", "pt"},
    {"Poland", "pl"},        {"Canada", "ca"},
};

constexpr int kCountryCount = sizeof(kCountries) / sizeof(kCountries[0]);
constexpr int kMaxStations = 12;
constexpr int kMaxListItems =
    max(max(kGenreCount, kCountryCount),
        kMaxStations);  // >= kGenreCount, kCountryCount, kMaxStations

StationDirectory<kMaxStations> stationDirectory;
RadioOutput audioOutput(kWifiSsid, kWifiPassword);

AppBar<RGB565> appBar(Bounds(0, 0, kWidth, 48), "ESP32 Radio");
IconButton<RGB565> menuButton;

Drawer<RGB565> navDrawer(Bounds(0, 0, 200, kHeight));
ListItem<RGB565> genresItem(navDrawer.itemRect(0), "Radios by Genre",
                            drawChevronRight<RGB565>);
ListItem<RGB565> countriesItem(navDrawer.itemRect(1), "Radios by Country",
                               drawChevronRight<RGB565>);
ListItem<RGB565> switzerlandItem(navDrawer.itemRect(2), "Radios in Switzerland",
                                 drawChevronRight<RGB565>);
ListItem<RGB565> aboutItem(navDrawer.itemRect(3), "About",
                           drawChevronRight<RGB565>);

constexpr int32_t kListTop = 56;  // just below the 48px app bar
constexpr int32_t kListRowHeight = 32;
constexpr int32_t kListRowPitch = 36;
ListItem<RGB565> listPool[kMaxListItems];

constexpr int kMaxGridItems =
    max(max(kGenreCount, kCountryCount),
        kMaxStations);  // >= kGenreCount, kCountryCount
constexpr int32_t kGridCellSize = 80;
constexpr int32_t kGridWidth = 2 * kGridCellSize + 8;
GridLayout mediaGrid(Bounds((kWidth - kGridWidth) / 2, kListTop, kGridWidth, 0),
                     kGridCellSize, kGridCellSize);
MediaCard<RGB565> mediaCardPool[kMaxGridItems];
Surface<RGB565> gridImagePool[kMaxGridItems];

Dialog<RGB565> aboutDialog(Bounds(20, (kHeight - 180) / 2, kWidth - 40, 180),
                           "Status", "");
Button<RGB565> aboutDialogClose(Bounds(0, 0, 80, 36), "Close",
                                ButtonVariant::kText);

void renderNow() {
  screen.draw(surface, theme);
  display.writeData(surface);
}

void hideList() {
  for (auto& item : listPool) {
    item.visible = false;
    item.bounds = Bounds(0, 0, 0, 0);
  }
}

void hideGrid() {
  for (auto& card : mediaCardPool) {
    card.visible = false;
    card.bounds = Bounds(0, 0, 0, 0);
  }
}

void showStationList(int count) {
  hideGrid();
  for (int i = 0; i < kMaxListItems; ++i) {
    if (i < count) {
      listPool[i].visible = true;
      listPool[i].bounds =
          Bounds(16, kListTop + i * kListRowPitch, kWidth - 32, kListRowHeight);
    } else {
      listPool[i].visible = false;
      listPool[i].bounds = Bounds(0, 0, 0, 0);
    }
  }
  screen.invalidate();
}

void showGrid(int count) {
  hideList();
  for (int i = 0; i < kMaxGridItems; ++i) {
    if (i < count) {
      mediaCardPool[i].visible = true;
      mediaCardPool[i].bounds = mediaGrid.cellRect(i);
    } else {
      mediaCardPool[i].visible = false;
      mediaCardPool[i].bounds = Bounds(0, 0, 0, 0);
    }
  }
  screen.invalidate();
}

void playStation(int index) {
  if (index < 0 || index >= stationDirectory.count()) return;
  for (int i = 0; i < stationDirectory.count(); ++i)
    listPool[i].setSelected(i == index);
  Serial.print("Selected station: ");
  Serial.print(stationDirectory.name(index));
  Serial.print(" -> ");
  Serial.println(stationDirectory.url(index));
  audioOutput.setStation(stationDirectory.name(index),
                         stationDirectory.url(index));
}

void fetchStations(const String& url) {
  showStationList(0);  // clear the old list while the background fetch runs
  stationDirectory.requestFetch(url);
}

// Called from loop() once the background fetch task has new results.
void applyStationResults() {
  const int count = stationDirectory.count();
  if (count == 0) {
    showStationList(0);
    return;
  }

  String activeUrl = audioOutput.currentStationUrl();
  for (int i = 0; i < count; ++i) {
    listPool[i].setTitle(stationDirectory.name(i).c_str());
    listPool[i].setSelected(stationDirectory.url(i) == activeUrl);
    listPool[i].onClick = [i]() { playStation(i); };
  }
  showStationList(count);
}

void applyThumb(MediaCard<RGB565>& card, Surface<RGB565>& target,
                const EmbeddedJpeg* thumb) {
  card.setImage(nullptr);
  if (thumb == nullptr) return;
  JPEGParser<RGB565> parser(target);
  if (parser.decode(thumb->data, thumb->size)) {
    card.setImage(&target);
  } else {
    Serial.println(parser.errorMessage());
  }
}

void showGenres() {
  for (int i = 0; i < kGenreCount; ++i) {
    mediaCardPool[i].setCaption(kGenres[i].label);
    applyThumb(mediaCardPool[i], gridImagePool[i],
               findGenreThumb(kGenres[i].value));
    const char* value = kGenres[i].value;
    mediaCardPool[i].onClick = [value]() {
      fetchStations(StationDirectory<kMaxStations>::queryUrl("tag", value));
    };
  }
  showGrid(kGenreCount);
  screen.dismissDialog();
}

void showCountries() {
  for (int i = 0; i < kCountryCount; ++i) {
    mediaCardPool[i].setCaption(kCountries[i].label);
    applyThumb(mediaCardPool[i], gridImagePool[i],
               findFlagThumb(kCountries[i].value));
    const char* value = kCountries[i].value;
    mediaCardPool[i].onClick = [value]() {
      fetchStations(
          StationDirectory<kMaxStations>::queryUrl("countrycode", value));
    };
  }
  showGrid(kCountryCount);
  screen.dismissDialog();
}

void showSwitzerland() {
  screen.dismissDialog();
  fetchStations(StationDirectory<kMaxStations>::queryUrl("countrycode", "ch"));
}

void refreshAboutDialogBody() {
  String stationName = audioOutput.currentStationName();
  char buffer[160];
#ifdef ESP32
  snprintf(
      buffer, sizeof(buffer), "WiFi: %s\nHeap: %u bytes free\nStation: %s",
      WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "not connected",
      static_cast<unsigned>(ESP.getFreeHeap()),
      stationName.length() > 0 ? stationName.c_str() : "none");
#else
  snprintf(buffer, sizeof(buffer),
           "Desktop debug build (Arduino-Emulator + PortAudio)\nStation: %s",
           stationName.length() > 0 ? stationName.c_str() : "none");
#endif
  aboutDialog.setMessage(buffer);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

  board.begin();
  display.begin();
  surface.begin();

  appBar.setColorOverride(theme.colors.primary, theme.colors.onPrimary);

  menuButton = IconButton<RGB565>(appBar.leadingRect(), drawMenu<RGB565>);
  appBar.leading = &menuButton;
  menuButton.onClick = []() { screen.presentDialog(navDrawer); };

  genresItem.onClick = []() { showGenres(); };
  countriesItem.onClick = []() { showCountries(); };
  switzerlandItem.onClick = []() { showSwitzerland(); };
  aboutItem.onClick = []() {
    refreshAboutDialogBody();
    screen.dismissDialog();
    screen.presentDialog(aboutDialog);
  };
  for (ListItem<RGB565>* item :
       {&genresItem, &countriesItem, &switzerlandItem, &aboutItem}) {
    item->setTypographyRole(TypographyRole::kLabel);
  }
  navDrawer.addItem(genresItem);
  navDrawer.addItem(countriesItem);
  navDrawer.addItem(switzerlandItem);
  navDrawer.addItem(aboutItem);
  navDrawer.onScrimTap = []() { screen.dismissDialog(); };

  aboutDialogClose.bounds = aboutDialog.actionRect(0, 1);
  aboutDialogClose.onClick = []() { screen.dismissDialog(); };
  aboutDialog.addAction(aboutDialogClose);

  screen.addFixedWidget(appBar);

  for (auto& item : listPool) item.setTypographyRole(TypographyRole::kLabel);
  for (auto& item : listPool) screen.addWidget(item);
  for (auto& card : mediaCardPool) screen.addWidget(card);

  showGenres();  // esp32_radio's Vue router defaults to the Genres view too

  gestures.onGesture = [](GestureEvent& event) { screen.handleGesture(event); };
  gestures.isDraggable = [](int16_t x, int16_t y) {
    return screen.isDraggableAt(x, y);
  };

  audioOutput.begin();
  stationDirectory.begin();
}

void loop() {
  gestures.update(*board.touch());
  screen.update(millis());

  if (stationDirectory.consumeReady()) {
    applyStationResults();
  }

  if (screen.isDirty()) {
    screen.draw(surface, theme);
    display.writeData(surface);
  }
}
