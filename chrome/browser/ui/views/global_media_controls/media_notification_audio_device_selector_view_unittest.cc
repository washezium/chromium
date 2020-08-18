// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/global_media_controls/media_notification_audio_device_selector_view.h"
#include <memory>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/test/mock_media_router.h"
#include "chrome/browser/ui/global_media_controls/media_notification_device_provider.h"
#include "chrome/browser/ui/global_media_controls/media_notification_service.h"
#include "chrome/browser/ui/views/global_media_controls/media_notification_audio_device_selector_view_delegate.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "media/audio/audio_device_description.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"

class MediaNotificationContainerObserver;

namespace {

class MockMediaNotificationDeviceProvider
    : public MediaNotificationDeviceProvider {
 public:
  MockMediaNotificationDeviceProvider() = default;
  ~MockMediaNotificationDeviceProvider() override = default;

  void AddDevice(const std::string& device_name, const std::string& device_id) {
    device_descriptions.emplace_back(device_name, device_id, "");
  }
  std::unique_ptr<MediaNotificationDeviceProvider::
                      GetOutputDevicesCallbackList::Subscription>
  GetOutputDeviceDescriptions(GetOutputDevicesCallback cb) override {
    std::move(cb).Run(device_descriptions);
    return std::unique_ptr<MockMediaNotificationDeviceProvider::
                               GetOutputDevicesCallbackList::Subscription>(
        nullptr);
  }

  media::AudioDeviceDescriptions device_descriptions;
};

class MockMediaNotificationAudioDeviceSelectorViewDelegate
    : public MediaNotificationAudioDeviceSelectorViewDelegate {
 public:
  MOCK_METHOD(void,
              OnAudioSinkChosen,
              (const std::string& sink_id),
              (override));
  MOCK_METHOD(void, OnAudioDeviceSelectorViewSizeChanged, (), (override));
};

}  // anonymous namespace

class MediaNotificationAudioDeviceSelectorViewTest
    : public ChromeViewsTestBase {
 public:
  MediaNotificationAudioDeviceSelectorViewTest() = default;
  ~MediaNotificationAudioDeviceSelectorViewTest() override = default;

  // ChromeViewsTestBase
  void SetUp() override {
    ChromeViewsTestBase::SetUp();
    provider_ = std::make_unique<MockMediaNotificationDeviceProvider>();
    media_router::MediaRouterFactory::GetInstance()->SetTestingFactory(
        &profile_, base::BindRepeating(&media_router::MockMediaRouter::Create));
    service_ = std::make_unique<MediaNotificationService>(&profile_);
  }

  void TearDown() override {
    view_.reset();
    service_.reset();
    provider_.reset();
    ChromeViewsTestBase::TearDown();
  }

  std::string GetButtonText(views::View* view) {
    return base::UTF16ToUTF8(static_cast<views::LabelButton*>(view)->GetText());
  }

  TestingProfile profile_;
  std::unique_ptr<MockMediaNotificationDeviceProvider> provider_;
  std::unique_ptr<MediaNotificationService> service_;
  std::unique_ptr<MediaNotificationAudioDeviceSelectorView> view_;
};

TEST_F(MediaNotificationAudioDeviceSelectorViewTest, DeviceButtonsCreated) {
  // Buttons should be created for every device reported by the provider
  provider_->AddDevice("Speaker", "1");
  provider_->AddDevice("Headphones", "2");
  provider_->AddDevice("Earbuds", "3");
  service_->set_device_provider_for_testing(std::move(provider_));

  MockMediaNotificationAudioDeviceSelectorViewDelegate delegate;
  view_ = std::make_unique<MediaNotificationAudioDeviceSelectorView>(
      &delegate, service_.get(), gfx::Size(), "1");

  std::vector<std::string> button_texts;
  ASSERT_TRUE(view_->device_button_container_ != nullptr);

  std::transform(
      view_->device_button_container_->children().cbegin(),
      view_->device_button_container_->children().cend(),
      std::back_inserter(button_texts), [](views::View* child) {
        return base::UTF16ToASCII(
            static_cast<const views::LabelButton*>(child)->GetText());
      });
  EXPECT_THAT(button_texts, testing::UnorderedElementsAre(
                                "Speaker", "Headphones", "Earbuds"));
}

TEST_F(MediaNotificationAudioDeviceSelectorViewTest,
       DeviceButtonClickNotifiesContainer) {
  // When buttons are clicked the media notification container should be
  // informed.
  provider_->AddDevice("Speaker", "1");
  provider_->AddDevice("Headphones", "2");
  provider_->AddDevice("Earbuds", "3");
  service_->set_device_provider_for_testing(std::move(provider_));

  MockMediaNotificationAudioDeviceSelectorViewDelegate delegate;
  EXPECT_CALL(delegate, OnAudioSinkChosen("1")).Times(1);
  EXPECT_CALL(delegate, OnAudioSinkChosen("2")).Times(1);
  EXPECT_CALL(delegate, OnAudioSinkChosen("3")).Times(1);

  view_ = std::make_unique<MediaNotificationAudioDeviceSelectorView>(
      &delegate, service_.get(), gfx::Size(), "1");

  for (views::View* child : view_->device_button_container_->children()) {
    view_->ButtonPressed(
        static_cast<views::Button*>(child),
        ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), 0, 0));
  }
}

TEST_F(MediaNotificationAudioDeviceSelectorViewTest, CurrentDeviceHighlighted) {
  // The 'current' audio device should be highlighted in the UI and appear
  // before other devices.
  provider_->AddDevice("Speaker", "1");
  provider_->AddDevice("Headphones", "2");
  provider_->AddDevice("Earbuds", "3");
  service_->set_device_provider_for_testing(std::move(provider_));

  MockMediaNotificationAudioDeviceSelectorViewDelegate delegate;
  view_ = std::make_unique<MediaNotificationAudioDeviceSelectorView>(
      &delegate, service_.get(), gfx::Size(), "3");

  auto* first_button = static_cast<views::MdTextButton*>(
      view_->device_button_container_->children().front());
  EXPECT_EQ(first_button->GetText(), base::UTF8ToUTF16("Earbuds"));
  EXPECT_TRUE(first_button->GetProminent());
}

TEST_F(MediaNotificationAudioDeviceSelectorViewTest,
       DeviceHighlightedOnChange) {
  // When the audio output device changes, the UI should highlight that one.
  provider_->AddDevice("Speaker", "1");
  provider_->AddDevice("Headphones", "2");
  provider_->AddDevice("Earbuds", "3");
  service_->set_device_provider_for_testing(std::move(provider_));

  MockMediaNotificationAudioDeviceSelectorViewDelegate delegate;
  view_ = std::make_unique<MediaNotificationAudioDeviceSelectorView>(
      &delegate, service_.get(), gfx::Size(), "1");

  auto button_is_highlighted = [](views::View* view) {
    return static_cast<views::MdTextButton*>(view)->GetProminent();
  };

  auto& buttons = view_->device_button_container_->children();

  // There should be only one highlighted button. It should be the first button.
  // It's text should be "Speaker"
  EXPECT_EQ(
      std::count_if(buttons.begin(), buttons.end(), button_is_highlighted), 1);
  EXPECT_EQ(std::find_if(buttons.begin(), buttons.end(), button_is_highlighted),
            buttons.begin());
  EXPECT_EQ(GetButtonText(buttons.front()), "Speaker");

  // Simulate a device change
  view_->UpdateCurrentAudioDevice("3");

  // The button for "Earbuds" should come before all others & be highlighted.
  EXPECT_EQ(
      std::count_if(buttons.begin(), buttons.end(), button_is_highlighted), 1);
  EXPECT_EQ(std::find_if(buttons.begin(), buttons.end(), button_is_highlighted),
            buttons.begin());
  EXPECT_EQ(GetButtonText(buttons.front()), "Earbuds");
}
