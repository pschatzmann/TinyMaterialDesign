/**
 * @file esp32-radio.ino
 * @brief Internet radio player - a TinyMaterialDesign port of
 * https://github.com/pschatzmann/esp32_radio, reproducing its Vue.js GUI's
 * navigation structure (see vue-radio/src/components/NavigationMenu.vue):
 * a drawer with "Radios by Genre", "Radios by Country" and a shortcut into
 * one specific country - "Radios in Switzerland" here, hardcoded in place
 * of the original's IP-geolocated home country. Picking a genre/country
 * queries the same backend the original GUI used -
 * https://api.radio-browser.info - for a live list of stations, exactly
 * like esp32_radio's Radios.vue did via WebService.getRadios(field, value).
 * The screen is deliberately minimal: just the app bar plus a centered
 * grid/list of tappable cards/rows - no title/status text, no volume
 * slider, no play/stop button. Tapping a genre/country card fetches its
 * stations; tapping a station plays it (switching away from whatever was
 * already playing, if anything) - the list highlights whichever one that
 * is, which is all the "now playing" indication there is.
 *
 * Playback is done with arduino-audio-tools
 * (https://github.com/pschatzmann/arduino-audio-tools) instead of the
 * original's ESP8266Audio+BluetoothA2DPSink:
 *
 *  - ICYStream fetches the chosen station's MP3 stream.
 *  - MP3DecoderHelix -> I2SCodecStream decodes and plays it over this
 *    board's ES8311 codec + speaker amp. I2SCodecStream (unlike plain
 *    I2SStream) also drives the codec's own I2C init (volume, PA enable,
 *    ...) via arduino-audio-driver's ready-made ESP32S3HosyondDisplay
 *    board config (see the include below) - its I2C/I2S/PA-enable pins
 *    match this exact board (verified against TinyGPU's own
 *    LCDBoardESP32S3_2_8Display doc comment), so no manual pin setup is
 *    needed here at all.
 *
 * SD-card recording is not implemented in this example.
 *
 * Station browsing needs HTTPS, so this also uses the ESP32 core's
 * HTTPClient/WiFiClientSecure - requesting CSV rather than JSON output
 * (radio-browser supports both) so results can be parsed one line at a
 * time via a small hand-rolled RFC 4180 reader (see parseCsvLine()) with
 * no whole-response buffer and no extra parsing library.
 *
 * The genre/country pickers use MediaCard grids with real thumbnails -
 * the same genre photos and country flags esp32_radio's Genres.vue/
 * Countries.vue showed - via TinyGPU's JPEGParser (see RadioImages.h for
 * why they're re-encoded/embedded rather than fetched from the original
 * repo at runtime). The live station-search results underneath don't have
 * embedded art (radio-browser's per-station favicons are arbitrary
 * broadcaster logos, mostly PNG - the same decoder gap - so those stay
 * plain ListItem rows).
 *
 * Targets LCDBoardESP32S3_2_8Display (ESP32-S3 2.8" "Hosyond"/FBBA0125-002
 * display - ILI9341 SPI TFT + FT6336G capacitive touch + ES8311 audio
 * codec, *with* PSRAM, unlike the classic-ESP32 CYD this example
 * originally targeted - see TinyGPU/Boards/LCDBoardsESP32.h). Swap the
 * board type below for a different LCDBoard if your panel differs, and
 * adjust kWifiSsid/kWifiPassword below for your own network.
 *
 * Because this board has PSRAM, rendering is the plain, simple
 * Surface+DeviceOutput+Screen::draw() pattern (matches kitchen-sink.ino) -
 * a full 240x320 RGB565 framebuffer (153,600 bytes) allocates fine in
 * PSRAM (see TinyGPU/Util/PSRAMAllocator.h), so none of the per-widget
 * Screen::drawDirect() workaround the CYD's lack of PSRAM required is
 * needed here.
 *
 * Desktop debug build: like kitchen-sink.ino, #ifdef ESP32 picks
 * LCDBoardDesktopSDL (an SDL2 window, mouse-as-touch) instead of the real
 * board when built via examples/esp32-radio/CMakeLists.txt, so the layout
 * can be exercised on this machine without flashing hardware. Station
 * search and playback are real there too - the Arduino-Emulator's WiFi/
 * WiFiClient are thin wrappers over the host's own networking (see its
 * WiFi.h), so the same ICYStream/URLStream (arduino-audio-tools) code that
 * talks to api.radio-browser.info and the station streams on the real
 * board works unchanged on desktop. Only the codec output differs -
 * I2SCodecStream (real ES8311 DAC) on ESP32 vs. PortAudioStream (host
 * sound card) on desktop. https:// stations work on both - the desktop
 * build's Arduino-Emulator is fetched with its own USE_HTTPS CMake
 * option on (see examples/esp32-radio/CMakeLists.txt), which builds in
 * real TLS via wolfSSL; station search (stationsUrl() below) still uses
 * plain http://, since api.radio-browser.info serves both and there's no
 * reason to pay for a TLS handshake there. The genre/country thumbnails
 * (JPEG, via TinyGPU's portable, ESP32-independent JPEGParser) render on
 * both.
 */
#include <TinyMaterialDesign.h>
#include <TinyGPU/Boards/LCDBoards.h>
#ifdef ESP32
#include <Wire.h>
#endif
// WiFi.h works on both platforms - the Arduino-Emulator's own WiFi.h (see
// this file's header comment) stands in for the real ESP32 one on the
// desktop debug build.
#include <WiFi.h>
// TinyJPEG is portable C++17 (see its own docs) - used for the genre/
// country thumbnails on both ESP32 and the desktop debug build.
//
// Arduino's dependency scanner only discovers a library from an #include
// that appears directly in the sketch - one nested inside another
// library's header (here, JPEGParser.h's own #include <TinyJPEGDecoder.h>)
// isn't enough for it to add that library's -I path (the same gap Wire.h
// above needed working around for) - so it's included explicitly here too.
#include <TinyJPEGDecoder.h>
#include <TinyGPU/IO/JPEGParser.h>
// URLStream's (and so ICYStream's) own connect/read timeout defaults to
// 60s (URL_CLIENT_TIMEOUT in AudioToolsConfig.h) and isn't otherwise
// overridable per-instance for ICYStream - urlStream.begin() below runs
// on audioTask (see its declaration further down), so a single slow or
// unreachable station would otherwise block that task, and so any
// station switch, for up to a minute. Must be defined before AudioTools.h
// is first included (its own #ifndef guard only takes the first value).
#define URL_CLIENT_TIMEOUT 8000
// arduino-audio-tools (ICYStream/URLStream, MP3DecoderHelix, ...) is
// portable and works on both platforms - see this file's header comment.
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Communication/AudioHttp.h"
// WiFi connect and the streaming/decode/output pipeline run on a
// dedicated background task (see audioTask/audioTaskLoop() below) rather
// than in loop(), so a slow/unreachable station's connect - or the
// occasional multi-second wait for a WiFi association - doesn't freeze
// the UI (touch handling, rendering). AudioTools/Concurrency.h's Task
// wraps a FreeRTOS task on ESP32 and a std::thread on desktop behind the
// same begin(std::function<void()>)-called-in-a-loop API; only ESP32's
// own platform config (PlatformConfig/esp32.h) pulls this in
// automatically via AudioTools.h (USE_CONCURRENCY), so it's included
// explicitly here for the desktop build too.
#include "AudioTools/Concurrency.h"
#ifdef ESP32
#include "AudioTools/AudioLibs/I2SCodecStream.h"
// Same dependency-scanner gap as Wire.h/TinyJPEGDecoder.h above:
// I2SCodecStream.h pulls this in transitively (via AudioBoard.h's own
// #include "AudioBoards/AudioBoards.h"), which is enough for it to
// compile but not enough for Arduino's own library resolution to notice
// arduino-audio-driver is needed - included explicitly here too.
#include "AudioBoards/ESP32S3HosyondDisplay.h"
#else
#include "AudioTools/AudioLibs/PortAudioStream.h"
#endif
#include "RadioImages.h"
#include <atomic>
#include <cstdio>

// --- Network ---------------------------------------------------------------
constexpr const char* kWifiSsid = "your ssid";
constexpr const char* kWifiPassword = "your password";

constexpr size_t kWidth = 240;
constexpr size_t kHeight = 320;

#ifdef ESP32
LCDBoardESP32S3_2_8Display board;
#else
LCDBoardDesktopSDL board(kWidth, kHeight);
#endif

// PSRAM makes a plain full-screen framebuffer + Screen::draw() (same
// pattern as kitchen-sink.ino) straightforward here - see this file's
// header comment for why esp32-radio.ino previously needed
// Screen::drawDirect() instead.
Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
DeviceOutput<RGB565> display(board.display());

GestureDetector gestures;
Screen<RGB565> screen;
MaterialTheme<RGB565> theme = defaultTheme<RGB565>();


// --- Genre/country catalogs ----------------------------------------------
// esp32_radio's Genres/Countries views populated these lists from
// data/genres.json / data/countries.json hosted alongside the GUI. Those
// files aren't part of this repo, so this is a curated stand-in: the genre
// tags mirror the image filenames actually shipped in
// esp32_radio/vue-radio/public/genres/, and the countries are a practical
// subset that fits a small touchscreen list instead of the original's full
// ISO-3166 flag picker.
struct TagEntry {
  const char* label;
  const char* value;  // radio-browser tag or countrycode
};
constexpr TagEntry kGenres[] = {
    {"Jazz", "jazz"},       {"Rock", "rock"},         {"Pop", "pop"},
    {"Classical", "classical"}, {"Electronic", "electronic"}, {"Hip-Hop", "hiphop"},
    {"Country", "country"}, {"Latin", "latin"},       {"Blues", "blues"},
    {"Folk", "folk"},       {"Soul", "soul"},         {"African", "african"},
    {"Caribbean", "caribbean"},
};
constexpr int kGenreCount = 13;

constexpr TagEntry kCountries[] = {
    {"Switzerland", "ch"}, {"Germany", "de"},  {"Austria", "at"},
    {"France", "fr"},      {"Italy", "it"},    {"United Kingdom", "gb"},
    {"United States", "us"}, {"Spain", "es"},  {"Netherlands", "nl"},
    {"Sweden", "se"},      {"Belgium", "be"},  {"Portugal", "pt"},
    {"Poland", "pl"},      {"Canada", "ca"},
};
constexpr int kCountryCount = 14;

// --- Live station results (populated from api.radio-browser.info, or a
// few fixed mock rows on the desktop debug build) --------------------------
constexpr int kMaxStations = 12;
struct StationEntry {
  String name;
  String url;
};
StationEntry stations[kMaxStations];
int stationCount = 0;

constexpr int kMaxListItems = 14;  // >= kGenreCount, kCountryCount, kMaxStations

// currentStationName/currentStationUrl are written by the UI thread
// (playStation(), below) and read by audioTaskLoop() on the background
// audio task (see the "Audio pipeline" section) - stationMutex guards
// every access to either, since Arduino's String isn't thread-safe (a
// read racing a reallocation on the other thread can crash). Mutex on
// ESP32 wraps a real FreeRTOS semaphore; StdMutex on desktop wraps
// std::mutex - see the AudioTools/Concurrency.h include above.
#ifdef ESP32
using AppMutex = audio_tools::Mutex;
#else
using AppMutex = audio_tools::StdMutex;
#endif
AppMutex stationMutex;
String currentStationName;
String currentStationUrl;

// --- Audio pipeline: url -> decoder -> codec output -----------------------
// ICYStream/MP3DecoderHelix are plain arduino-audio-tools and work
// unchanged on both platforms (see this file's header comment) - only the
// output stream differs: I2SCodecStream drives the real ES8311 DAC on
// ESP32 (and, via VolumeSupport, also provides setVolume() below);
// PortAudioStream plays through the host's sound card on desktop.
//
// All of this - WiFi connect, dacOut.begin(), and the actual
// connect/decode/copy work - runs exclusively on audioTask (see
// audioTaskLoop() below), never on the UI thread/loop(), so a slow or
// unreachable station never freezes touch handling or rendering.
// Both ESP32 and the desktop debug build have a real TLS stack now (the
// latter via Arduino-Emulator's own USE_HTTPS/wolfSSL - see this file's
// header comment and examples/esp32-radio/CMakeLists.txt), and plenty of
// radio-browser stations are https:// - so, unlike stationsHttp below,
// this does NOT force a single plain WiFiClient: ICYStream's default
// constructor lets URLStream pick WiFiClient/WiFiClientSecure itself per
// station, per its URL scheme.
ICYStream urlStream;

// Station search (fetchStations() below) reuses this URLStream rather
// than creating one locally - see fetchStations()'s own comment for why.
// fetchStations() itself still runs on the UI thread (only WiFi setup and
// the streaming pipeline were asked to move to a background task), so no
// locking is needed around this one.
WiFiClient stationsClient;
URLStream stationsHttp(stationsClient);
#ifdef ESP32
I2SCodecStream dacOut(ESP32S3HosyondDisplay);
#else
PortAudioStream dacOut;
#endif
MP3DecoderHelix mp3Decoder;
EncodedAudioStream decoder(&dacOut, &mp3Decoder);
StreamCopy copier(decoder, urlStream);
std::atomic<bool> playing{false};  // read by the UI thread (About dialog)

// activeStationUrl tracks what audioTaskLoop() is actually streaming;
// compared each iteration against currentStationUrl to notice a new
// pick - owned exclusively by the audio task, no locking needed.
String activeStationUrl;
// Priority 2 / core 0: loop() (rendering + touch polling) runs on the
// Arduino core's own loopTask, priority 1, pinned to core 1 - without an
// explicit core/priority here, a tight-spinning loop() could starve this
// task of CPU time on the same core, leading to audio stutter or silence
// even though the pipeline itself is working. core/priority are ignored
// on desktop (std::thread there - see AudioTools/Concurrency/Desktop/
// Task.h), so this only changes ESP32 behavior.
audio_tools::Task audioTask("audio-task", 8192, 2, 0);

// --- App bar -------------------------------------------------------------
AppBar<RGB565> appBar(Bounds(0, 0, kWidth, 48), "ESP32 Radio");
IconButton<RGB565> menuButton;

// --- Navigation drawer: mirrors NavigationMenu.vue -------------------------
Drawer<RGB565> navDrawer(Bounds(0, 0, 200, kHeight));
ListItem<RGB565> genresItem(navDrawer.itemRect(0), "Radios by Genre", drawChevronRight<RGB565>);
ListItem<RGB565> countriesItem(navDrawer.itemRect(1), "Radios by Country", drawChevronRight<RGB565>);
ListItem<RGB565> switzerlandItem(navDrawer.itemRect(2), "Radios in Switzerland",
                                 drawChevronRight<RGB565>);
ListItem<RGB565> aboutItem(navDrawer.itemRect(3), "About", drawChevronRight<RGB565>);

// --- Scrollable content: two reusable widget pools - a MediaCard grid
// (genres/countries, with thumbnails) and a ListItem list (live station
// search results, no art available) - only one shown at a time (see
// showGrid()/showStationList()). No title/status text above either - see
// this file's header comment. -------------------------------------------
constexpr int32_t kListTop = 56;  // just below the 48px app bar
constexpr int32_t kListRowHeight = 32;
constexpr int32_t kListRowPitch = 36;
ListItem<RGB565> listPool[kMaxListItems];

constexpr int kMaxGridItems = 14;  // >= kGenreCount, kCountryCount
// 2 columns of 80px cards (8px default GridLayout spacing) = 168px total -
// centered in the 240px-wide screen (x = (240-168)/2 = 36), rather than
// left-aligned with the leftover space pushed to one side.
constexpr int32_t kGridCellSize = 80;
constexpr int32_t kGridWidth = 2 * kGridCellSize + 8;
GridLayout mediaGrid(Bounds((kWidth - kGridWidth) / 2, kListTop, kGridWidth, 0), kGridCellSize,
                     kGridCellSize);
MediaCard<RGB565> mediaCardPool[kMaxGridItems];
Surface<RGB565> gridImagePool[kMaxGridItems];

// --- Dialog: About / status (mirrors esp32_radio's /service/info) --------
Dialog<RGB565> aboutDialog(Bounds(20, (kHeight - 180) / 2, kWidth - 40, 180), "Status", "");
Button<RGB565> aboutDialogClose(Bounds(0, 0, 80, 36), "Close", ButtonVariant::kText);

// Draws the current widget state immediately, bypassing isDirty() - used
// before a blocking network call so the (empty, until results arrive)
// station list is actually shown instead of the screen appearing frozen.
void renderNow() {
  screen.draw(surface, theme);
  display.writeData(surface);
}

/// Hides+zeroes every list row so it doesn't inflate Screen's scroll-content
/// height (which ignores `visible`, see Screen::contentHeight()).
void hideList() {
  for (auto& item : listPool) {
    item.visible = false;
    item.bounds = Bounds(0, 0, 0, 0);
  }
}

/// Hides+zeroes every grid card - see hideList().
void hideGrid() {
  for (auto& card : mediaCardPool) {
    card.visible = false;
    card.bounds = Bounds(0, 0, 0, 0);
  }
}

/// Positions/shows the first `count` pool rows starting at kListTop,
/// hides the rest (see hideList()), and hides the grid pool - used for
/// live station search results.
void showStationList(int count) {
  hideGrid();
  for (int i = 0; i < kMaxListItems; ++i) {
    if (i < count) {
      listPool[i].visible = true;
      listPool[i].bounds = Bounds(16, kListTop + i * kListRowPitch, kWidth - 32, kListRowHeight);
    } else {
      listPool[i].visible = false;
      listPool[i].bounds = Bounds(0, 0, 0, 0);
    }
  }
  screen.invalidate();
}

/// Positions/shows the first `count` grid cards via mediaGrid, hides the
/// rest (see hideGrid()), and hides the list pool - used for the genre/
/// country pickers.
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

String stationsUrl(const char* field, const char* value) {
  // csv rather than json - see parseCsvLine()/fetchStations() below: a
  // line-at-a-time CSV reader needs neither a whole-response buffer nor
  // a JSON parsing library.
  // http, not https - see this file's header comment.
  String url = "http://de1.api.radio-browser.info/csv/stations/search?";
  url += field;
  url += "=";
  url += value;
  url += "&codec=MP3&hidebroken=true&order=clickcount&reverse=true&limit=";
  url += String(kMaxStations);
  return url;
}

// radio-browser's csv output is standard RFC 4180: comma-separated, a
// field containing a comma/quote/newline is wrapped in double quotes with
// literal quotes doubled ("") inside it. Parses up to `maxFields` fields
// of `line` into `fields` and returns how many were found - stops as soon
// as `maxFields` is reached without scanning the rest of the line, since
// fetchStations() below only ever needs the first few columns.
int parseCsvLine(const String& line, String* fields, int maxFields) {
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

void playStation(int index) {
  if (index < 0 || index >= stationCount) return;
  for (int i = 0; i < stationCount; ++i) listPool[i].setSelected(i == index);
  Serial.print("Selected station: ");
  Serial.print(stations[index].name);
  Serial.print(" -> ");
  Serial.println(stations[index].url);
  // Just records the pick - audioTaskLoop() (background task) notices
  // currentStationUrl changed and does the actual connect/stop, so this
  // returns immediately instead of blocking the UI on a network call.
  audio_tools::LockGuard guard(stationMutex);
  currentStationName = stations[index].name;
  currentStationUrl = stations[index].url;
}

/// Fetches stations from `url` (api.radio-browser.info, same backend
/// esp32_radio's WebService.getRadios() called) and fills the list pool -
/// the on-device equivalent of Radios.vue's getRadios(). Uses
/// arduino-audio-tools' own URLStream (rather than ESP32's HTTPClient) so
/// this runs unchanged on the desktop debug build too - see this file's
/// header comment.
void fetchStations(const String& url) {
  showStationList(0);
  renderNow();

  // stationsHttp is a file-scope global (see its declaration above),
  // reused rather than a local URLStream here - URLStream's destructor
  // deletes its internal client before its own HttpRequest member (which
  // still references that client) is destroyed, a use-after-free that
  // segfaults as soon as a stack-local URLStream goes out of scope.
  stationsHttp.setTimeout(8000);
  stationsHttp.addRequestHeader("User-Agent", "TinyMaterialDesign-ESP32Radio/1.0");
  // Without this, the server keeps the HTTP/1.1 connection alive and
  // connected() below never goes false once the body has been fully
  // read, since nothing else here tracks the response's Content-Length.
  stationsHttp.setConnectionClose(true);
  if (!stationsHttp.begin(url.c_str())) return;
  URLStream& http = stationsHttp;

  // csv columns (fixed by the API): changeuuid,stationuuid,serveruuid,
  // name,url,url_resolved,... - discard the header row, then read one
  // data row at a time (never the whole response at once).
  constexpr int kNameField = 3;
  constexpr int kUrlField = 4;
  constexpr int kUrlResolvedField = 5;
  constexpr int kCsvFieldsNeeded = kUrlResolvedField + 1;

  // Content-Length (not connected()/available()) drives the loop below -
  // the server may keep an HTTP/1.1 connection alive regardless of the
  // Connection: close request header above, in which case connected()
  // never goes false once the body has been fully read.
  const int expectedLength = http.contentLength();
  http.readStringUntil('\n');  // header row

  stationCount = 0;
  while (expectedLength < 0 || (int)http.totalRead() < expectedLength) {
    if (!http.available()) {
      if (!http.httpRequest().connected()) break;
      delay(1);  // wait for more data without starving the watchdog
      continue;
    }
    String line = http.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (stationCount >= kMaxStations) continue;  // keep draining to EOF

    String fields[kCsvFieldsNeeded];
    const int fieldCount = parseCsvLine(line, fields, kCsvFieldsNeeded);
    if (fieldCount <= kNameField) continue;
    const String& name = fields[kNameField];
    String streamUrl = (fieldCount > kUrlResolvedField) ? fields[kUrlResolvedField] : "";
    if (streamUrl.length() == 0 && fieldCount > kUrlField) streamUrl = fields[kUrlField];
    if (name.length() == 0 || streamUrl.length() == 0) continue;

    stations[stationCount].name = name;
    stations[stationCount].url = streamUrl;
    ++stationCount;
  }
  // No explicit http.end() here - ~URLStream() (http goes out of scope
  // below) already calls it once; calling it twice segfaults inside the
  // Arduino-Emulator's socket wrapper on the second connected() check.

  if (stationCount == 0) {
    showStationList(0);
    return;
  }

  String activeUrl;
  {
    audio_tools::LockGuard guard(stationMutex);
    activeUrl = currentStationUrl;
  }
  for (int i = 0; i < stationCount; ++i) {
    listPool[i].setTitle(stations[i].name.c_str());
    listPool[i].setSelected(stations[i].url == activeUrl);
    listPool[i].onClick = [i]() { playStation(i); };
  }
  showStationList(stationCount);
}

/// Decodes `thumb` (if not null) into `target` and hands it to `card`;
/// leaves the card on its plain placeholder fill if decoding fails or
/// there's no thumbnail for this key - see RadioImages.h.
void applyThumb(MediaCard<RGB565>& card, Surface<RGB565>& target, const EmbeddedJpeg* thumb) {
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
    applyThumb(mediaCardPool[i], gridImagePool[i], findGenreThumb(kGenres[i].value));
    const char* value = kGenres[i].value;
    mediaCardPool[i].onClick = [value]() { fetchStations(stationsUrl("tag", value)); };
  }
  showGrid(kGenreCount);
  screen.dismissDialog();
}

void showCountries() {
  for (int i = 0; i < kCountryCount; ++i) {
    mediaCardPool[i].setCaption(kCountries[i].label);
    applyThumb(mediaCardPool[i], gridImagePool[i], findFlagThumb(kCountries[i].value));
    const char* value = kCountries[i].value;
    mediaCardPool[i].onClick = [value]() { fetchStations(stationsUrl("countrycode", value)); };
  }
  showGrid(kCountryCount);
  screen.dismissDialog();
}

void showSwitzerland() {
  screen.dismissDialog();
  fetchStations(stationsUrl("countrycode", "ch"));
}

void refreshAboutDialogBody() {
  String stationName;
  {
    audio_tools::LockGuard guard(stationMutex);
    stationName = currentStationName;
  }
  char buffer[160];
#ifdef ESP32
  snprintf(buffer, sizeof(buffer),
           "WiFi: %s\nHeap: %u bytes free\nStation: %s",
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

// Everything below runs on audioTask (see its declaration above), never
// on the UI thread - stopStreaming()/startStreaming() touch urlStream/
// decoder/dacOut, none of which are safe to also touch from loop().

void stopStreaming() {
  if (!playing) return;
  urlStream.end();
  decoder.end();
  playing = false;
}

uint32_t lastProgressMs = 0;
uint32_t streamStartedMs = 0;
// Some stations' connections stall after the response headers - the
// underlying Client::read() then returns -1 forever (HttpChunkReader
// logs "client.read result -1" repeatedly) without AudioTools itself
// ever detecting the dead connection, so copier.copy() below would
// otherwise spin indefinitely without producing audio. If nothing has
// come through for this long, audioTaskLoop() force-reconnects instead.
// 15s (not, say, 8s) because some stations legitimately go quiet for a
// few seconds at a time (TLS record boundaries, network jitter) without
// actually being dead - too short a timeout just means reconnecting into
// the same brief lull repeatedly.
constexpr uint32_t kStallTimeoutMs = 15000;
// A station whose connection keeps dying gets retried with increasing
// delay between attempts (2s, 4s, 8s, ... capped at 32s) rather than
// hammered every kStallTimeoutMs - see audioTaskLoop()'s "not playing"
// branch. Resets once a station has been stable for a while (below).
uint32_t stallRetryDelayMs = 0;
uint32_t retryAtMs = 0;
constexpr uint32_t kMaxStallRetryDelayMs = 32000;
constexpr uint32_t kStableAfterMs = 20000;

void startStreaming(const String& url) {
  if (url.length() == 0) return;
  if (playing) stopStreaming();
  Serial.print("Connecting to: ");
  Serial.println(url.c_str());
  if (!urlStream.begin(url.c_str(), "audio/mp3")) {
    // Same backoff as a stall (see audioTaskLoop()) - an immediate
    // connect failure would otherwise retry just as fast next iteration.
    stallRetryDelayMs =
        stallRetryDelayMs == 0 ? 2000 : min(stallRetryDelayMs * 2, kMaxStallRetryDelayMs);
    retryAtMs = millis() + stallRetryDelayMs;
    Serial.print("  connect failed - retrying in ");
    Serial.print(stallRetryDelayMs);
    Serial.println("ms");
    return;
  }
  decoder.begin();
  playing = true;
  lastProgressMs = millis();
  streamStartedMs = lastProgressMs;
}

/// audioTask's loop body (see audioTask.begin() in setup()): connects to
/// WiFi once, then on every iteration either notices currentStationUrl
/// changed (set by playStation() on the UI thread) and (re)connects, or -
/// if already playing - copies one chunk of decoded audio. Never touches
/// screen/surface/display, all owned by the UI thread/loop().
void audioTaskLoop() {
  static bool wifiConnected = false;
  if (!wifiConnected) {
#ifdef ESP32
    WiFi.mode(WIFI_STA);
#endif
    WiFi.begin(kWifiSsid, kWifiPassword);
    while (WiFi.status() != WL_CONNECTED) {
      delay(300);
    }
    auto dacConfig = dacOut.defaultConfig(TX_MODE);
    dacOut.begin(dacConfig);
#ifdef ESP32
    dacOut.setVolume(0.6f);  // fixed - no on-screen volume control; ESP32
                             // only - PortAudioStream has no
                             // VolumeSupport, use the host's own volume
                             // control instead.
#endif
    // Some stations (esp. TLS ones, where a chunk's last partial TLS
    // record needs its own round trip to decrypt) deliver data in bursts
    // with small gaps between them rather than a steady trickle -
    // HttpChunkReader has no retry/backoff of its own, so a copy buffer
    // sized for a steady trickle (the 1024-byte default) empties during
    // those gaps and the DAC underruns, audible as choppy/breaking-up
    // audio. A larger buffer absorbs more of that burstiness before it
    // becomes audible.
    copier.resize(8192);
    wifiConnected = true;
    return;
  }

  String desired;
  {
    audio_tools::LockGuard guard(stationMutex);
    desired = currentStationUrl;
  }
  if (desired != activeStationUrl) {
    // A newly-picked station always cuts in immediately, even if the
    // previous one is mid-backoff below - only a stall retry of the same
    // URL waits out retryAtMs.
    activeStationUrl = desired;
    stallRetryDelayMs = 0;
    startStreaming(activeStationUrl);
    return;
  }

  if (playing) {
    if (copier.copy() > 0) {
      lastProgressMs = millis();
      if (millis() - streamStartedMs > kStableAfterMs) {
        stallRetryDelayMs = 0;  // been solid for a while - forget past trouble
      }
    } else if (millis() - lastProgressMs > kStallTimeoutMs) {
      stopStreaming();
      stallRetryDelayMs =
          stallRetryDelayMs == 0 ? 2000 : min(stallRetryDelayMs * 2, kMaxStallRetryDelayMs);
      retryAtMs = millis() + stallRetryDelayMs;
      Serial.print("Stream stalled - retrying in ");
      Serial.print(stallRetryDelayMs);
      Serial.println("ms");
    }
  } else if (activeStationUrl.length() > 0 && millis() >= retryAtMs) {
    startStreaming(activeStationUrl);
  } else {
    delay(5);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

  board.begin();
  // Color inversion (on) and output byte-swap (off) for this panel are
  // both set inside LCDBoardESP32S3_2_8Display::begin() just above now -
  // see that class's doc comment in TinyGPU/Boards/LCDBoardsESP32.h -
  // rather than overridden here per-sketch.
  display.begin();
  surface.begin();

  appBar.setColorOverride(theme.colors.primary, theme.colors.onPrimary);

  // menuButton picks up the app bar's own primary/onPrimary color
  // automatically (see AppBar::draw()'s setThemeTint() call).
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
  // Smaller (kLabel, not the default kBody) text for the drawer - a denser
  // list reads fine here, unlike the station list rows, which stay at the
  // default size.
  for (ListItem<RGB565>* item : {&genresItem, &countriesItem, &switzerlandItem, &aboutItem}) {
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

  // Smaller (kLabel, not the default kBody) text - station names are
  // often long ("SomaFM Secret Agent (128k mp3)"), and a denser row reads
  // fine here, same rationale as the drawer's own items above.
  for (auto& item : listPool) item.setTypographyRole(TypographyRole::kLabel);
  for (auto& item : listPool) screen.addWidget(item);
  for (auto& card : mediaCardPool) screen.addWidget(card);

  // navDrawer/aboutDialog are NOT added here - presented modally via
  // screen.presentDialog(), same mechanism as kitchen-sink.ino.

  showGenres();  // esp32_radio's Vue router defaults to the Genres view too

  gestures.onGesture = [](GestureEvent& event) { screen.handleGesture(event); };
  gestures.isDraggable = [](int16_t x, int16_t y) { return screen.isDraggableAt(x, y); };

  // WiFi connect, dacOut.begin() and the streaming pipeline all happen on
  // this task's first and subsequent iterations (audioTaskLoop() above),
  // not here - setup() returns immediately into loop() rather than
  // blocking on WiFi association.
  audioTask.begin(audioTaskLoop);
}

void loop() {
  gestures.update(*board.touch());
  screen.update(millis());

  if (screen.isDirty()) {
    screen.draw(surface, theme);
    display.writeData(surface);
  }
}
