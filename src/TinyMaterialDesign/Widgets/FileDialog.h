#pragma once

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
 * The dialog is a panel (panel()) to show with Screen::presentDialog(). It
 * lists the folders first, then the files whose extension is in the filter (all
 * files if the filter is empty). A row "..", shown below the root, goes to the
 * parent folder. Tapping a file calls onSelect(path); the application should
 * dismiss the dialog then. Tapping Cancel calls onCancel (if set) and the
 * dialog stays open unless the application dismisses it.
 *
 * @code
 *   FileDialog<RGB565> dialog;
 *   dialog.listDirectory = [](const std::string& dir) { ... SD entries ... };
 *   dialog.onSelect = [&](const std::string& path) { screen.dismissDialog(); open(path); };
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
  /// Called when Cancel is tapped. The Cancel row is only shown when set.
  std::function<void()> onCancel;

  /// The panel to show with Screen::presentDialog().
  Container<RGB_T>& panel() { return panel_; }

  /// Opens the dialog at directory dir. extensions is a comma separated list such as
  /// ".mobi,.azw" (empty: all files).
  void open(Bounds bounds, const char* title, const std::string& dir, const std::string& extensions = "") {
    bounds_ = bounds;
    title_ = title;
    extensions_ = extensions;
    cancelButton_ = Button<RGB_T>();
    navigate(dir.empty() ? "/" : dir);
  }

  /// The folder shown now.
  const std::string& directory() const { return dir_; }

 private:
  static constexpr int32_t kRowHeight = 36;
  static constexpr size_t kMaxRows = 200;

  /// Builds the panel for the folder dir.
  void navigate(const std::string& dir) {
    dir_ = dir;
    rows_.clear();
    panel_ = Container<RGB_T>(bounds_);
    background_ = Card<RGB_T>(bounds_);
    panel_.addChild(background_);

    titleLabel_ = Label<RGB_T>(Bounds(bounds_.x + 12, bounds_.y + 8, bounds_.w - 24, 28), title_.c_str(),
                               TypographyRole::kTitle);
    pathLabel_ = Label<RGB_T>(Bounds(bounds_.x + 12, bounds_.y + 38, bounds_.w - 24, 20), dir_.c_str(),
                              TypographyRole::kLabel);
    panel_.addChild(titleLabel_);
    panel_.addChild(pathLabel_);

    int32_t y = bounds_.y + 64;
    const int32_t bottom = bounds_.bottom() - (onCancel ? 52 : 12);
    if (dir_ != "/") {
      addRow(y, "..", [this]() { navigate(parentOf(dir_)); });
      y += kRowHeight;
    }
    std::vector<FileEntry> entries;
    if (listDirectory) entries = listDirectory(dir_);
    // folders first, then the files of the filter
    for (int pass = 0; pass < 2; ++pass) {
      for (const FileEntry& e : entries) {
        if (rows_.size() >= kMaxRows) break;
        if ((pass == 0) != e.isDirectory) continue;
        if (!e.isDirectory && !matches(e.name)) continue;
        const std::string name = e.name;
        if (e.isDirectory) {
          addRow(y, name + "/", [this, name]() { navigate(join(dir_, name)); });
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

    if (onCancel) {
      cancelButton_ = Button<RGB_T>(Bounds(bounds_.right() - 112, bottom + 8, 100, 36), "Cancel",
                                    ButtonVariant::kText);
      cancelButton_.onClick = [this]() {
        if (onCancel) onCancel();
      };
      panel_.addChild(cancelButton_);
    }
  }

  void addRow(int32_t y, const std::string& label, std::function<void()> onClick) {
    std::unique_ptr<ListItem<RGB_T>> row(new ListItem<RGB_T>(
        Bounds(bounds_.x + 8, y, bounds_.w - 16, kRowHeight), label.c_str()));
    row->enabled = static_cast<bool>(onClick);
    row->onClick = std::move(onClick);
    panel_.addChild(*row);
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

  static std::string parentOf(const std::string& dir) {
    const size_t slash = dir.find_last_of('/');
    if (slash == std::string::npos || slash == 0) return "/";
    return dir.substr(0, slash);
  }

  Bounds bounds_;
  std::string title_;
  std::string extensions_;
  std::string dir_;
  Container<RGB_T> panel_;
  Card<RGB_T> background_;
  Label<RGB_T> titleLabel_;
  Label<RGB_T> pathLabel_;
  Button<RGB_T> cancelButton_;
  std::vector<std::unique_ptr<ListItem<RGB_T>>> rows_;
};

}  // namespace tinymd
