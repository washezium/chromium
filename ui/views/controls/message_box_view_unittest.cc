// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/message_box_view.h"

#include <algorithm>
#include <memory>

#include "base/bind_helpers.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/test/views_test_base.h"

namespace views {

namespace {
// The default mesage width same as defined in message_box_view.cc.
constexpr int kDefaultMessageWidth = 400;
const base::string16 kDefaultMessage =
    base::ASCIIToUTF16("This is a test message for MessageBoxView.");
}  // namespace

class MessageBoxViewTest : public ViewsTestBase {
 public:
  MessageBoxViewTest() = default;
  MessageBoxViewTest(const MessageBoxViewTest&) = delete;
  MessageBoxViewTest& operator=(const MessageBoxViewTest&) = delete;
  ~MessageBoxViewTest() override = default;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    message_box_ = std::make_unique<MessageBoxView>(kDefaultMessage);
    provider_ = LayoutProvider::Get();
  }

  std::unique_ptr<MessageBoxView> message_box_;
  const LayoutProvider* provider_;
};

TEST_F(MessageBoxViewTest, CheckMessageOnlySize) {
  message_box_->SizeToPreferredSize();

  gfx::Insets box_border =
      provider_->GetDialogInsetsForContentType(views::TEXT, views::TEXT);
  gfx::Size scroll_size = message_box_->scroll_view_->size();
  scroll_size.Enlarge(0, box_border.top() + box_border.bottom());
  EXPECT_EQ(scroll_size, message_box_->size());
}

TEST_F(MessageBoxViewTest, CheckWithOptionalViewsSize) {
  message_box_->SetPromptField(base::string16());
  message_box_->SizeToPreferredSize();

  gfx::Insets box_border =
      provider_->GetDialogInsetsForContentType(views::TEXT, views::CONTROL);
  gfx::Size scroll_size = message_box_->scroll_view_->size();
  gfx::Size prompt_size = message_box_->prompt_field_->size();
  gfx::Size content_size(std::max(scroll_size.width(), prompt_size.width()),
                         scroll_size.height() + prompt_size.height());
  content_size.Enlarge(0, box_border.top() + box_border.bottom() +
                              message_box_->inter_row_vertical_spacing_);
  EXPECT_EQ(content_size, message_box_->size());

  // Add a checkbox and a link.
  message_box_->SetCheckBoxLabel(base::ASCIIToUTF16("A checkbox"));
  message_box_->SetLink(base::ASCIIToUTF16("Link to display"),
                        base::DoNothing());
  message_box_->SizeToPreferredSize();

  box_border =
      provider_->GetDialogInsetsForContentType(views::TEXT, views::TEXT);
  gfx::Size checkbox_size = message_box_->checkbox_->size();
  gfx::Size link_size = message_box_->link_->size();
  content_size =
      gfx::Size(std::max(std::max(scroll_size.width(), prompt_size.width()),
                         std::max(checkbox_size.width(), link_size.width())),
                scroll_size.height() + prompt_size.height() +
                    checkbox_size.height() + link_size.height());
  content_size.Enlarge(0, box_border.top() + box_border.bottom() +
                              3 * message_box_->inter_row_vertical_spacing_);
  EXPECT_EQ(content_size, message_box_->size());
}

TEST_F(MessageBoxViewTest, CheckMessageWidthChange) {
  message_box_->SizeToPreferredSize();
  EXPECT_EQ(kDefaultMessageWidth, message_box_->width());

  static constexpr int kNewWidth = 210;
  message_box_->SetMessageWidth(kNewWidth);
  message_box_->SizeToPreferredSize();
  EXPECT_EQ(kNewWidth, message_box_->width());
}

TEST_F(MessageBoxViewTest, CheckInterRowHeightChange) {
  message_box_->SetPromptField(base::string16());
  message_box_->SizeToPreferredSize();

  int scroll_height = message_box_->scroll_view_->height();
  int prompt_height = message_box_->prompt_field_->height();
  gfx::Insets box_border =
      provider_->GetDialogInsetsForContentType(views::TEXT, views::CONTROL);
  int inter_row_spacing = message_box_->inter_row_vertical_spacing_;
  EXPECT_EQ(
      scroll_height + inter_row_spacing + prompt_height + box_border.height(),
      message_box_->height());

  static constexpr int kNewInterRowSpacing = 50;
  EXPECT_NE(kNewInterRowSpacing, inter_row_spacing);
  message_box_->SetInterRowVerticalSpacing(kNewInterRowSpacing);
  message_box_->SizeToPreferredSize();
  EXPECT_EQ(kNewInterRowSpacing, message_box_->inter_row_vertical_spacing_);
  EXPECT_EQ(
      scroll_height + kNewInterRowSpacing + prompt_height + box_border.height(),
      message_box_->height());
}

}  // namespace views
