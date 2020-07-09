// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_UI_SUGGESTION_WINDOW_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_UI_SUGGESTION_WINDOW_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "ui/chromeos/ui_chromeos_export.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/button_observer.h"
#include "ui/views/metadata/metadata_header_macros.h"

namespace chromeos {
struct AssistiveWindowProperties;
}  // namespace chromeos

namespace views {
class ImageButton;
class Link;
}

namespace ui {
namespace ime {

class AssistiveDelegate;
class SuggestionView;
struct AssistiveWindowButton;
struct SuggestionDetails;

// SuggestionWindowView is the main container of the suggestion window UI.
class UI_CHROMEOS_EXPORT SuggestionWindowView
    : public views::BubbleDialogDelegateView,
      public views::ButtonListener,
      public views::ButtonObserver {
 public:
  METADATA_HEADER(SuggestionWindowView);

  // Creates a bubble widget containing a SuggestionWindowView.  Returns a
  // pointer to the contained view.
  static SuggestionWindowView* Create(gfx::NativeView parent,
                                      AssistiveDelegate* delegate);

  // views::BubbleDialogDelegateView:
  std::unique_ptr<views::NonClientFrameView> CreateNonClientFrameView(
      views::Widget* widget) override;

  // Shows suggestion text.
  void Show(const SuggestionDetails& details);

  void ShowMultipleCandidates(
      const chromeos::AssistiveWindowProperties& properties);

  // This highlights/unhighlights a valid button based on the given params.
  // Only one button of the same id will be highlighted at anytime.
  void SetButtonHighlighted(const AssistiveWindowButton& button,
                            bool highlighted);

  views::View* GetCandidateAreaForTesting();
  views::View* GetSettingLinkViewForTesting();
  views::View* GetLearnMoreButtonForTesting();

 private:
  SuggestionWindowView(gfx::NativeView parent, AssistiveDelegate* delegate);
  ~SuggestionWindowView() override;

  // Overridden from views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // views::ButtonObserver's override:
  void OnStateChanged(views::Button* observed_button,
                      views::Button::ButtonState old_state) override;

  // views::View's override:
  void OnThemeChanged() override;

  std::unique_ptr<views::ImageButton> CreateLearnMoreButton();

  void MaybeInitializeSuggestionViews(size_t candidates_size);

  void MakeVisible();

  // This highlights at most one |candidate| at any time.
  // No-op if the candidate is already highlighted.
  void HighlightCandidate(SuggestionView* candidate);

  // This unhighlights the given |candidate|.
  // No-op if the candidate is currently not highlighted.
  void UnhighlightCandidate(SuggestionView* candidate);

  // The delegate to handle events from this class.
  AssistiveDelegate* delegate_;

  // The view containing all the suggestions.
  views::View* candidate_area_;

  // The setting link, positioned below candidate_area_.
  // TODO(crbug/1102175): Rename setting to settings since there can be multiple
  // things to set.
  views::Link* setting_link_;

  views::ImageButton* learn_more_button_;

  // The currently-highlighted candidate, if any.
  SuggestionView* highlighted_candidate_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SuggestionWindowView);
};

}  // namespace ime
}  // namespace ui

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_UI_SUGGESTION_WINDOW_VIEW_H_
