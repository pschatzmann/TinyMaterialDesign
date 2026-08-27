/**
 * @file kitchen-sink.ino
 * @brief One of every TinyMaterialDesign widget, laid out at 240x320 - the
 * same panel size as the ESP32 Cheap Yellow Display (ESP32-2432S028R) via
 * LCDBoardGuitionESP32_LVGL_2_4Display - so this sketch is unchanged source
 * that runs on that real hardware, and, via LCDBoardDesktopSDL, in an
 * identically-sized SDL2 window on desktop for mouse-driven testing/
 * debugging without touch hardware (see TinyGPU's basic-example.ino for the
 * same #ifdef ESP32 pattern this follows). Swap the board type below for a
 * different LCDBoard if your real panel differs - see
 * TinyGPU/Boards/LCDBoardsESP32.h for the full list/pinouts.
 *
 * There's more content here than fits 240x320 at once, so it's registered
 * as Screen's scrollable content (the default for addWidget() - see
 * Core/Screen.h): drag up/down over any non-interactive spot to scroll it.
 * appBar and keyboard are registered via addFixedWidget() instead, so they
 * stay pinned to the top/bottom of the screen regardless of scroll
 * position.
 */
#include <TinyMaterialDesign.h>
#include <TinyGPU/Boards/LCDBoards.h>
#include <cstdio>

// Matches LCDBoardGuitionESP32_LVGL_2_4Display's fixed 240x320 ILI9341
// panel - kept identical on desktop (LCDBoardDesktopSDL below) so this one
// layout previews accurately on both.
constexpr size_t kWidth = 240;
constexpr size_t kHeight = 320;

#ifdef ESP32
LCDBoardGuitionESP32_LVGL_2_4Display board;
#else
LCDBoardDesktopSDL board(kWidth, kHeight);
#endif
Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
DeviceOutput<RGB565> display(board.display());

GestureDetector gestures;
Screen<RGB565> screen;
MaterialTheme<RGB565> theme = defaultTheme<RGB565>();

// --- App bar -----------------------------------------------------------
AppBar<RGB565> appBar(Bounds(0, 0, kWidth, 48), "Kitchen Sink");
IconButton<RGB565> addButton;
IconButton<RGB565> menuButton;

// --- Navigation drawer, opened via the app bar's leading menu icon
// (presented modally, like Dialog - see setup()) --------------------------
Drawer<RGB565> navDrawer(Bounds(0, 0, 220, kHeight));
ListItem<RGB565> radiosItem(navDrawer.itemRect(0), "Radios", drawChevronRight<RGB565>);
ListItem<RGB565> countriesItem(navDrawer.itemRect(1), "Countries", drawChevronRight<RGB565>);
ListItem<RGB565> genresItem(navDrawer.itemRect(2), "Genres", drawChevronRight<RGB565>);
ListItem<RGB565> aboutItem(navDrawer.itemRect(3), "About", drawChevronRight<RGB565>);

// --- Selection controls --------------------------------------------------
Checkbox<RGB565> notificationsCheckbox(Bounds(16, 56, 24, 24), true);
Label<RGB565> notificationsLabel(Bounds(48, 56, 176, 24), "Enable notifications");

// --- Text entry (both fields share one Keyboard - see setup()) -------------
TextField<RGB565> nameField(Bounds(16, 88, 208, 48), "Name", "Your name");
TextArea<RGB565> notesArea(Bounds(16, 144, 208, 90), "Notes", "Multi-line notes...");

Switch<RGB565> darkSwitch(Bounds(16, 256, 48, 28));
Label<RGB565> darkLabel(Bounds(72, 256, 150, 28), "Dark theme");

// --- Color scheme selector: exclusive-select filter chips over the named
// schemes from MaterialTheme.h (defaultTheme()/blueTheme()/greenTheme()/
// redTheme()/orangeTheme()), combined with darkSwitch above to pick that
// family's light or dark variant - see applyScheme() below. Wrapped over
// two rows (3 + 2) since all 5 don't fit one row at this width. -----------
enum class SchemeFamily { kDefault, kBlue, kGreen, kRed, kOrange };
SchemeFamily currentScheme = SchemeFamily::kDefault;
constexpr int kSchemeCount = 5;
Chip<RGB565> schemeChips[kSchemeCount] = {
    Chip<RGB565>(Bounds(16, 292, 64, 28), "Std", /*selectable=*/true, /*selected=*/true),
    Chip<RGB565>(Bounds(88, 292, 64, 28), "Blue", /*selectable=*/true),
    Chip<RGB565>(Bounds(160, 292, 64, 28), "Green", /*selectable=*/true),
    Chip<RGB565>(Bounds(16, 328, 64, 28), "Red", /*selectable=*/true),
    Chip<RGB565>(Bounds(88, 328, 64, 28), "Org", /*selectable=*/true),
};

/// Rebuilds the global `theme` from currentScheme + darkSwitch, and keeps
/// the app bar's setColorOverride() (see AppBar.h) in sync with the new
/// scheme's primary color so the title bar visibly reflects the choice.
void applyScheme() {
  const bool dark = darkSwitch.value();
  switch (currentScheme) {
    case SchemeFamily::kBlue:
      theme = dark ? blueDarkTheme<RGB565>() : blueTheme<RGB565>();
      break;
    case SchemeFamily::kGreen:
      theme = dark ? greenDarkTheme<RGB565>() : greenTheme<RGB565>();
      break;
    case SchemeFamily::kRed:
      theme = dark ? redDarkTheme<RGB565>() : redTheme<RGB565>();
      break;
    case SchemeFamily::kOrange:
      theme = dark ? orangeDarkTheme<RGB565>() : orangeTheme<RGB565>();
      break;
    case SchemeFamily::kDefault:
    default:
      theme = dark ? defaultDarkTheme<RGB565>() : defaultTheme<RGB565>();
      break;
  }
  appBar.setColorOverride(theme.colors.primary, theme.colors.onPrimary);
}

RadioGroup<RGB565> radioGroup;
RadioButton<RGB565> radioA(Bounds(16, 368, 24, 24), true);
RadioButton<RGB565> radioB(Bounds(90, 368, 24, 24));
RadioButton<RGB565> radioC(Bounds(164, 368, 24, 24));
Label<RGB565> radioALabel(Bounds(44, 368, 30, 24), "A");
Label<RGB565> radioBLabel(Bounds(118, 368, 30, 24), "B");
Label<RGB565> radioCLabel(Bounds(192, 368, 30, 24), "C");

// --- Slider ---------------------------------------------------------------
Slider<RGB565> volumeSlider(Bounds(16, 412, 160, 24), 0.0f, 100.0f, 40.0f);
Label<RGB565> volumeValueLabel(Bounds(184, 408, 40, 24), "40");

// --- Buttons: one per row pair (Filled+Tonal, Outlined+Text, Elevated
// alone) rather than 3+2, since 3 across no longer fits at this width. ----
Button<RGB565> filledButton(Bounds(16, 444, 92, 40), "Filled", ButtonVariant::kFilled);
Button<RGB565> tonalButton(Bounds(116, 444, 92, 40), "Tonal", ButtonVariant::kTonal);
Button<RGB565> outlinedButton(Bounds(16, 492, 92, 40), "Outlined", ButtonVariant::kOutlined);
Button<RGB565> textButton(Bounds(116, 492, 92, 40), "Text", ButtonVariant::kText);
Button<RGB565> elevatedButton(Bounds(16, 540, 92, 40), "Elevated", ButtonVariant::kElevated);

// --- Progress indicators ---------------------------------------------------
CircularProgressIndicator<RGB565> circularProgress(Bounds(184, 580, 40, 40), 0.0f,
                                                  /*indeterminate=*/true);
LinearProgressIndicator<RGB565> linearProgress(Bounds(16, 596, 160, 8), 0.0f, /*indeterminate=*/true);

// --- Chips -------------------------------------------------------------
Chip<RGB565> chipA(Bounds(16, 628, 90, 32), "Filter A", /*selectable=*/true, /*selected=*/true);
Chip<RGB565> chipB(Bounds(114, 628, 90, 32), "Filter B", /*selectable=*/true);

// --- Divider + card ---------------------------------------------------
Divider<RGB565> divider(Bounds(16, 668, kWidth - 32, 2));
Card<RGB565> infoCard(
    Bounds(16, 678, kWidth - 32, 140), "TinyMaterialDesign",
    "Header-only Material Design widgets, drawn with TinyGPU. Tap the + "
    "button in the app bar to open a dialog.");

// --- Floating action button (filled IconButton variant): a play/stop
// control that swaps icon and color on tap, the way esp32_radio's
// Start/Stop control does ---------------------------------------------
bool playing = false;
Label<RGB565> mediaLabel(Bounds(16, 826, 200, 24), "Now Playing", TypographyRole::kTitle);
IconButton<RGB565> playFab;

// --- Media card grid: image-backed, tappable tiles laid out with
// GridLayout (Core/GridLayout.h) - images are nullptr here (no HTTP/image
// decode in this demo), so each card shows a plain placeholder fill plus
// its caption. At this width GridLayout wraps to 2 columns automatically -
// no layout math needed here beyond the container width. ------------------
Label<RGB565> gridLabel(Bounds(16, 890, 200, 24), "Genres", TypographyRole::kTitle);
GridLayout genreGrid(Bounds(16, 922, kWidth - 32, 0), 90, 110);
MediaCard<RGB565> genreCards[4] = {
    MediaCard<RGB565>(genreGrid.cellRect(0), "Jazz"),
    MediaCard<RGB565>(genreGrid.cellRect(1), "Rock"),
    MediaCard<RGB565>(genreGrid.cellRect(2), "Pop"),
    MediaCard<RGB565>(genreGrid.cellRect(3), "Blues"),
};

// --- On-screen keyboard (hidden until a text field is tapped) -----------
Keyboard<RGB565> keyboard(Bounds(0, kHeight - 170, kWidth, 170));

// --- Dialog (hidden until requested) ---------------------------------
Dialog<RGB565> infoDialog(Bounds(40, (kHeight - 160) / 2, kWidth - 80, 160), "Hello",
                          "This is a TinyMaterialDesign dialog.");
Button<RGB565> dialogCancel(Bounds(0, 0, 80, 36), "Cancel", ButtonVariant::kText);
Button<RGB565> dialogOk(Bounds(0, 0, 80, 36), "OK", ButtonVariant::kFilled);

void setup() {
  board.begin();
  display.begin();
  surface.begin();

  darkSwitch.onChange = [](bool) { applyScheme(); };
  for (int i = 0; i < kSchemeCount; ++i) {
    schemeChips[i].onClick = [i]() {
      for (int j = 0; j < kSchemeCount; ++j) schemeChips[j].setSelected(j == i);
      currentScheme = static_cast<SchemeFamily>(i);
      applyScheme();
    };
  }
  applyScheme();

  addButton = IconButton<RGB565>(appBar.trailingRect(), drawPlus<RGB565>);
  appBar.trailing = &addButton;
  addButton.onClick = []() { screen.presentDialog(infoDialog); };

  menuButton = IconButton<RGB565>(appBar.leadingRect(), drawMenu<RGB565>);
  appBar.leading = &menuButton;
  menuButton.onClick = []() { screen.presentDialog(navDrawer); };

  radiosItem.setSelected(true);
  radiosItem.onClick = []() {
    printf("Nav: Radios\n");
    screen.dismissDialog();
  };
  countriesItem.onClick = []() {
    printf("Nav: Countries\n");
    screen.dismissDialog();
  };
  genresItem.onClick = []() {
    printf("Nav: Genres\n");
    screen.dismissDialog();
  };
  aboutItem.onClick = []() {
    printf("Nav: About\n");
    screen.dismissDialog();
  };
  navDrawer.onScrimTap = []() { screen.dismissDialog(); };
  navDrawer.addItem(radiosItem);
  navDrawer.addItem(countriesItem);
  navDrawer.addItem(genresItem);
  navDrawer.addItem(aboutItem);

  playFab = IconButton<RGB565>(Bounds(kWidth - 72, 826, 56, 56), drawPlus<RGB565>,
                               IconButtonVariant::kFilled);
  playFab.setColorOverride(colorFromHex<RGB565>(0x2E7D32), colorFromHex<RGB565>(0xFFFFFF));
  playFab.onClick = []() {
    playing = !playing;
    playFab.setIcon(playing ? drawClose<RGB565> : drawPlus<RGB565>);
    playFab.setColorOverride(
        playing ? colorFromHex<RGB565>(0xC62828) : colorFromHex<RGB565>(0x2E7D32),
        colorFromHex<RGB565>(0xFFFFFF));
    printf(playing ? "Playing\n" : "Stopped\n");
  };

  genreCards[0].onClick = []() { printf("Genre tapped: Jazz\n"); };
  genreCards[1].onClick = []() { printf("Genre tapped: Rock\n"); };
  genreCards[2].onClick = []() { printf("Genre tapped: Pop\n"); };
  genreCards[3].onClick = []() { printf("Genre tapped: Blues\n"); };

  // One Keyboard drives both fields - manage() blurs whichever one it was
  // previously targeting when you tap the other.
  keyboard.manage(nameField);
  keyboard.manage(notesArea);
  nameField.onSubmit = []() { printf("Name submitted: %s\n", nameField.text().c_str()); };

  volumeSlider.onChange = [](float value) {
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(value));
    volumeValueLabel.setText(buffer);
  };

  filledButton.onClick = []() { printf("Filled button tapped\n"); };
  tonalButton.onClick = []() { printf("Tonal button tapped\n"); };
  outlinedButton.onClick = []() { printf("Outlined button tapped\n"); };
  textButton.onClick = []() { printf("Text button tapped\n"); };
  elevatedButton.onClick = []() { printf("Elevated button tapped\n"); };

  dialogCancel.bounds = infoDialog.actionRect(0, 2);
  dialogOk.bounds = infoDialog.actionRect(1, 2);
  dialogCancel.onClick = []() { screen.dismissDialog(); };
  dialogOk.onClick = []() {
    printf("Dialog OK tapped\n");
    screen.dismissDialog();
  };
  infoDialog.addAction(dialogCancel);
  infoDialog.addAction(dialogOk);

  screen.addFixedWidget(appBar);
  screen.addWidget(notificationsCheckbox);
  screen.addWidget(notificationsLabel);
  screen.addWidget(nameField);
  screen.addWidget(notesArea);
  screen.addWidget(darkSwitch);
  screen.addWidget(darkLabel);

  for (auto& chip : schemeChips) screen.addWidget(chip);

  radioGroup.addButton(radioA);
  radioGroup.addButton(radioB);
  radioGroup.addButton(radioC);
  screen.addWidget(radioA);
  screen.addWidget(radioB);
  screen.addWidget(radioC);
  screen.addWidget(radioALabel);
  screen.addWidget(radioBLabel);
  screen.addWidget(radioCLabel);

  screen.addWidget(volumeSlider);
  screen.addWidget(volumeValueLabel);

  screen.addWidget(filledButton);
  screen.addWidget(tonalButton);
  screen.addWidget(outlinedButton);
  screen.addWidget(textButton);
  screen.addWidget(elevatedButton);

  screen.addWidget(linearProgress);
  screen.addWidget(circularProgress);

  screen.addWidget(chipA);
  screen.addWidget(chipB);

  screen.addWidget(divider);
  screen.addWidget(infoCard);

  screen.addWidget(mediaLabel);
  screen.addWidget(playFab);

  screen.addWidget(gridLabel);
  for (auto& card : genreCards) screen.addWidget(card);

  // navDrawer is NOT added here - it's presented modally via
  // screen.presentDialog(), same mechanism as infoDialog.

  // Fixed (see addFixedWidget()), so it stays docked to the screen's
  // bottom edge - and drawn on top of - the scrollable content above,
  // regardless of scroll position, instead of scrolling away with it.
  screen.addFixedWidget(keyboard);

  gestures.onGesture = [](GestureEvent& event) { screen.handleGesture(event); };
  gestures.isDraggable = [](int16_t x, int16_t y) { return screen.isDraggableAt(x, y); };
}

void loop() {
  gestures.update(*board.touch());
  screen.update(millis());
  screen.draw(surface, theme);
  display.writeData(surface);
}
