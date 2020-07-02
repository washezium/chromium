// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/bubble_controllers/post_save_compromised_bubble_controller.h"

#include "base/metrics/histogram_functions.h"
#include "chrome/browser/ui/passwords/passwords_model_delegate.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/password_manager/core/common/password_manager_ui.h"
#include "ui/base/l10n/l10n_util.h"

PostSaveCompromisedBubbleController::PostSaveCompromisedBubbleController(
    base::WeakPtr<PasswordsModelDelegate> delegate)
    : PasswordBubbleControllerBase(
          std::move(delegate),
          password_manager::metrics_util::
              AUTOMATIC_COMPROMISED_CREDENTIALS_REMINDER) {
  switch (delegate_->GetState()) {
    case password_manager::ui::PASSWORD_UPDATED_SAFE_STATE:
      type_ = BubbleType::kPasswordUpdatedSafeState;
      break;
    case password_manager::ui::PASSWORD_UPDATED_MORE_TO_FIX:
      type_ = BubbleType::kPasswordUpdatedWithMoreToFix;
      break;
    case password_manager::ui::PASSWORD_UPDATED_UNSAFE_STATE:
      type_ = BubbleType::kUnsafeState;
      break;
    default:
      NOTREACHED();
  }
  base::UmaHistogramEnumeration("PasswordBubble.CompromisedBubbleType", type_);
}

PostSaveCompromisedBubbleController::~PostSaveCompromisedBubbleController() {
  // Make sure the interactions are reported even if Views didn't notify the
  // controller about the bubble being closed.
  if (!interaction_reported_)
    OnBubbleClosing();
}

base::string16 PostSaveCompromisedBubbleController::GetBody() const {
  switch (type_) {
    case BubbleType::kPasswordUpdatedSafeState:
      return l10n_util::GetStringUTF16(
          IDS_PASSWORD_MANAGER_SAFE_STATE_BODY_MESSAGE);
    case BubbleType::kPasswordUpdatedWithMoreToFix:
      return l10n_util::GetPluralStringFUTF16(
          IDS_PASSWORD_MANAGER_MORE_TO_FIX_BODY_MESSAGE,
          delegate_->GetTotalNumberCompromisedPasswords());
    case BubbleType::kUnsafeState:
      return l10n_util::GetPluralStringFUTF16(
          IDS_PASSWORD_MANAGER_UNSAFE_STATE_BODY_MESSAGE,
          delegate_->GetTotalNumberCompromisedPasswords());
  }
}

base::string16 PostSaveCompromisedBubbleController::GetButtonText() const {
  switch (type_) {
    case BubbleType::kPasswordUpdatedSafeState:
      return base::string16();
    case BubbleType::kPasswordUpdatedWithMoreToFix:
      return l10n_util::GetStringUTF16(
          IDS_PASSWORD_MANAGER_CHECK_REMAINING_BUTTON);
    case BubbleType::kUnsafeState:
      return l10n_util::GetStringUTF16(IDS_PASSWORD_MANAGER_CHECK_BUTTON);
  }
}

int PostSaveCompromisedBubbleController::GetImageID(bool dark) const {
  switch (type_) {
    case BubbleType::kPasswordUpdatedSafeState:
      return dark ? IDR_SAVED_PASSWORDS_SAFE_STATE_DARK
                  : IDR_SAVED_PASSWORDS_SAFE_STATE;
    case BubbleType::kPasswordUpdatedWithMoreToFix:
      return dark ? IDR_SAVED_PASSWORDS_NEUTRAL_STATE_DARK
                  : IDR_SAVED_PASSWORDS_NEUTRAL_STATE;
    case BubbleType::kUnsafeState:
      return dark ? IDR_SAVED_PASSWORDS_WARNING_STATE_DARK
                  : IDR_SAVED_PASSWORDS_WARNING_STATE;
  }
}

void PostSaveCompromisedBubbleController::OnAccepted() {
  checked_clicked_ = true;
  if (delegate_)
    delegate_->NavigateToPasswordCheckup();
}

base::string16 PostSaveCompromisedBubbleController::GetTitle() const {
  switch (type_) {
    case BubbleType::kPasswordUpdatedSafeState:
    case BubbleType::kPasswordUpdatedWithMoreToFix:
      return l10n_util::GetStringUTF16(
          IDS_PASSWORD_MANAGER_UPDATED_BUBBLE_TITLE);
    case BubbleType::kUnsafeState:
      return l10n_util::GetPluralStringFUTF16(
          IDS_PASSWORD_MANAGER_COMPROMISED_REMINDER_TITLE,
          delegate_->GetTotalNumberCompromisedPasswords());
  }
}

void PostSaveCompromisedBubbleController::ReportInteractions() {
  base::UmaHistogramBoolean("PasswordBubble.CompromisedBubbleCheckClicked",
                            checked_clicked_);
}
