// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_WIZARD_CONTEXT_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_WIZARD_CONTEXT_H_

#include "base/values.h"

namespace chromeos {

// Structure that defines data that need to be passed between screens during
// WizardController flows.
class WizardContext {
 public:
  WizardContext();
  ~WizardContext();

  WizardContext(const WizardContext&) = delete;
  WizardContext& operator=(const WizardContext&) = delete;

  // Configuration for automating OOBE screen actions, e.g. during device
  // version rollback.
  // Set by WizardController.
  // Used by multiple screens.
  base::Value configuration{base::Value::Type::DICTIONARY};

  // Indicates that enterprise enrollment was triggered early in the OOBE
  // process, so Update screen should be skipped and Enrollment start right
  // after EULA screen.
  // Set by Welcome, Network and EULA screens.
  // Used by Update screen and WizardController.
  bool enrollment_triggered_early = false;

  // Indicates that user selects to sign in or create a new account for a child.
  bool sign_in_as_child = false;

  // Whether the enrollment screen should be skipped when enrollment isn't
  // mandatory so that the normal gaia login is shown. Set by WizardController
  // SkipToLoginForTesting and checked on EnrollmentScreen::MaybeSkip
  bool skip_non_forced_enrollment_for_tests = false;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_WIZARD_CONTEXT_H_
