// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_GLOBAL_MEDIA_CONTROLS_MEDIA_NOTIFICATION_AUDIO_DEVICE_SELECTOR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_GLOBAL_MEDIA_CONTROLS_MEDIA_NOTIFICATION_AUDIO_DEVICE_SELECTOR_VIEW_H_

#include "chrome/browser/ui/global_media_controls/media_notification_device_provider.h"
#include "media/audio/audio_device_description.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/box_layout.h"

namespace {
class AudioDeviceEntryView;
}  // anonymous namespace

class MediaNotificationAudioDeviceSelectorViewDelegate;
class MediaNotificationService;

class MediaNotificationAudioDeviceSelectorView : public views::View,
                                                 public views::ButtonListener {
 public:
  MediaNotificationAudioDeviceSelectorView(
      MediaNotificationAudioDeviceSelectorViewDelegate* delegate,
      MediaNotificationService* service,
      const std::string& current_device_id,
      const SkColor& foreground_color,
      const SkColor& background_color);
  ~MediaNotificationAudioDeviceSelectorView() override;

  // Called when audio output devices are discovered.
  void UpdateAvailableAudioDevices(
      const media::AudioDeviceDescriptions& device_descriptions);
  // Called when an audio device switch has occurred
  void UpdateCurrentAudioDevice(const std::string& current_device_id);
  void OnColorsChanged(const SkColor& foreground_color,
                       const SkColor& background_color);

  // ButtonListener
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  static std::string get_entry_label_for_testing(views::View* entry_view);
  static bool get_entry_is_highlighted_for_testing(views::View* entry_view);

 private:
  FRIEND_TEST_ALL_PREFIXES(MediaNotificationAudioDeviceSelectorViewTest,
                           DeviceButtonsCreated);
  FRIEND_TEST_ALL_PREFIXES(MediaNotificationAudioDeviceSelectorViewTest,
                           ExpandButtonOpensEntryContainer);
  FRIEND_TEST_ALL_PREFIXES(MediaNotificationAudioDeviceSelectorViewTest,
                           DeviceButtonClickNotifiesContainer);
  FRIEND_TEST_ALL_PREFIXES(MediaNotificationAudioDeviceSelectorViewTest,
                           CurrentDeviceHighlighted);
  FRIEND_TEST_ALL_PREFIXES(MediaNotificationAudioDeviceSelectorViewTest,
                           DeviceHighlightedOnChange);
  FRIEND_TEST_ALL_PREFIXES(MediaNotificationAudioDeviceSelectorViewTest,
                           DeviceButtonsChange);

  bool ShouldBeVisible(
      const media::AudioDeviceDescriptions& device_descriptions);

  void ShowDevices();
  void HideDevices();

  bool is_expanded_ = false;
  MediaNotificationAudioDeviceSelectorViewDelegate* const delegate_;
  std::string current_device_id_;
  SkColor foreground_color_, background_color_;
  AudioDeviceEntryView* current_device_entry_view_ = nullptr;

  // Child views
  views::View* expand_button_strip_;
  views::LabelButton* expand_button_;
  views::View* audio_device_entries_container_;

  std::unique_ptr<MediaNotificationDeviceProvider::
                      GetOutputDevicesCallbackList::Subscription>
      audio_device_subscription_;

  base::WeakPtrFactory<MediaNotificationAudioDeviceSelectorView>
      weak_ptr_factory_{this};
};

#endif  // CHROME_BROWSER_UI_VIEWS_GLOBAL_MEDIA_CONTROLS_MEDIA_NOTIFICATION_AUDIO_DEVICE_SELECTOR_VIEW_H_
