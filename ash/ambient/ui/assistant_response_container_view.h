// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_AMBIENT_UI_ASSISTANT_RESPONSE_CONTAINER_VIEW_H_
#define ASH_AMBIENT_UI_ASSISTANT_RESPONSE_CONTAINER_VIEW_H_

#include <memory>

#include "ash/assistant/ui/main_stage/animated_container_view.h"
#include "base/macros.h"

namespace ash {

class AssistantErrorElement;
class AssistantTextElement;
class AssistantViewDelegate;

class AssistantResponseContainerView : public AnimatedContainerView {
 public:
  explicit AssistantResponseContainerView(AssistantViewDelegate* delegate);
  ~AssistantResponseContainerView() override;

  // AnimatedContainerView:
  const char* GetClassName() const override;
  gfx::Size CalculatePreferredSize() const override;
  void OnContentsPreferredSizeChanged(views::View* content_view) override;

 private:
  void InitLayout();
  void AddTextElementView(const AssistantTextElement* text_element);
  void AddErrorElementView(const AssistantErrorElement* error_element);

  // AnimatedContainerView:
  std::unique_ptr<ElementAnimator> HandleUiElement(
      const AssistantUiElement* ui_element) override;

  DISALLOW_COPY_AND_ASSIGN(AssistantResponseContainerView);
};

}  //  namespace ash

#endif  // ASH_AMBIENT_UI_ASSISTANT_RESPONSE_CONTAINER_VIEW_H_
