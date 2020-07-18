// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/crosapi/select_file_crosapi.h"

#include <utility>
#include <vector>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/desks/desks_util.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/numerics/ranges.h"
#include "chrome/browser/ui/views/select_file_dialog_extension.h"
#include "chromeos/crosapi/mojom/select_file.mojom.h"
#include "components/exo/shell_surface_util.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/selected_file_info.h"
#include "ui/wm/public/activation_client.h"
#include "url/gurl.h"

namespace {

// TODO(https://crbug.com/1090587): Replace with window ID from Wayland client.
int g_next_window_id = 0;

ui::SelectFileDialog::Type GetUiType(
    crosapi::mojom::SelectFileDialogType type) {
  switch (type) {
    case crosapi::mojom::SelectFileDialogType::kFolder:
      return ui::SelectFileDialog::Type::SELECT_FOLDER;
    case crosapi::mojom::SelectFileDialogType::kUploadFolder:
      return ui::SelectFileDialog::Type::SELECT_UPLOAD_FOLDER;
    case crosapi::mojom::SelectFileDialogType::kExistingFolder:
      return ui::SelectFileDialog::Type::SELECT_EXISTING_FOLDER;
    case crosapi::mojom::SelectFileDialogType::kOpenFile:
      return ui::SelectFileDialog::Type::SELECT_OPEN_FILE;
    case crosapi::mojom::SelectFileDialogType::kOpenMultiFile:
      return ui::SelectFileDialog::Type::SELECT_OPEN_MULTI_FILE;
    case crosapi::mojom::SelectFileDialogType::kSaveAsFile:
      return ui::SelectFileDialog::Type::SELECT_SAVEAS_FILE;
  }
}

ui::SelectFileDialog::FileTypeInfo::AllowedPaths GetUiAllowedPaths(
    crosapi::mojom::AllowedPaths allowed_paths) {
  switch (allowed_paths) {
    case crosapi::mojom::AllowedPaths::kAnyPath:
      return ui::SelectFileDialog::FileTypeInfo::ANY_PATH;
    case crosapi::mojom::AllowedPaths::kNativePath:
      return ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;
    case crosapi::mojom::AllowedPaths::kAnyPathOrUrl:
      return ui::SelectFileDialog::FileTypeInfo::ANY_PATH_OR_URL;
  }
}

// TODO(https://crbug.com/1090587): Parent to the ShellSurface that spawned
// the dialog. For now, parent to the active window, which in practice should be
// the spawning window.
aura::Window* GetOwnerWindow() {
  aura::Window* root = ash::Shell::GetRootWindowForNewWindows();
  aura::Window* active = ::wm::GetActivationClient(root)->GetActiveWindow();
  // Check that the active window is still a ShellSurface window.
  if (active && exo::GetShellSurfaceBaseForWindow(active))
    return active;
  // Fallback to the active virtual desk.
  return ash::Shell::GetContainer(root,
                                  ash::desks_util::GetActiveDeskContainerId());
}

// Manages a single open/save dialog. There may be multiple dialogs showing at
// the same time. Deletes itself when the dialog is closed.
class SelectFileDialogHolder : public ui::SelectFileDialog::Listener {
 public:
  SelectFileDialogHolder(crosapi::mojom::SelectFileOptionsPtr options,
                         crosapi::mojom::SelectFile::SelectCallback callback)
      : select_callback_(std::move(callback)) {
    // Policy is null because showing the file-dialog-blocked infobar is handled
    // client-side in lacros-chrome.
    select_file_dialog_ =
        SelectFileDialogExtension::Create(this, /*policy=*/nullptr);

    SelectFileDialogExtension::Owner owner;
    owner.window = GetOwnerWindow();
    // TODO(https://crbug.com/1090587): Replace with ID from Wayland client.
    owner.lacros_window_id = g_next_window_id++;

    int file_type_index = 0;
    if (options->file_types) {
      file_types_ = std::make_unique<ui::SelectFileDialog::FileTypeInfo>();
      file_types_->extensions = options->file_types->extensions;
      // Only apply description overrides if the right number are provided.
      if (options->file_types->extensions.size() ==
          options->file_types->extension_description_overrides.size()) {
        file_types_->extension_description_overrides =
            options->file_types->extension_description_overrides;
      }
      // Index is 1-based (hence range 1 to size()), but 0 is allowed because it
      // means "no selection". See ui::SelectFileDialog::SelectFile().
      file_type_index =
          base::ClampToRange(options->file_types->default_file_type_index, 0,
                             int{file_types_->extensions.size()});
      file_types_->include_all_files = options->file_types->include_all_files;
      file_types_->allowed_paths =
          GetUiAllowedPaths(options->file_types->allowed_paths);
    }
    // |default_extension| is unused on Chrome OS.
    select_file_dialog_->SelectFileWithFileManagerParams(
        GetUiType(options->type), options->title, options->default_path,
        file_types_.get(), file_type_index,
        /*params=*/nullptr, owner,
        /*show_android_picker_apps=*/false);
  }

  SelectFileDialogHolder(const SelectFileDialogHolder&) = delete;
  SelectFileDialogHolder& operator=(const SelectFileDialogHolder&) = delete;
  ~SelectFileDialogHolder() override = default;

 private:
  // ui::SelectFileDialog::Listener:
  void FileSelected(const base::FilePath& path,
                    int file_type_index,
                    void* params) override {
    FileSelectedWithExtraInfo(ui::SelectedFileInfo(path, path), file_type_index,
                              params);
  }

  void FileSelectedWithExtraInfo(const ui::SelectedFileInfo& file,
                                 int file_type_index,
                                 void* params) override {
    OnSelected({file}, file_type_index);
  }

  void MultiFilesSelected(const std::vector<base::FilePath>& files,
                          void* params) override {
    MultiFilesSelectedWithExtraInfo(
        ui::FilePathListToSelectedFileInfoList(files), params);
  }

  void MultiFilesSelectedWithExtraInfo(
      const std::vector<ui::SelectedFileInfo>& files,
      void* params) override {
    OnSelected(files, /*file_type_index=*/0);
  }

  void FileSelectionCanceled(void* params) override {
    // Cancel is the same as selecting nothing.
    OnSelected({}, /*file_type_index=*/0);
  }

  // Invokes |select_callback_| with the list of files and deletes this object.
  void OnSelected(const std::vector<ui::SelectedFileInfo>& files,
                  int file_type_index) {
    std::vector<crosapi::mojom::SelectedFileInfoPtr> mojo_files;
    for (const ui::SelectedFileInfo& file : files) {
      crosapi::mojom::SelectedFileInfoPtr mojo_file =
          crosapi::mojom::SelectedFileInfo::New();
      mojo_file->file_path = file.file_path;
      mojo_file->local_path = file.local_path;
      mojo_file->display_name = file.display_name;
      mojo_file->url = file.url;
      mojo_files.push_back(std::move(mojo_file));
    }
    std::move(select_callback_)
        .Run(crosapi::mojom::SelectFileResult::kSuccess, std::move(mojo_files),
             file_type_index);
    delete this;
  }

  // Callback run after files are selected or the dialog is canceled.
  crosapi::mojom::SelectFile::SelectCallback select_callback_;

  // The file select dialog.
  scoped_refptr<SelectFileDialogExtension> select_file_dialog_;

  // Optional file type extension filters.
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> file_types_;
};

}  // namespace

// TODO(https://crbug.com/1090587): Connection error handling.
SelectFileCrosapi::SelectFileCrosapi(
    mojo::PendingReceiver<crosapi::mojom::SelectFile> receiver)
    : receiver_(this, std::move(receiver)) {}

SelectFileCrosapi::~SelectFileCrosapi() = default;

void SelectFileCrosapi::Select(crosapi::mojom::SelectFileOptionsPtr options,
                               SelectCallback callback) {
  // Deletes itself when the dialog closes.
  new SelectFileDialogHolder(std::move(options), std::move(callback));
}
