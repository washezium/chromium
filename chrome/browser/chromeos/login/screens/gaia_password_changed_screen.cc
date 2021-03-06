// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/gaia_password_changed_screen.h"

#include "chrome/browser/chromeos/login/reauth_stats.h"
#include "chrome/browser/chromeos/login/screen_manager.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/ui/webui/chromeos/login/gaia_password_changed_screen_handler.h"

namespace chromeos {

namespace {
constexpr const char kUserActionCancelLogin[] = "cancel";
constexpr const char kUserActionResyncData[] = "resync";

}  // namespace

// static
GaiaPasswordChangedScreen* GaiaPasswordChangedScreen::Get(
    ScreenManager* manager) {
  return static_cast<GaiaPasswordChangedScreen*>(
      manager->GetScreen(GaiaPasswordChangedView::kScreenId));
}

GaiaPasswordChangedScreen::GaiaPasswordChangedScreen(
    GaiaPasswordChangedView* view)
    : BaseScreen(GaiaPasswordChangedView::kScreenId,
                 OobeScreenPriority::DEFAULT),
      view_(view) {
  if (view_)
    view_->Bind(this);
}

GaiaPasswordChangedScreen::~GaiaPasswordChangedScreen() {
  if (view_)
    view_->Unbind();
}

void GaiaPasswordChangedScreen::OnViewDestroyed(GaiaPasswordChangedView* view) {
  if (view_ == view)
    view_ = nullptr;
}

void GaiaPasswordChangedScreen::ShowImpl() {
  DCHECK(account_id_.is_valid());
  if (view_)
    view_->Show(account_id_.GetUserEmail(), show_error_);
}

void GaiaPasswordChangedScreen::HideImpl() {
  account_id_.clear();
  show_error_ = false;
}

void GaiaPasswordChangedScreen::Configure(const AccountId& account_id,
                                          bool after_incorrect_attempt) {
  DCHECK(account_id.is_valid());
  account_id_ = account_id;
  show_error_ = after_incorrect_attempt;
}

void GaiaPasswordChangedScreen::OnUserAction(const std::string& action_id) {
  if (action_id == kUserActionCancelLogin) {
    CancelPasswordChangedFlow();
  } else if (action_id == kUserActionResyncData) {
    // LDH will pass control to ExistingUserController to proceed with clearing
    // cryptohome.
    if (LoginDisplayHost::default_host())
      LoginDisplayHost::default_host()->ResyncUserData();
  }
}

void GaiaPasswordChangedScreen::MigrateUserData(
    const std::string& old_password) {
  // LDH will pass control to ExistingUserController to proceed with updating
  // cryptohome keys.
  if (LoginDisplayHost::default_host())
    LoginDisplayHost::default_host()->MigrateUserData(old_password);
}

void GaiaPasswordChangedScreen::CancelPasswordChangedFlow() {
  if (account_id_.is_valid()) {
    RecordReauthReason(account_id_, ReauthReason::PASSWORD_UPDATE_SKIPPED);
  }
  ProfileHelper* profile_helper = ProfileHelper::Get();
  profile_helper->ClearSigninProfile(
      base::Bind(&GaiaPasswordChangedScreen::OnCookiesCleared,
                 weak_factory_.GetWeakPtr()));
}

void GaiaPasswordChangedScreen::OnCookiesCleared() {
  LoginDisplayHost::default_host()->StartSignInScreen();
}

}  // namespace chromeos
