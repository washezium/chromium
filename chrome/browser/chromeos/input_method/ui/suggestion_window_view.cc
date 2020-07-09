// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/ui/suggestion_window_view.h"

#include <stddef.h>

#include <string>

#include "base/i18n/number_formatting.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/input_method/assistive_window_properties.h"
#include "chrome/browser/chromeos/input_method/ui/assistive_delegate.h"
#include "chrome/browser/chromeos/input_method/ui/border_factory.h"
#include "chrome/browser/chromeos/input_method/ui/suggestion_details.h"
#include "chrome/browser/chromeos/input_method/ui/suggestion_view.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_properties.h"

namespace ui {
namespace ime {

const int kSettingLinkFontSize = 13;
// TODO(crbug/1094843): Add localised string.
const char kSettingLinkLabel[] = "Why am I seeing this suggestion?";
// TODO(crbug/1099044): Update and use cros colors.
constexpr SkColor kSecondaryIconColor = gfx::kGoogleGrey500;

// TODO(crbug/1102175): Rename setting to settings since there can be multiple
// things to set.
class SettingLinkView : public views::View {
 public:
  explicit SettingLinkView(AssistiveDelegate* delegate) : delegate_(delegate) {
    SetLayoutManager(std::make_unique<views::FillLayout>());
    setting_link_ = AddChildView(
        std::make_unique<views::Link>(base::UTF8ToUTF16(kSettingLinkLabel)));
    setting_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    // TODO(crbug/1102215): Implement proper UI layout using Insets constant.
    const gfx::Insets insets(0, kPadding, kPadding, kPadding);
    setting_link_->SetBorder(views::CreateEmptyBorder(insets));
    setting_link_->SetFontList(gfx::FontList({kFontStyle}, gfx::Font::ITALIC,
                                             kSettingLinkFontSize,
                                             gfx::Font::Weight::NORMAL));
    setting_link_->set_callback(base::BindRepeating(
        &SettingLinkView::LinkClicked, base::Unretained(this)));
  }

  void SetHighlighted(bool highlighted) {
    if (highlighted_ == highlighted)
      return;

    SetBackground(highlighted
                      ? views::CreateSolidBackground(kButtonHighlightColor)
                      : nullptr);
    highlighted_ = highlighted;
    SchedulePaint();
  }

 private:
  AssistiveDelegate* delegate_;
  views::Link* setting_link_;
  bool highlighted_ = false;

  void LinkClicked() {
    AssistiveWindowButton button;
    button.id = ButtonId::kSmartInputsSettingLink;
    delegate_->AssistiveWindowButtonClicked(button);
  }

  DISALLOW_COPY_AND_ASSIGN(SettingLinkView);
};

SuggestionWindowView::SuggestionWindowView(gfx::NativeView parent,
                                           AssistiveDelegate* delegate)
    : delegate_(delegate) {
  DialogDelegate::SetButtons(ui::DIALOG_BUTTON_NONE);
  SetCanActivate(false);
  DCHECK(parent);
  set_parent_window(parent);
  set_margins(gfx::Insets());

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  candidate_area_ = AddChildView(std::make_unique<views::View>());
  candidate_area_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  setting_link_view_ =
      AddChildView(std::make_unique<SettingLinkView>(delegate));
  setting_link_view_->SetVisible(false);

  learn_more_button_ = AddChildView(CreateLearnMoreButton());
}

SuggestionWindowView::~SuggestionWindowView() = default;

// static
SuggestionWindowView* SuggestionWindowView::Create(
    gfx::NativeView parent,
    AssistiveDelegate* delegate) {
  auto* const view = new SuggestionWindowView(parent, delegate);
  views::Widget* const widget =
      views::BubbleDialogDelegateView::CreateBubble(view);
  wm::SetWindowVisibilityAnimationTransition(widget->GetNativeView(),
                                             wm::ANIMATE_NONE);
  return view;
}

std::unique_ptr<views::NonClientFrameView>
SuggestionWindowView::CreateNonClientFrameView(views::Widget* widget) {
  std::unique_ptr<views::NonClientFrameView> frame =
      views::BubbleDialogDelegateView::CreateNonClientFrameView(widget);
  static_cast<views::BubbleFrameView*>(frame.get())
      ->SetBubbleBorder(GetBorderForWindow(WindowBorderType::Suggestion));
  return frame;
}

std::unique_ptr<views::ImageButton>
SuggestionWindowView::CreateLearnMoreButton() {
  auto button = std::make_unique<views::ImageButton>(this);
  button->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  button->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  button->SetFocusForPlatform();
  button->SetTooltipText(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  button->SetBorder(views::CreatePaddedBorder(
      views::CreateSolidSidedBorder(
          1, 0, 0, 0,
          GetNativeTheme()->GetSystemColor(
              ui::NativeTheme::kColorId_FootnoteContainerBorder)),
      views::LayoutProvider::Get()->GetInsetsMetric(
          views::INSETS_VECTOR_IMAGE_BUTTON)));
  button->AddButtonObserver(this);
  button->SetVisible(false);
  return button;
}

void SuggestionWindowView::MakeVisible() {
  candidate_area_->SetVisible(true);
  SizeToContents();
}

void SuggestionWindowView::Show(const SuggestionDetails& details) {
  MaybeInitializeSuggestionViews(1);
  auto* const candidate =
      static_cast<SuggestionView*>(candidate_area_->children().front());
  candidate->SetEnabled(true);
  candidate->SetView(details);
  if (details.show_setting_link)
    candidate->SetMinWidth(setting_link_view_->GetPreferredSize().width());
  setting_link_view_->SetVisible(details.show_setting_link);
  MakeVisible();
}

void SuggestionWindowView::ShowMultipleCandidates(
    const chromeos::AssistiveWindowProperties& properties) {
  const std::vector<base::string16>& candidates = properties.candidates;
  MaybeInitializeSuggestionViews(candidates.size());
  for (size_t i = 0; i < candidates.size(); i++) {
    auto* const candidate =
        static_cast<SuggestionView*>(candidate_area_->children()[i]);
    if (properties.show_indices) {
      candidate->SetViewWithIndex(base::FormatNumber(i + 1), candidates[i]);
    } else {
      SuggestionDetails details;
      details.text = candidates[i];
      candidate->SetView(details);
    }
    candidate->SetEnabled(true);
  }
  learn_more_button_->SetVisible(true);
  MakeVisible();
}

void SuggestionWindowView::MaybeInitializeSuggestionViews(
    size_t candidates_size) {
  if (highlighted_candidate_)
    UnhighlightCandidate(highlighted_candidate_);

  const views::View::Views& candidates = candidate_area_->children();
  while (candidates.size() > candidates_size) {
    std::unique_ptr<views::View> child =
        candidate_area_->RemoveChildViewT(candidates.back());
    static_cast<SuggestionView*>(child.get())->RemoveButtonObserver(this);
  }

  while (candidates.size() < candidates_size) {
    candidate_area_->AddChildView(std::make_unique<SuggestionView>(this))
        ->AddButtonObserver(this);
  }
}

void SuggestionWindowView::SetButtonHighlighted(
    const AssistiveWindowButton& button,
    bool highlighted) {
  switch (button.id) {
    case ButtonId::kSuggestion: {
      const views::View::Views& candidates = candidate_area_->children();
      if (button.index < candidates.size()) {
        auto* const candidate =
            static_cast<SuggestionView*>(candidates[button.index]);
        if (highlighted)
          HighlightCandidate(candidate);
        else
          UnhighlightCandidate(candidate);
      }
      break;
    }
    case ButtonId::kSmartInputsSettingLink:
      setting_link_view_->SetHighlighted(highlighted);
      break;
    case ButtonId::kLearnMore:
      SetLearnMoreButtonHighlighted(highlighted);
      break;
    default:
      break;
  }
}

void SuggestionWindowView::HighlightCandidate(SuggestionView* candidate) {
  DCHECK(candidate);
  DCHECK_EQ(candidate_area_, candidate->parent());

  // Can't highlight a highlighted candidate.
  if (candidate == highlighted_candidate_)
    return;

  if (highlighted_candidate_)
    UnhighlightCandidate(highlighted_candidate_);
  candidate->SetHighlighted(true);
  highlighted_candidate_ = candidate;
}

void SuggestionWindowView::UnhighlightCandidate(SuggestionView* candidate) {
  DCHECK(candidate);
  DCHECK_EQ(candidate_area_, candidate->parent());

  // Can't unhighlight an unhighlighted candidate.
  if (candidate != highlighted_candidate_)
    return;

  candidate->SetHighlighted(false);
  highlighted_candidate_ = nullptr;
}

// TODO(b/1101669): Create abstract HighlightableButton for learn_more button,
// setting_link_view, suggestion_view and undo_view.
void SuggestionWindowView::SetLearnMoreButtonHighlighted(bool highlighted) {
  if (is_learn_more_button_highlighted == highlighted) {
    return;
  }

  learn_more_button_->SetBackground(
      highlighted ? views::CreateSolidBackground(kButtonHighlightColor)
                  : nullptr);
  is_learn_more_button_highlighted = highlighted;

  SchedulePaint();
}

// TODO(crbug/1099116): Add test for ButtonPressed.
void SuggestionWindowView::ButtonPressed(views::Button* sender,
                                         const ui::Event& event) {
  if (sender == learn_more_button_) {
    AssistiveWindowButton button;
    button.id = ui::ime::ButtonId::kLearnMore;
    button.window_type = ui::ime::AssistiveWindowType::kEmojiSuggestion;
    delegate_->AssistiveWindowButtonClicked(button);
    return;
  }

  if (sender->parent() == candidate_area_) {
    AssistiveWindowButton button;
    button.id = ui::ime::ButtonId::kSuggestion;
    button.index = candidate_area_->GetIndexOf(sender);
    delegate_->AssistiveWindowButtonClicked(button);
  }
}

// TODO(crbug/1099062): Add tests for mouse hovered and pressed.
void SuggestionWindowView::OnStateChanged(
    views::Button* observed_button,
    views::Button::ButtonState old_state) {
  if (observed_button == learn_more_button_) {
    switch (observed_button->state()) {
      case views::Button::ButtonState::STATE_HOVERED:
      case views::Button::ButtonState::STATE_PRESSED:
        SetLearnMoreButtonHighlighted(true);
        break;
      default:
        SetLearnMoreButtonHighlighted(false);
    }
    return;
  }

  if (observed_button->parent() == candidate_area_) {
    auto* const candidate = static_cast<SuggestionView*>(observed_button);
    switch (observed_button->state()) {
      case views::Button::ButtonState::STATE_HOVERED:
      case views::Button::ButtonState::STATE_PRESSED:
        HighlightCandidate(candidate);
        break;
      default:
        UnhighlightCandidate(candidate);
    }
  }
}

void SuggestionWindowView::OnThemeChanged() {
  learn_more_button_->SetImage(
      views::Button::ButtonState::STATE_NORMAL,
      gfx::CreateVectorIcon(vector_icons::kHelpOutlineIcon,
                            kSecondaryIconColor));
  BubbleDialogDelegateView::OnThemeChanged();
}

views::View* SuggestionWindowView::GetCandidateAreaForTesting() {
  return candidate_area_;
}

views::View* SuggestionWindowView::GetSettingLinkViewForTesting() {
  return setting_link_view_;
}

views::View* SuggestionWindowView::GetLearnMoreButtonForTesting() {
  return learn_more_button_;
}

BEGIN_METADATA(SuggestionWindowView)
METADATA_PARENT_CLASS(views::BubbleDialogDelegateView)
END_METADATA()

}  // namespace ime
}  // namespace ui
