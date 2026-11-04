#pragma once
#include <functional>
#include <string>
#include <vector>

#include "TinyMaterialDesign/Core/Container.h"
#include "TinyMaterialDesign/Core/Screen.h"
#include "TinyMaterialDesign/Widgets/IconButton.h"
#include "TinyMaterialDesign/Widgets/NavigationBar.h"

namespace tinymd {

/**
 * @brief One page (tab) of an app: a scrollable Container with a title and
 * an icon for the navigation, that builds its own widgets.
 *
 * Derive from it and add the widgets in build(); PageNavigator calls it once
 * the page is on the screen (so it already has its theme and bounds):
 *
 *   class ClockPage : public Page<RGB565> {
 *    public:
 *     ClockPage() : Page("Clock", drawClockIcon) {}
 *     void build() override { addChild(time_); ... }
 *     bool tick(uint32_t nowMs) override { ... return true; }  // redraw
 *   };
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class Page : public Container<RGB_T> {
 public:
  explicit Page(const char* title = "", IconPainter<RGB_T> icon = nullptr) : title_(title), icon_(icon) {}

  /// Label and icon of the page in a NavigationBar.
  virtual const char* title() const { return title_.c_str(); }
  virtual IconPainter<RGB_T> icon() const { return icon_; }
  void setTitle(const char* title) { title_ = title; }
  void setIcon(IconPainter<RGB_T> icon) { icon_ = icon; }

  /// Creates the widgets (called once by PageNavigator::begin()).
  virtual void build() {}
  /// The page becomes visible.
  virtual void onShow() {}
  /// The page is hidden (another page is shown).
  virtual void onHide() {}
  /// Called regularly while the page is shown (PageNavigator::tick()):
  /// return true if the page changed and must be redrawn.
  virtual bool tick(uint32_t nowMs) {
    (void)nowMs;
    return false;
  }

 private:
  std::string title_;
  IconPainter<RGB_T> icon_ = nullptr;
};

/**
 * @brief Connects Pages with a NavigationBar: one destination per page,
 * only the selected page is visible. All pages share the same bounds, so the
 * screen itself never scrolls; each page scrolls on its own.
 *
 *   NavigationBar<RGB565> nav(Bounds(0, 264, 240, 56));
 *   PageNavigator<RGB565> pages(app.screen(), nav);
 *   pages.addPage(clockPage);
 *   pages.addPage(settingsPage);
 *   pages.begin(Bounds(0, 40, 240, 224));  // adds nav + pages, builds them
 *   // loop(): if (pages.tick(millis())) app.screen().invalidate();
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class PageNavigator {
 public:
  PageNavigator(Screen<RGB_T>& screen, NavigationBar<RGB_T>& nav) : screen_(screen), nav_(nav) {}

  /// Called after the page was changed (by the user or show()).
  std::function<void(int index)> onPageChanged;

  /// Adds a page (before begin()).
  void addPage(Page<RGB_T>& page) { pages_.push_back(&page); }

  /**
   * @brief Adds the navigation bar (fixed) and the pages (with the indicated
   * bounds) to the screen, builds the pages and shows the first one.
   */
  void begin(Bounds pageBounds) {
    for (Page<RGB_T>* page : pages_) nav_.addDestination(page->icon(), page->title());
    nav_.onChange = [this](int index) { show(index); };
    screen_.addFixedWidget(nav_);
    for (Page<RGB_T>* page : pages_) {
      page->bounds = pageBounds;
      screen_.addWidget(*page);  // gives it the theme
      page->build();
    }
    begun_ = true;
    current_ = -1;
    show(0);
  }

  /// Shows the page with the index.
  void show(int index) {
    if (index < 0 || index >= count()) return;
    if (begun_ && current_ >= 0 && current_ != index) pages_[current_]->onHide();
    const bool changed = current_ != index;
    current_ = index;
    for (size_t i = 0; i < pages_.size(); ++i) pages_[i]->visible = static_cast<int>(i) == index;
    nav_.setSelectedIndex(index);
    pages_[current_]->onShow();
    screen_.invalidate();
    if (changed && onPageChanged) onPageChanged(index);
  }

  /// Calls tick() of the visible page: true if it must be redrawn.
  bool tick(uint32_t nowMs) { return current_ >= 0 && pages_[current_]->tick(nowMs); }

  int current() const { return current_; }
  int count() const { return static_cast<int>(pages_.size()); }
  Page<RGB_T>& page(int index) { return *pages_[index]; }
  Page<RGB_T>& currentPage() { return *pages_[current_]; }

 private:
  Screen<RGB_T>& screen_;
  NavigationBar<RGB_T>& nav_;
  std::vector<Page<RGB_T>*> pages_;
  int current_ = -1;
  bool begun_ = false;
};

}  // namespace tinymd
