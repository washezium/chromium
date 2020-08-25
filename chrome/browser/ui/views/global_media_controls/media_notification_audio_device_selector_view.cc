// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/global_media_controls/media_notification_audio_device_selector_view.h"

#include "base/strings/utf_string_conversions.h"
#include "base/util/ranges/algorithm.h"
#include "chrome/browser/ui/global_media_controls/media_notification_container_impl.h"
#include "chrome/browser/ui/global_media_controls/media_notification_service.h"
#include "chrome/browser/ui/views/global_media_controls/media_notification_audio_device_selector_view_delegate.h"
#include "chrome/grit/chromium_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "media/audio/audio_device_description.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/style/typography.h"

namespace {

// Constants for the AudioDeviceEntryView
constexpr gfx::Insets kIconContainerInsets{10, 15};
constexpr int kDeviceIconSize = 18;
constexpr gfx::Insets kLabelsContainerInsets{18, 0};
constexpr gfx::Size kAudioDeviceEntryViewSize{400, 30};
constexpr int kEntryHighlightOpacity = 45;
// Constants for the MediaNotificationAudioDeviceSelectorView
constexpr gfx::Insets kExpandButtonStripInsets{6, 15};
constexpr gfx::Size kExpandButtonStripSize{400, 30};
constexpr gfx::Insets kExpandButtonBorderInsets{4, 8};
constexpr int kExpandButtonBorderCornerRadius = 16;

class AudioDeviceEntryView : public views::Button {
 public:
  AudioDeviceEntryView(const SkColor& foreground_color,
                       const SkColor& background_color,
                       const std::string& raw_device_id,
                       const std::string& name,
                       const std::string& subtext = "");

  const std::string& GetDeviceId() { return raw_device_id_; }
  const std::string& GetDeviceName() { return device_name_; }

  void SetHighlighted(bool highlighted);

  void OnColorsChanged(const SkColor& foreground_color,
                       const SkColor& background_color);

  bool is_highlighted_for_testing() { return is_highlighted_; }

 protected:
  SkColor foreground_color_, background_color_;
  const std::string raw_device_id_, device_name_;
  bool is_highlighted_ = false;
  views::View* icon_container_;
  views::ImageView* device_icon_;
  views::View* labels_container_;
  views::Label* device_name_label_;
  views::Label* device_subtext_label_ = nullptr;
};

}  // anonymous namespace

AudioDeviceEntryView::AudioDeviceEntryView(const SkColor& foreground_color,
                                           const SkColor& background_color,
                                           const std::string& raw_device_id,
                                           const std::string& name,
                                           const std::string& subtext)
    : foreground_color_(foreground_color),
      background_color_(background_color),
      raw_device_id_(raw_device_id),
      device_name_(name) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal));

  icon_container_ = AddChildView(std::make_unique<views::View>());
  auto* icon_container_layout =
      icon_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, kIconContainerInsets));
  icon_container_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  icon_container_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  device_icon_ =
      icon_container_->AddChildView(std::make_unique<views::ImageView>());
  device_icon_->SetImage(gfx::CreateVectorIcon(
      vector_icons::kHeadsetIcon, kDeviceIconSize, foreground_color));

  labels_container_ = AddChildView(std::make_unique<views::View>());
  auto* labels_container_layout_ =
      labels_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, kLabelsContainerInsets));
  labels_container_layout_->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  labels_container_layout_->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);

  views::Label::CustomFont device_name_label_font{
      views::Label::GetDefaultFontList().DeriveWithSizeDelta(1)};
  device_name_label_ =
      labels_container_->AddChildView(std::make_unique<views::Label>(
          base::UTF8ToUTF16(device_name_), device_name_label_font));
  device_name_label_->SetEnabledColor(foreground_color);
  device_name_label_->SetBackgroundColor(background_color);

  if (!subtext.empty()) {
    device_subtext_label_ = labels_container_->AddChildView(
        std::make_unique<views::Label>(base::UTF8ToUTF16(subtext)));
    device_subtext_label_->SetTextStyle(
        views::style::TextStyle::STYLE_SECONDARY);
    device_subtext_label_->SetEnabledColor(foreground_color);
    device_subtext_label_->SetBackgroundColor(background_color);
  }

  // Ensures that hovering over these items also hovers this view.
  icon_container_->set_can_process_events_within_subtree(false);
  labels_container_->set_can_process_events_within_subtree(false);

  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  SetInkDropMode(Button::InkDropMode::ON);
  set_ink_drop_base_color(foreground_color);
  set_has_ink_drop_action_on_click(true);
  SetPreferredSize(kAudioDeviceEntryViewSize);
}

void AudioDeviceEntryView::SetHighlighted(bool highlighted) {
  is_highlighted_ = highlighted;
  if (highlighted) {
    SetInkDropMode(Button::InkDropMode::OFF);
    set_has_ink_drop_action_on_click(false);
    SetBackground(views::CreateSolidBackground(
        SkColorSetA(GetInkDropBaseColor(), kEntryHighlightOpacity)));
  } else {
    SetInkDropMode(Button::InkDropMode::ON);
    set_has_ink_drop_action_on_click(true);
    SetBackground(nullptr);
  }
}

void AudioDeviceEntryView::OnColorsChanged(const SkColor& foreground_color,
                                           const SkColor& background_color) {
  foreground_color_ = foreground_color;
  background_color_ = background_color;
  set_ink_drop_base_color(foreground_color_);

  device_icon_->SetImage(gfx::CreateVectorIcon(
      vector_icons::kHeadsetIcon, kDeviceIconSize, foreground_color_));

  device_name_label_->SetEnabledColor(foreground_color_);
  device_name_label_->SetBackgroundColor(background_color_);

  if (device_subtext_label_) {
    device_subtext_label_->SetEnabledColor(foreground_color_);
    device_subtext_label_->SetBackgroundColor(background_color_);
  }

  // Reapply highlight formatting as some effects rely on these colors.
  SetHighlighted(is_highlighted_);
}

MediaNotificationAudioDeviceSelectorView::
    MediaNotificationAudioDeviceSelectorView(
        MediaNotificationAudioDeviceSelectorViewDelegate* delegate,
        MediaNotificationService* service,
        const std::string& current_device_id,
        const SkColor& foreground_color,
        const SkColor& background_color)
    : delegate_(delegate),
      current_device_id_(current_device_id),
      foreground_color_(foreground_color),
      background_color_(background_color) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  expand_button_strip_ = AddChildView(std::make_unique<views::View>());
  auto* expand_button_strip_layout =
      expand_button_strip_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          kExpandButtonStripInsets));
  expand_button_strip_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  expand_button_strip_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  expand_button_strip_->SetPreferredSize(kExpandButtonStripSize);

  auto expand_button = std::make_unique<views::LabelButton>(
      this, l10n_util::GetStringUTF16(
                IDS_GLOBAL_MEDIA_CONTROLS_DEVICES_BUTTON_LABEL));
  expand_button->SetTextColor(views::MdTextButton::ButtonState::STATE_NORMAL,
                              foreground_color_);
  expand_button->SetBackground(views::CreateSolidBackground(background_color_));
  auto border = std::make_unique<views::BubbleBorder>(
      views::BubbleBorder::Arrow::NONE, views::BubbleBorder::Shadow::NO_SHADOW,
      background_color_);
  border->set_insets(kExpandButtonBorderInsets);
  border->SetCornerRadius(kExpandButtonBorderCornerRadius);
  expand_button->SetBorder(std::move(border));
  expand_button_ = expand_button_strip_->AddChildView(std::move(expand_button));

  audio_device_entries_container_ =
      AddChildView(std::make_unique<views::View>());
  audio_device_entries_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  audio_device_entries_container_->SetVisible(false);

  SetBackground(views::CreateSolidBackground(background_color_));
  // Set the size of this view
  SetPreferredSize(kExpandButtonStripSize);
  Layout();

  // This view will become visible when devices are discovered.
  SetVisible(false);

  // Get a list of the connected audio output devices
  audio_device_subscription_ =
      service->RegisterAudioOutputDeviceDescriptionsCallback(
          base::BindRepeating(&MediaNotificationAudioDeviceSelectorView::
                                  UpdateAvailableAudioDevices,
                              weak_ptr_factory_.GetWeakPtr()));
}

void MediaNotificationAudioDeviceSelectorView::UpdateCurrentAudioDevice(
    const std::string& current_device_id) {
  if (current_device_entry_view_) {
    current_device_entry_view_->SetHighlighted(false);
    current_device_entry_view_ = nullptr;
  }

  auto it = util::ranges::find_if(
      audio_device_entries_container_->children(),
      [&current_device_id](auto& item) {
        return static_cast<AudioDeviceEntryView*>(item)->GetDeviceId() ==
               current_device_id;
      });

  if (it == audio_device_entries_container_->children().end())
    return;

  current_device_entry_view_ = static_cast<AudioDeviceEntryView*>(*it);
  current_device_entry_view_->SetHighlighted(true);
  audio_device_entries_container_->ReorderChildView(current_device_entry_view_,
                                                    0);

  current_device_entry_view_->Layout();
}

MediaNotificationAudioDeviceSelectorView::
    ~MediaNotificationAudioDeviceSelectorView() {
  audio_device_subscription_.release();
}

void MediaNotificationAudioDeviceSelectorView::UpdateAvailableAudioDevices(
    const media::AudioDeviceDescriptions& device_descriptions) {
  audio_device_entries_container_->RemoveAllChildViews(true);
  current_device_entry_view_ = nullptr;

  bool current_device_still_exists = false;
  for (auto description : device_descriptions) {
    auto device_entry_view = std::make_unique<AudioDeviceEntryView>(
        foreground_color_, background_color_, description.unique_id,
        description.device_name, "");
    device_entry_view->set_listener(this);
    audio_device_entries_container_->AddChildView(std::move(device_entry_view));
    if (!current_device_still_exists &&
        description.unique_id == current_device_id_)
      current_device_still_exists = true;
  }

  // If the current device no longer exists, fallback to the default device
  UpdateCurrentAudioDevice(
      current_device_still_exists
          ? current_device_id_
          : media::AudioDeviceDescription::kDefaultDeviceId);

  SetVisible(ShouldBeVisible(device_descriptions));
  delegate_->OnAudioDeviceSelectorViewSizeChanged();
}

void MediaNotificationAudioDeviceSelectorView::OnColorsChanged(
    const SkColor& foreground_color,
    const SkColor& background_color) {
  foreground_color_ = foreground_color;
  background_color_ = background_color;

  expand_button_->SetTextColor(views::MdTextButton::ButtonState::STATE_NORMAL,
                               foreground_color_);
  expand_button_->SetBackground(
      views::CreateSolidBackground(background_color_));
  SetBackground(views::CreateSolidBackground(background_color_));
  for (auto* view : audio_device_entries_container_->children()) {
    static_cast<AudioDeviceEntryView*>(view)->OnColorsChanged(
        foreground_color_, background_color_);
  }
  SchedulePaint();
}

void MediaNotificationAudioDeviceSelectorView::ButtonPressed(
    views::Button* sender,
    const ui::Event& event) {
  if (sender == expand_button_) {
    if (is_expanded_)
      HideDevices();
    else
      ShowDevices();

    delegate_->OnAudioDeviceSelectorViewSizeChanged();
  } else {
    DCHECK(std::find(audio_device_entries_container_->children().cbegin(),
                     audio_device_entries_container_->children().cend(),
                     sender) !=
           audio_device_entries_container_->children().end());
    delegate_->OnAudioSinkChosen(
        static_cast<AudioDeviceEntryView*>(sender)->GetDeviceId());
  }
}

// static
std::string
MediaNotificationAudioDeviceSelectorView::get_entry_label_for_testing(
    views::View* entry_view) {
  return static_cast<AudioDeviceEntryView*>(entry_view)->GetDeviceName();
}

// static
bool MediaNotificationAudioDeviceSelectorView::
    get_entry_is_highlighted_for_testing(views::View* entry_view) {
  return static_cast<AudioDeviceEntryView*>(entry_view)
      ->is_highlighted_for_testing();
}

void MediaNotificationAudioDeviceSelectorView::ShowDevices() {
  DCHECK(!is_expanded_);
  is_expanded_ = true;

  audio_device_entries_container_->SetVisible(true);
  PreferredSizeChanged();
}

void MediaNotificationAudioDeviceSelectorView::HideDevices() {
  DCHECK(is_expanded_);
  is_expanded_ = false;

  audio_device_entries_container_->SetVisible(false);
  PreferredSizeChanged();
}

bool MediaNotificationAudioDeviceSelectorView::ShouldBeVisible(
    const media::AudioDeviceDescriptions& device_descriptions) {
  // The UI should be visible if there are more than one unique devices. That is
  // when:
  // * There are at least three devices
  // * Or, there are two devices and one of them has the default ID but not the
  // default name.
  if (audio_device_entries_container_->children().size() == 2) {
    return util::ranges::any_of(
        audio_device_entries_container_->children(), [](views::View* view) {
          auto* entry = static_cast<AudioDeviceEntryView*>(view);
          return entry->GetDeviceId() ==
                     media::AudioDeviceDescription::kDefaultDeviceId &&
                 entry->GetDeviceName() !=
                     media::AudioDeviceDescription::GetDefaultDeviceName();
        });
  }
  return device_descriptions.size() > 2;
}
