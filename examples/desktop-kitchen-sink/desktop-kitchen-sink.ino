/**
 * @file desktop-kitchen-sink.ino
 * @brief One of every TinyMaterialDesign widget, running on TinyGPU's SDL2
 * desktop backend via LCDBoardDesktopSDL (display + mouse-as-touch in one
 * object) so it's clickable with a real mouse - no touch hardware needed to
 * exercise the whole interaction model.
 *
 * There's more content here than fits an actual small display at once, so
 * it's registered as Screen's scrollable content (the default for
 * addWidget() - see Core/Screen.h): drag up/down over any non-interactive
 * spot to scroll it. appBar and keyboard are registered via
 * addFixedWidget() instead, so they stay pinned to the top/bottom of the
 * window regardless of scroll position.
 */
#include <TinyMaterialDesign.h>
#include <TinyGPU/Boards/LCDBoardsSDL.h>
#include <cstdio>

constexpr size_t kWidth = 340;
// Window/viewport size - kept small enough to fit on an ordinary laptop
// display; the content below is taller than this and scrolls (see the file
// comment above), the same way it would need to on a real small panel.
constexpr size_t kHeight = 680;

LCDBoardDesktopSDL board(kWidth, kHeight);
Surface<RGB565> surface(kWidth, kHeight, FontRGB565);
DeviceOutput<RGB565> display(board.display());

GestureDetector gestures;
Screen<RGB565> screen;
MaterialTheme<RGB565> theme = defaultTheme<RGB565>();

// --- App bar -----------------------------------------------------------
AppBar<RGB565> appBar(Bounds(0, 0, kWidth, 48), "TinyMaterialDesign");
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
Label<RGB565> notificationsLabel(Bounds(48, 56, 220, 24), "Enable notifications");

// --- Text entry (both fields share one Keyboard - see setup()) -------------
TextField<RGB565> nameField(Bounds(16, 88, 220, 48), "Name", "Your name");
TextArea<RGB565> notesArea(Bounds(16, 144, 220, 90), "Notes", "Multi-line notes...");

Switch<RGB565> darkSwitch(Bounds(16, 256, 48, 28));
Label<RGB565> darkLabel(Bounds(72, 256, 220, 28), "Dark theme");

// --- Color scheme selector: exclusive-select filter chips over the named
// schemes from MaterialTheme.h (defaultTheme()/blueTheme()/greenTheme()/
// redTheme()/orangeTheme()), combined with darkSwitch above to pick that
// family's light or dark variant - see applyScheme() below. -----------
enum class SchemeFamily { kDefault, kBlue, kGreen, kRed, kOrange };
SchemeFamily currentScheme = SchemeFamily::kDefault;
constexpr int kSchemeCount = 5;
Chip<RGB565> schemeChips[kSchemeCount] = {
    Chip<RGB565>(Bounds(16, 292, 58, 28), "Std", /*selectable=*/true, /*selected=*/true),
    Chip<RGB565>(Bounds(78, 292, 58, 28), "Blue", /*selectable=*/true),
    Chip<RGB565>(Bounds(140, 292, 58, 28), "Green", /*selectable=*/true),
    Chip<RGB565>(Bounds(202, 292, 58, 28), "Red", /*selectable=*/true),
    Chip<RGB565>(Bounds(264, 292, 58, 28), "Org", /*selectable=*/true),
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
RadioButton<RGB565> radioA(Bounds(16, 332, 24, 24), true);
RadioButton<RGB565> radioB(Bounds(90, 332, 24, 24));
RadioButton<RGB565> radioC(Bounds(164, 332, 24, 24));
Label<RGB565> radioALabel(Bounds(44, 332, 30, 24), "A");
Label<RGB565> radioBLabel(Bounds(118, 332, 30, 24), "B");
Label<RGB565> radioCLabel(Bounds(192, 332, 30, 24), "C");

// --- Slider ---------------------------------------------------------------
Slider<RGB565> volumeSlider(Bounds(16, 376, 200, 24), 0.0f, 100.0f, 40.0f);
Label<RGB565> volumeValueLabel(Bounds(224, 372, 100, 24), "40");

// --- Buttons ---------------------------------------------------------------
Button<RGB565> filledButton(Bounds(16, 408, 92, 40), "Filled", ButtonVariant::kFilled);
Button<RGB565> tonalButton(Bounds(116, 408, 92, 40), "Tonal", ButtonVariant::kTonal);
Button<RGB565> outlinedButton(Bounds(216, 408, 92, 40), "Outlined", ButtonVariant::kOutlined);
Button<RGB565> textButton(Bounds(16, 456, 92, 40), "Text", ButtonVariant::kText);
Button<RGB565> elevatedButton(Bounds(116, 456, 92, 40), "Elevated", ButtonVariant::kElevated);

// --- Progress indicators ---------------------------------------------------
LinearProgressIndicator<RGB565> linearProgress(Bounds(16, 512, 200, 8), 0.0f, /*indeterminate=*/true);
CircularProgressIndicator<RGB565> circularProgress(Bounds(232, 496, 40, 40), 0.0f,
                                                  /*indeterminate=*/true);

// --- Chips -------------------------------------------------------------
Chip<RGB565> chipA(Bounds(16, 548, 90, 32), "Filter A", /*selectable=*/true, /*selected=*/true);
Chip<RGB565> chipB(Bounds(114, 548, 90, 32), "Filter B", /*selectable=*/true);

// --- Divider + card ---------------------------------------------------
Divider<RGB565> divider(Bounds(16, 592, kWidth - 32, 2));
Card<RGB565> infoCard(
    Bounds(16, 604, kWidth - 32, 140), "TinyMaterialDesign",
    "Header-only Material Design widgets, drawn with TinyGPU. Tap the + "
    "button in the app bar to open a dialog.");

// --- Floating action button (filled IconButton variant): a play/stop
// control that swaps icon and color on tap, the way esp32_radio's
// Start/Stop control does ---------------------------------------------
bool playing = false;
Label<RGB565> mediaLabel(Bounds(16, 752, 200, 24), "Now Playing", TypographyRole::kTitle);
IconButton<RGB565> playFab;

// --- Media card grid: image-backed, tappable tiles laid out with
// GridLayout (Core/GridLayout.h) - images are nullptr here (no HTTP/image
// decode in this desktop demo), so each card shows a plain placeholder
// fill plus its caption ----------------------------------------------
Label<RGB565> gridLabel(Bounds(16, 832, 200, 24), "Genres", TypographyRole::kTitle);
GridLayout genreGrid(Bounds(16, 864, kWidth - 32, 0), 90, 110);
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

  playFab = IconButton<RGB565>(Bounds(kWidth - 72, 752, 56, 56), drawPlus<RGB565>,
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

  // Fixed (see addFixedWidget()), so it stays docked to the window's
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
