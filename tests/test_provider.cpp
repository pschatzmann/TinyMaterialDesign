// Covers: callback-driven "provider" mode (setChildProvider()/
// setActionProvider()/setItemProvider()) on Container, Dialog, Carousel,
// Banner, and Drawer - a large logical item count served by a small
// reused pool of real Widgets, with no per-item storage.
#include <TinyMaterialDesign.h>

#include <array>

#include "tmd_test.h"

using namespace tinygpu;
using namespace tinymd;

int main() {
  Surface<RGB565> surface(240, 320, FontRGB565);
  surface.begin();
  MaterialTheme<RGB565> theme = defaultTheme<RGB565>();
  Screen<RGB565> screen(theme);

  // --- Container: 100,000 logical rows, a pool of only 4 real widgets.
  constexpr int kLogicalCount = 100000;
  constexpr int kPoolSize = 4;
  std::array<Button<RGB565>, kPoolSize> pool = {
      Button<RGB565>(Bounds(), ""), Button<RGB565>(Bounds(), ""),
      Button<RGB565>(Bounds(), ""), Button<RGB565>(Bounds(), "")};

  Container<RGB565> bigList(Bounds(0, 0, 240, 200));
  bigList.setUniformItemHeight(40);  // O(1) contentHeight() at this scale
  bigList.setChildProvider(
      [&]() { return kLogicalCount; },
      [&](int index) -> Widget<RGB565>& {
        Button<RGB565>& b = pool[static_cast<size_t>(index % kPoolSize)];
        b.bounds = Bounds(0, index * 40, 240, 40);
        return b;
      });

  screen.addWidget(bigList);
  screen.draw(surface);  // must not construct 100,000 widgets
  TMD_CHECK(bigList.childCount() == kLogicalCount);

  int clickedIndex = -1;
  pool[0].onClick = [&]() { clickedIndex = 0; };
  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {50, 10};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(clickedIndex == 0);

  // --- Dialog: provider-based actions.
  Dialog<RGB565> dialog(Bounds(20, 20, 200, 150), "T", "M");
  std::array<Button<RGB565>, 2> dialogActions = {Button<RGB565>(Bounds(), "OK"),
                                                 Button<RGB565>(Bounds(), "Cancel")};
  dialog.setActionProvider([]() { return 2; },
                           [&](int i) -> Widget<RGB565>& {
                             dialogActions[static_cast<size_t>(i)].bounds = dialog.actionRect(i, 2);
                             return dialogActions[static_cast<size_t>(i)];
                           });
  int okClicks = 0;
  dialogActions[0].onClick = [&]() { okClicks++; };
  screen.presentDialog(dialog);
  screen.draw(surface);
  {
    Bounds okBounds = dialog.actionRect(0, 2);
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {static_cast<int16_t>(okBounds.centerX()), static_cast<int16_t>(okBounds.centerY())};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(okClicks == 1);
  screen.dismissDialog();

  // --- Carousel: provider-based items, bounds recomputed per fetch.
  Carousel<RGB565> carousel(Bounds(0, 0, 240, 150), 100, 10);
  std::array<MediaCard<RGB565>, 3> cards = {MediaCard<RGB565>(Bounds(), "A"),
                                            MediaCard<RGB565>(Bounds(), "B"),
                                            MediaCard<RGB565>(Bounds(), "C")};
  carousel.setItemProvider([]() { return 50; },
                           [&](int i) -> Widget<RGB565>& { return cards[static_cast<size_t>(i % 3)]; });
  TMD_CHECK(carousel.pageCount() == 50);
  carousel.setCurrentIndex(49, false);
  TMD_CHECK(carousel.currentIndex() == 49);

  // --- Banner: provider-based actions.
  Banner<RGB565> banner(Bounds(0, 0, 240, 60), "Offline");
  std::array<Button<RGB565>, 1> bannerActions = {Button<RGB565>(Bounds(), "Retry")};
  int retryClicks = 0;
  bannerActions[0].onClick = [&]() { retryClicks++; };
  banner.setActionProvider([]() { return 1; },
                           [&](int i) -> Widget<RGB565>& {
                             bannerActions[static_cast<size_t>(i)].bounds = banner.actionRect(i, 1);
                             return bannerActions[static_cast<size_t>(i)];
                           });
  screen.addWidget(banner);
  screen.draw(surface);
  {
    Bounds retryBounds = banner.actionRect(0, 1);
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {static_cast<int16_t>(retryBounds.centerX()), static_cast<int16_t>(retryBounds.centerY())};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(retryClicks == 1);

  // --- Drawer: provider-based items (delegates to internal Container).
  Drawer<RGB565> drawer(Bounds(0, 0, 220, 320));
  std::array<ListItem<RGB565>, 4> drawerPool = {
      ListItem<RGB565>(Bounds(), "x"), ListItem<RGB565>(Bounds(), "x"),
      ListItem<RGB565>(Bounds(), "x"), ListItem<RGB565>(Bounds(), "x")};
  int drawerClicks = 0;
  for (auto& item : drawerPool) item.onClick = [&]() { drawerClicks++; };
  drawer.setItemProvider([]() { return 500; },
                         [&](int i) -> Widget<RGB565>& {
                           ListItem<RGB565>& item = drawerPool[static_cast<size_t>(i % 4)];
                           item.bounds = drawer.itemRect(i);
                           return item;
                         });
  screen.presentDialog(drawer);
  screen.draw(surface);
  TMD_CHECK(drawer.childCount() == 500);
  {
    GestureEvent e;
    e.type = GestureType::kTap;
    e.phase = GesturePhase::kEnded;
    e.point = {50, 10};
    e.startPoint = e.point;
    screen.handleGesture(e);
  }
  TMD_CHECK(drawerClicks == 1);
  screen.dismissDialog();

  if (tmd_test_ok) printf("test_provider: PASSED\n");
  return tmd_test_ok ? 0 : 1;
}
