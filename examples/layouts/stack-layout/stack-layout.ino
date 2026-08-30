/**
 * @file stack-layout.ino
 * @brief Minimal, self-contained demo of TinyMaterialDesign's `StackLayout`.
 *
 * Builds and runs unchanged on a real ESP32 board (see kitchen-sink.ino for
 * the board-selection pattern) and, via LCDBoardDesktopSDL, in a desktop
 * SDL2 window for mouse-driven testing without touch hardware.
 */
#include <TinyMaterialDesign.h>

App<RGB565> app(DefaultBoard);

// --- StackLayout (the control under test) -----------------------------------
// Not a widget - two unrelated single-purpose helpers bundled under one
// name, each demonstrated separately below:
// - `panel`/`centeredIcon` show centered(): centers one rect inside a
//   container - here, an icon in the middle of a card.
// - `avatars` show offset(): places a series of same-size rects, each
//   shifted `overlapStep` pixels from the last, starting at a given
//   origin - here, three plain colored circles (standing in for profile
//   pictures) fanned into the classic overlapping-avatar-stack look. A
//   circular IconButton (not a rectangular Button) is what actually sells
//   this - each one reads as a circle peeking out from behind the next,
//   rather than a clipped-looking rectangle.
Card<RGB565> panel(Bounds(10, 30, app.width() - 20, 90), nullptr, nullptr);
IconButton<RGB565> centeredIcon(Bounds(), drawPlus<RGB565>, IconButtonVariant::kFilled);

IconButton<RGB565> avatarA(Bounds(), nullptr, IconButtonVariant::kFilled);
IconButton<RGB565> avatarB(Bounds(), nullptr, IconButtonVariant::kFilled);
IconButton<RGB565> avatarC(Bounds(), nullptr, IconButtonVariant::kFilled);
IconButton<RGB565>* avatars[] = {&avatarA, &avatarB, &avatarC};

void setup() {
  app.begin();

  StackLayout iconStack(panel.bounds);
  centeredIcon.bounds = iconStack.centered(48, 48);
  centeredIcon.onClick = []() { printf("Centered icon tapped\n"); };

  // Only the (30, 160) origin matters to offset() - width/height are
  // ignored, so they're left at 0 here rather than a real container size.
  StackLayout avatarStack(Bounds(30, 160, 0, 0), /*overlapStep=*/28);
  RGB565 avatarColors[] = {app.theme().colors.primary, app.theme().colors.secondary, app.theme().colors.error};
  for (size_t i = 0; i < 3; ++i) {
    avatars[i]->bounds = avatarStack.offset(static_cast<int>(i), 48, 48);
    avatars[i]->setColorOverride(avatarColors[i], app.theme().colors.onPrimary);
  }

  app.screen().addWidget(panel);
  app.screen().addWidget(centeredIcon);
  for (auto* avatar : avatars) app.screen().addWidget(*avatar);
}

void loop() {
  // App tracks whether anything actually changed - see Screen::isDirty()
  // - so an idle frame skips both the full-screen redraw and the full-
  // framebuffer display write.
  app.update();
}
