// Covers: Drawer/Dialog/TabBar/NavigationRail/Carousel accepting more
// items/actions/tabs/destinations than their old (now-removed) fixed
// array caps - all converted to std::vector.
#include <TinyMaterialDesign.h>

#include <vector>

#include "tmd_test.h"

using namespace tinygpu;
using namespace tinymd;

int main() {
  MaterialTheme<RGB565> theme = defaultTheme<RGB565>();

  // Drawer: old cap was 8 items - add 12.
  Drawer<RGB565> drawer(Bounds(0, 0, 220, 320));
  std::vector<ListItem<RGB565>*> items;
  for (int i = 0; i < 12; ++i) {
    auto* item = new ListItem<RGB565>(drawer.itemRect(i), "Item");
    items.push_back(item);
    drawer.addItem(*item);
  }
  TMD_CHECK(drawer.childCount() == 12);
  for (int i = 0; i < 12; ++i) TMD_CHECK(drawer.child(i) == items[static_cast<size_t>(i)]);

  // Dialog: old cap was 2 actions - add 4.
  Dialog<RGB565> dialog(Bounds(20, 20, 200, 200), "T", "M");
  Button<RGB565> a1(Bounds(), "1"), a2(Bounds(), "2"), a3(Bounds(), "3"), a4(Bounds(), "4");
  dialog.addAction(a1);
  dialog.addAction(a2);
  dialog.addAction(a3);
  dialog.addAction(a4);

  // TabBar: old cap was 6 - add 9.
  TabBar<RGB565> tabs(Bounds(0, 0, 240, 40));
  for (int i = 0; i < 9; ++i) tabs.addTab("Tab");
  tabs.setSelectedIndex(8);
  TMD_CHECK(tabs.selectedIndex() == 8);

  // NavigationRail: old cap was 7 - add 10.
  NavigationRail<RGB565> rail(Bounds(0, 0, 80, 320));
  for (int i = 0; i < 10; ++i) rail.addDestination(nullptr, "D");
  rail.setSelectedIndex(9);
  TMD_CHECK(rail.selectedIndex() == 9);

  // Carousel: old cap was 8 - add 11.
  Carousel<RGB565> carousel(Bounds(0, 0, 240, 150));
  std::vector<MediaCard<RGB565>*> cards;
  for (int i = 0; i < 11; ++i) {
    auto* card = new MediaCard<RGB565>(Bounds(), "C");
    cards.push_back(card);
    carousel.addItem(*card);
  }
  TMD_CHECK(carousel.pageCount() == 11);
  carousel.setCurrentIndex(10, false);
  TMD_CHECK(carousel.currentIndex() == 10);

  for (auto* p : items) delete p;
  for (auto* p : cards) delete p;

  if (tmd_test_ok) printf("test_widget_caps: PASSED\n");
  return tmd_test_ok ? 0 : 1;
}
