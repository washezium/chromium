// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/profile_picker_view.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/webui/signin/profile_picker_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/google_chrome_strings.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "chrome/browser/shell_integration_win.h"
#include "ui/base/win/shell.h"
#include "ui/views/win/hwnd_util.h"
#endif

namespace {

ProfilePickerView* g_profile_picker_view = nullptr;
constexpr int kWindowWidth = 1024;
constexpr int kWindowHeight = 758;
constexpr float kMaxRatioOfWorkArea = 0.9;

GURL CreateURLForPage(ProfilePicker::Page page) {
  GURL base_url = GURL(chrome::kChromeUIProfilePickerUrl);
  switch (page) {
    case ProfilePicker::Page::kManageProfiles:
      return base_url;
    case ProfilePicker::Page::kAddNewProfile:
      return base_url.Resolve("new-profile");
  }
}

}  // namespace

// static
void ProfilePicker::Show(Page page) {
  if (!g_profile_picker_view)
    g_profile_picker_view = new ProfilePickerView();

  g_profile_picker_view->Display(page);
}

// static
void ProfilePicker::Hide() {
  if (g_profile_picker_view)
    g_profile_picker_view->Clear();
}

ProfilePickerView::ProfilePickerView()
    : keep_alive_(KeepAliveOrigin::USER_MANAGER_VIEW,
                  KeepAliveRestartOption::DISABLED) {
  SetHasWindowSizeControls(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetTitle(IDS_PRODUCT_NAME);
  set_use_custom_frame(false);
  // TODO(crbug.com/1063856): Add |RecordDialogCreation|.
}

ProfilePickerView::~ProfilePickerView() = default;

void ProfilePickerView::Display(ProfilePicker::Page page) {
  if (initialized_ == kNotInitialized) {
    initialized_ = kInProgress;
    g_browser_process->profile_manager()->CreateProfileAsync(
        ProfileManager::GetSystemProfilePath(),
        base::BindRepeating(&ProfilePickerView::OnSystemProfileCreated,
                            weak_ptr_factory_.GetWeakPtr(), page),
        /*name=*/base::string16(), /*icon_url=*/std::string());
    return;
  }

  if (initialized_ == kInProgress)
    return;

  GetWidget()->Activate();
}

void ProfilePickerView::Clear() {
  if (initialized_ == kDone) {
    GetWidget()->Close();
    return;
  }

  WindowClosing();
  DeleteDelegate();
}

void ProfilePickerView::OnSystemProfileCreated(ProfilePicker::Page init_page,
                                               Profile* system_profile,
                                               Profile::CreateStatus status) {
  DCHECK_NE(status, Profile::CREATE_STATUS_LOCAL_FAIL);
  if (status != Profile::CREATE_STATUS_INITIALIZED)
    return;

  Init(init_page, system_profile);
}

void ProfilePickerView::Init(ProfilePicker::Page init_page,
                             Profile* system_profile) {
  web_view_ = new views::WebView(system_profile);
  web_view_->GetWebContents()->SetDelegate(this);
  AddChildView(web_view_);
  SetLayoutManager(std::make_unique<views::FillLayout>());

  CreateDialogWidget(this, nullptr, nullptr);

#if defined(OS_WIN)
  // Set the app id for the user manager to the app id of its parent.
  ui::win::SetAppIdForWindow(
      shell_integration::win::GetAppUserModelIdForBrowser(
          system_profile->GetPath()),
      views::HWNDForWidget(GetWidget()));
#endif

  web_view_->LoadInitialURL(CreateURLForPage(init_page));
  GetWidget()->Show();
  web_view_->RequestFocus();
  initialized_ = InitState::kDone;
}

gfx::Size ProfilePickerView::CalculatePreferredSize() const {
  gfx::Size preferred_size = gfx::Size(kWindowWidth, kWindowHeight);
  gfx::Size work_area_size = GetWidget()->GetWorkAreaBoundsInScreen().size();
  // Keep the window smaller then |work_area_size| so that it feels more like a
  // dialog then like the actual Chrome window.
  gfx::Size max_dialog_size = ScaleToFlooredSize(
      work_area_size, kMaxRatioOfWorkArea, kMaxRatioOfWorkArea);
  preferred_size.SetToMin(max_dialog_size);
  return preferred_size;
}

void ProfilePickerView::WindowClosing() {
  // Now that the window is closed, we can allow a new one to be opened.
  // (WindowClosing comes in asynchronously from the call to Close() and we
  // may have already opened a new instance).
  if (g_profile_picker_view == this)
    g_profile_picker_view = nullptr;
}

gfx::Size ProfilePickerView::GetMinimumSize() const {
  // On small screens, the preferred size may be smaller than the picker
  // minimum size. In that case there will be scrollbars on the picker.
  gfx::Size minimum_size = GetPreferredSize();
  minimum_size.SetToMin(ProfilePickerUI::GetMinimumSize());
  return minimum_size;
}

bool ProfilePickerView::HandleContextMenu(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params) {
  // Ignores context menu.
  return true;
}
