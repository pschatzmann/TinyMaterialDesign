#pragma once

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "TinyMaterialDesign/Core/Container.h"
#include "TinyMaterialDesign/Widgets/Button.h"
#include "TinyMaterialDesign/Widgets/Card.h"
#include "TinyMaterialDesign/Widgets/Label.h"
#include "TinyMaterialDesign/Widgets/ListItem.h"

namespace tinymd {

/// One entry of a directory: its name (without the path) and whether it is a folder.
struct FileEntry {
  std::string name;
  bool isDirectory = false;
};

/**
 * @brief Open file dialog: browse folders and choose a file.
 *
 * The dialog does not read the file system itself. The application supplies
 * listDirectory(), which returns the entries of a folder, so the same dialog
 * works with SD cards, flash file systems and the desktop.
 *
 * The dialog is a fixed panel (panel()) with a frame, a title, the current
 * folder and a Cancel button. The folders and files are listed in a scrolling
 * list inside the frame. Folders come first, then the files whose extension is
 * in the filter (all files if the filter is empty). A row ".." goes to the
 * parent folder. Tapping a file calls onSelect(path); the application should
 * then dismiss the dialog.
 *
 * @code
 *   FileDialog<RGB565> dialog;
 *   dialog.listDirectory = [](const std::string& dir) { ... SD entries ... };
 *   dialog.onSelect = [&](const std::string& path) { screen.dismissDialog(); open(path); };
 *   dialog.setTheme(theme);
 *   dialog.open(Bounds(8, 8, 224, 304), "Open book", "/", ".mobi,.azw,.azw3");
 *   screen.presentDialog(dialog.panel());
 * @endcode
 */
template <typename RGB_T = TINYMD_DEFAULT_RGB_T>
class FileDialog {
 public:
  /// Returns the entries of a folder (e.g. "/books").
  std::function<std::vector<FileEntry>(const std::string& dir)> listDirectory;
  /// Called with the full path of the chosen file.
  std::function<void(const std::string& path)> onSelect;
  /// Called when Cancel is tapped. The Cancel button is only shown when set.
  std::function<void()> onCancel;

  /// The panel to show with Screen::presentDialog().
  Container<RGB_T>& panel() { return panel_; }

  /// Sets the theme of the dialog (call before presenting it).
  void setTheme(const MaterialTheme<RGB_T>& theme) {
    theme_ = &theme;
    panel_.setTheme(theme);
  }

  /// Opens the dialog at directory dir. extensions is a comma separated list such as
  /// ".mobi,.azw" (empty: all files).
  void open(Bounds bounds, const char* title, const std::string& dir, const std::string& extensions = "") {
    bounds_ = bounds;
    title_ = title;
    extensions_ = extensions;
    build();
    navigate(dir.empty() ? "/" : dir);
  }

  /// The folder shown now.
  const std::string& directory() const { return dir_; }
  /// Shows a folder that was chosen by a tap. Call this from the application's
  /// loop (or update), and invalidate the screen when it returns true: the rows
  /// are rebuilt here, not inside the tap, because a row must not be destroyed
  /// while its own click handler is running.
  /// Returns true if the folder changed (the screen then needs a redraw).
  bool poll() {
    if (!hasPending_) return false;
    hasPending_ = false;
    navigate(pendingDir_);
    return true;
  }

 private:
  static constexpr int32_t kRowHeight = 36;
  static constexpr int32_t kHeaderHeight = 64;
  static constexpr int32_t kFooterHeight = 52;
  static constexpr size_t kMaxRows = 200;

  /// Changes the folder for the next poll().
  void requestFolder(const std::string& dir) {
    pendingDir_ = dir;
    hasPending_ = true;
  }

  /// The fixed part: frame, title, path and Cancel. The list is added inside it.
  void build() {
    panel_ = Container<RGB_T>(bounds_);
    if (theme_ != nullptr) panel_.setTheme(*theme_);
    background_ = Card<RGB_T>(bounds_);
    titleLabel_ = Label<RGB_T>(Bounds(bounds_.x + 12, bounds_.y + 8, bounds_.w - 24, 28), title_.c_str(),
                               TypographyRole::kTitle);
    pathLabel_ = Label<RGB_T>(Bounds(bounds_.x + 12, bounds_.y + 38, bounds_.w - 24, 20), "",
                              TypographyRole::kLabel);
    panel_.addChild(background_);
    panel_.addChild(titleLabel_);
    panel_.addChild(pathLabel_);
    panel_.addChild(list_);
    if (onCancel) {
      cancelButton_ = Button<RGB_T>(Bounds(bounds_.right() - 112, bounds_.bottom() - kFooterHeight + 8, 100, 36),
                                    "Cancel", ButtonVariant::kText);
      cancelButton_.onClick = [this]() {
        if (onCancel) onCancel();
      };
      panel_.addChild(cancelButton_);
    }
  }

  /// Rebuilds the list of the folder dir (the scrolling part).
  void navigate(const std::string& dir) {
    dir_ = dir;
    pathLabel_.setText(shortened(dir_).c_str());
    rows_.clear();
    const int32_t footer = onCancel ? kFooterHeight : 0;
    const Bounds listBounds(bounds_.x, bounds_.y + kHeaderHeight, bounds_.w, bounds_.h - kHeaderHeight - footer);
    list_ = Container<RGB_T>(listBounds);
    if (theme_ != nullptr) list_.setTheme(*theme_);

    int32_t y = listBounds.y + 4;
    if (dir_ != "/") {
      addRow(y, "..", [this]() { requestFolder(parentOf(dir_)); });
      y += kRowHeight;
    }
    std::vector<FileEntry> entries;
    if (listDirectory) entries = listDirectory(dir_);
    // hidden entries (names starting with '.') are not shown; the rest are sorted by name
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [](const FileEntry& e) { return !e.name.empty() && e.name[0] == '.'; }),
                  entries.end());
    std::sort(entries.begin(), entries.end(), [](const FileEntry& a, const FileEntry& b) {
      if (a.isDirectory != b.isDirectory) return a.isDirectory;
      return lowerCase(a.name) < lowerCase(b.name);
    });
    // folders first, then the files of the filter
    for (int pass = 0; pass < 2; ++pass) {
      for (const FileEntry& e : entries) {
        if (rows_.size() >= kMaxRows) break;
        if ((pass == 0) != e.isDirectory) continue;
        if (!e.isDirectory && !matches(e.name)) continue;
        const std::string name = e.name;
        if (e.isDirectory) {
          addRow(y, name + "/", [this, name]() { requestFolder(join(dir_, name)); });
        } else {
          addRow(y, name, [this, name]() {
            if (onSelect) onSelect(join(dir_, name));
          });
        }
        y += kRowHeight;
      }
    }
    if (rows_.empty()) {
      addRow(y, "No files", nullptr);
      rows_.back()->enabled = false;
    }
  }

  void addRow(int32_t y, const std::string& label, std::function<void()> onClick) {
    std::unique_ptr<ListItem<RGB_T>> row(
        new ListItem<RGB_T>(Bounds(bounds_.x + 8, y, bounds_.w - 24, kRowHeight), label.c_str()));
    row->enabled = static_cast<bool>(onClick);
    row->onClick = std::move(onClick);
    list_.addChild(*row);
    rows_.push_back(std::move(row));
  }

  bool matches(const std::string& name) const {
    if (extensions_.empty()) return true;
    size_t start = 0;
    while (start <= extensions_.size()) {
      size_t comma = extensions_.find(',', start);
      if (comma == std::string::npos) comma = extensions_.size();
      const std::string ext = extensions_.substr(start, comma - start);
      if (name.size() >= ext.size() && name.compare(name.size() - ext.size(), ext.size(), ext) == 0) return true;
      start = comma + 1;
    }
    return false;
  }

  static std::string join(const std::string& dir, const std::string& name) {
    if (dir.empty() || dir.back() == '/') return dir + name;
    return dir + "/" + name;
  }

  static std::string lowerCase(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
  }

  /// The end of a long path, so that the current folder stays visible.
  static std::string shortened(const std::string& path) {
    constexpr size_t kMaxChars = 28;
    if (path.size() <= kMaxChars) return path;
    return "..." + path.substr(path.size() - (kMaxChars - 3));
  }

  static std::string parentOf(const std::string& dir) {
    const size_t slash = dir.find_last_of('/');
    if (slash == std::string::npos || slash == 0) return "/";
    return dir.substr(0, slash);
  }

  Bounds bounds_;
  std::string title_;
  std::string extensions_;
  std::string dir_;
  const MaterialTheme<RGB_T>* theme_ = nullptr;
  std::string pendingDir_;
  bool hasPending_ = false;
  Container<RGB_T> panel_;
  Card<RGB_T> background_;
  Label<RGB_T> titleLabel_;
  Label<RGB_T> pathLabel_;
  Container<RGB_T> list_;
  Button<RGB_T> cancelButton_;
  std::vector<std::unique_ptr<ListItem<RGB_T>>> rows_;
};

}  // namespace tinymd
