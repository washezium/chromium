// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/bubble_controllers/post_save_compromised_bubble_controller.h"

#include "chrome/browser/ui/passwords/passwords_model_delegate_mock.h"
#include "chrome/grit/theme_resources.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class PostSaveCompromisedBubbleControllerTest : public ::testing::Test {
 public:
  PostSaveCompromisedBubbleControllerTest() {
    mock_delegate_ =
        std::make_unique<testing::NiceMock<PasswordsModelDelegateMock>>();
  }
  ~PostSaveCompromisedBubbleControllerTest() override = default;

  PasswordsModelDelegateMock* delegate() { return mock_delegate_.get(); }
  PostSaveCompromisedBubbleController* controller() {
    return controller_.get();
  }

  void CreateController(password_manager::ui::State state);

 private:
  std::unique_ptr<PasswordsModelDelegateMock> mock_delegate_;
  std::unique_ptr<PostSaveCompromisedBubbleController> controller_;
};

void PostSaveCompromisedBubbleControllerTest::CreateController(
    password_manager::ui::State state) {
  EXPECT_CALL(*delegate(), OnBubbleShown());
  EXPECT_CALL(*delegate(), GetState).WillOnce(testing::Return(state));
  controller_ = std::make_unique<PostSaveCompromisedBubbleController>(
      mock_delegate_->AsWeakPtr());
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(delegate()));
}

TEST_F(PostSaveCompromisedBubbleControllerTest, SafeState_Destroy) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_SAFE_STATE);

  EXPECT_CALL(*delegate(), OnBubbleHidden());
  controller()->OnBubbleClosing();
}

TEST_F(PostSaveCompromisedBubbleControllerTest, SafeState_DestroyImplicictly) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_SAFE_STATE);

  EXPECT_CALL(*delegate(), OnBubbleHidden());
}

TEST_F(PostSaveCompromisedBubbleControllerTest, SafeState_Content) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_SAFE_STATE);
  EXPECT_EQ(PostSaveCompromisedBubbleController::BubbleType::
                kPasswordUpdatedSafeState,
            controller()->type());
  EXPECT_NE(base::string16(), controller()->GetBody());
  EXPECT_EQ(base::string16(), controller()->GetButtonText());
  EXPECT_EQ(IDR_SAVED_PASSWORDS_SAFE_STATE_DARK,
            controller()->GetImageID(true));
  EXPECT_EQ(IDR_SAVED_PASSWORDS_SAFE_STATE, controller()->GetImageID(false));
}

TEST_F(PostSaveCompromisedBubbleControllerTest, MoreToFix_Destroy) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_MORE_TO_FIX);

  EXPECT_CALL(*delegate(), OnBubbleHidden());
  controller()->OnBubbleClosing();
}

TEST_F(PostSaveCompromisedBubbleControllerTest, MoreToFix_DestroyImplicictly) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_MORE_TO_FIX);

  EXPECT_CALL(*delegate(), OnBubbleHidden());
}

TEST_F(PostSaveCompromisedBubbleControllerTest, MoreToFix_Content) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_MORE_TO_FIX);
  EXPECT_EQ(PostSaveCompromisedBubbleController::BubbleType::
                kPasswordUpdatedWithMoreToFix,
            controller()->type());
  EXPECT_NE(base::string16(), controller()->GetBody());
  EXPECT_NE(base::string16(), controller()->GetButtonText());
  EXPECT_EQ(IDR_SAVED_PASSWORDS_NEUTRAL_STATE_DARK,
            controller()->GetImageID(true));
  EXPECT_EQ(IDR_SAVED_PASSWORDS_NEUTRAL_STATE, controller()->GetImageID(false));
}

TEST_F(PostSaveCompromisedBubbleControllerTest, MoreToFix_Click) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_MORE_TO_FIX);

  EXPECT_CALL(*delegate(), NavigateToPasswordCheckup());
  controller()->OnAccepted();
}

TEST_F(PostSaveCompromisedBubbleControllerTest, Unsafe_Destroy) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_UNSAFE_STATE);

  EXPECT_CALL(*delegate(), OnBubbleHidden());
  controller()->OnBubbleClosing();
}

TEST_F(PostSaveCompromisedBubbleControllerTest, Unsafe_DestroyImplicictly) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_UNSAFE_STATE);

  EXPECT_CALL(*delegate(), OnBubbleHidden());
}

TEST_F(PostSaveCompromisedBubbleControllerTest, Unsafe_Content) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_UNSAFE_STATE);
  EXPECT_EQ(PostSaveCompromisedBubbleController::BubbleType::kUnsafeState,
            controller()->type());
  EXPECT_NE(base::string16(), controller()->GetBody());
  EXPECT_NE(base::string16(), controller()->GetButtonText());
  EXPECT_EQ(IDR_SAVED_PASSWORDS_WARNING_STATE_DARK,
            controller()->GetImageID(true));
  EXPECT_EQ(IDR_SAVED_PASSWORDS_WARNING_STATE, controller()->GetImageID(false));
}

TEST_F(PostSaveCompromisedBubbleControllerTest, Unsafe_Click) {
  CreateController(password_manager::ui::PASSWORD_UPDATED_UNSAFE_STATE);

  EXPECT_CALL(*delegate(), NavigateToPasswordCheckup());
  controller()->OnAccepted();
}

}  // namespace
