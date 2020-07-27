// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/chromeos/login/screens/family_link_notice_screen.h"

#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/test/fake_gaia_mixin.h"
#include "chrome/browser/chromeos/login/test/js_checker.h"
#include "chrome/browser/chromeos/login/test/local_policy_test_server_mixin.h"
#include "chrome/browser/chromeos/login/test/login_manager_mixin.h"
#include "chrome/browser/chromeos/login/test/oobe_base_test.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_exit_waiter.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/test/user_policy_mixin.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/ui/webui/chromeos/login/family_link_notice_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/gaia_screen_handler.h"
#include "chromeos/constants/chromeos_features.h"
#include "chromeos/login/auth/stub_authenticator_builder.h"
#include "content/public/test/browser_test.h"

namespace chromeos {

namespace {

const test::UIPath kFamilyLinkDialog = {"family-link-notice",
                                        "familyLinkDialog"};
const test::UIPath kContinueButton = {"family-link-notice", "continueButton"};

}  // namespace

class FamilyLinkNoticeScreenTest : public OobeBaseTest {
 public:
  FamilyLinkNoticeScreenTest() {
    feature_list_.InitAndEnableFeature(
        chromeos::features::kChildSpecificSignin);
  }
  ~FamilyLinkNoticeScreenTest() override = default;

  void SetUpOnMainThread() override {
    FamilyLinkNoticeScreen* screen = static_cast<FamilyLinkNoticeScreen*>(
        WizardController::default_controller()->screen_manager()->GetScreen(
            FamilyLinkNoticeView::kScreenId));
    original_callback_ = screen->get_exit_callback_for_testing();
    screen->set_exit_callback_for_testing(base::BindRepeating(
        &FamilyLinkNoticeScreenTest::HandleScreenExit, base::Unretained(this)));
    OobeBaseTest::SetUpOnMainThread();
  }

  void SetUpInProcessBrowserTestFixture() override {
    // Child users require a user policy, set up an empty one so the user can
    // get through login.
    ASSERT_TRUE(user_policy_mixin_.RequestPolicyUpdate());
    OobeBaseTest::SetUpInProcessBrowserTestFixture();
  }

  void LoginAsRegularUser() {
    login_manager_mixin_.LoginAsNewRegularUser();
    OobeScreenExitWaiter(GaiaView::kScreenId).Wait();
  }

  void LoginAsChildUser() {
    login_manager_mixin_.LoginAsNewChildUser();
    OobeScreenExitWaiter(GaiaView::kScreenId).Wait();
  }

  void WaitForScreenExit() {
    if (screen_result_.has_value())
      return;
    base::RunLoop run_loop;
    screen_exit_callback_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  base::Optional<FamilyLinkNoticeScreen::Result> screen_result_;

 private:
  void HandleScreenExit(FamilyLinkNoticeScreen::Result result) {
    ASSERT_FALSE(screen_exited_);
    screen_exited_ = true;
    screen_result_ = result;
    original_callback_.Run(result);
    if (screen_exit_callback_)
      std::move(screen_exit_callback_).Run();
  }

  bool screen_exited_ = false;
  base::RepeatingClosure screen_exit_callback_;
  FamilyLinkNoticeScreen::ScreenExitCallback original_callback_;

  FakeGaiaMixin fake_gaia_{&mixin_host_, embedded_test_server()};
  LoginManagerMixin login_manager_mixin_{&mixin_host_, {}, &fake_gaia_};
  LocalPolicyTestServerMixin policy_server_mixin_{&mixin_host_};
  UserPolicyMixin user_policy_mixin_{
      &mixin_host_,
      AccountId::FromUserEmailGaiaId(test::kTestEmail, test::kTestGaiaId),
      &policy_server_mixin_};

  base::test::ScopedFeatureList feature_list_;
};

// Verify that regular account user should not see family link notice screen
// after log in.
IN_PROC_BROWSER_TEST_F(FamilyLinkNoticeScreenTest, RegularAccount) {
  WizardController::default_controller()
      ->get_wizard_context_for_testing()
      ->sign_in_as_child = false;
  LoginAsRegularUser();
  WaitForScreenExit();
  EXPECT_EQ(screen_result_.value(), FamilyLinkNoticeScreen::Result::SKIPPED);
}

// Verify user should see family link notice screen when selecting to sign in
// as a child account but log in as a regular account.
IN_PROC_BROWSER_TEST_F(FamilyLinkNoticeScreenTest, NonSupervisedChildAccount) {
  WizardController::default_controller()
      ->get_wizard_context_for_testing()
      ->sign_in_as_child = true;
  LoginAsRegularUser();
  OobeScreenWaiter(FamilyLinkNoticeView::kScreenId).Wait();
  test::OobeJS().ExpectVisiblePath(kFamilyLinkDialog);
  test::OobeJS().ExpectVisiblePath(kContinueButton);
  test::OobeJS().TapOnPath(kContinueButton);
  WaitForScreenExit();
  EXPECT_EQ(screen_result_.value(), FamilyLinkNoticeScreen::Result::DONE);
}

// Verify child account user should not see family link notice screen after log
// in.
IN_PROC_BROWSER_TEST_F(FamilyLinkNoticeScreenTest, ChildAccount) {
  WizardController::default_controller()
      ->get_wizard_context_for_testing()
      ->sign_in_as_child = true;
  LoginAsChildUser();
  WaitForScreenExit();
  EXPECT_EQ(screen_result_.value(), FamilyLinkNoticeScreen::Result::SKIPPED);
}

// Verify child account user should not see family link notice screen after log
// in if not selecting sign in as child.
IN_PROC_BROWSER_TEST_F(FamilyLinkNoticeScreenTest,
                       ChildAccountSignInAsRegular) {
  WizardController::default_controller()
      ->get_wizard_context_for_testing()
      ->sign_in_as_child = false;
  LoginAsChildUser();
  WaitForScreenExit();
  EXPECT_EQ(screen_result_.value(), FamilyLinkNoticeScreen::Result::SKIPPED);
}

}  // namespace chromeos
