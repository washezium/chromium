// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css/css_scroll_timeline.h"

#include "third_party/blink/renderer/core/css/css_id_selector_value.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/style_rule.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"

namespace blink {

namespace {

bool IsIdentifier(const CSSValue* value, CSSValueID value_id) {
  if (const auto* ident = DynamicTo<CSSIdentifierValue>(value))
    return ident->GetValueID() == value_id;
  return false;
}

bool IsAuto(const CSSValue* value) {
  return IsIdentifier(value, CSSValueID::kAuto);
}

bool IsNone(const CSSValue* value) {
  return IsIdentifier(value, CSSValueID::kNone);
}

const cssvalue::CSSIdSelectorValue* GetIdSelectorValue(const CSSValue* value) {
  if (const auto* selector = DynamicTo<CSSFunctionValue>(value)) {
    if (selector->FunctionType() != CSSValueID::kSelector)
      return nullptr;
    DCHECK_EQ(selector->length(), 1u);
    return DynamicTo<cssvalue::CSSIdSelectorValue>(selector->Item(0));
  }
  return nullptr;
}

Element* ComputeScrollSource(Element* element, const CSSValue* value) {
  if (const auto* id = GetIdSelectorValue(value))
    return element->GetDocument().getElementById(id->Id());
  if (IsNone(value))
    return nullptr;
  DCHECK(!value || IsAuto(value));
  return element->GetDocument().scrollingElement();
}

ScrollTimeline::ScrollDirection ComputeScrollDirection(const CSSValue* value) {
  CSSValueID value_id = CSSValueID::kAuto;

  if (const auto* identifier = DynamicTo<CSSIdentifierValue>(value))
    value_id = identifier->GetValueID();

  switch (value_id) {
    case CSSValueID::kInline:
      return ScrollTimeline::Inline;
    case CSSValueID::kHorizontal:
      return ScrollTimeline::Horizontal;
    case CSSValueID::kVertical:
      return ScrollTimeline::Vertical;
    case CSSValueID::kAuto:
    case CSSValueID::kBlock:
    default:
      return ScrollTimeline::Block;
  }
}

ScrollTimelineOffset* ComputeScrollOffset(const CSSValue* value) {
  if (auto* primitive_value = DynamicTo<CSSPrimitiveValue>(value))
    return MakeGarbageCollected<ScrollTimelineOffset>(primitive_value);
  DCHECK(!value || IsAuto(value));
  return MakeGarbageCollected<ScrollTimelineOffset>();
}

HeapVector<Member<ScrollTimelineOffset>>* ComputeScrollOffsets(
    const CSSValue* start,
    const CSSValue* end) {
  auto* offsets =
      MakeGarbageCollected<HeapVector<Member<ScrollTimelineOffset>>>();
  offsets->push_back(ComputeScrollOffset(start));
  offsets->push_back(ComputeScrollOffset(end));
  return offsets;
}

base::Optional<double> ComputeTimeRange(const CSSValue* value) {
  if (auto* primitive = DynamicTo<CSSPrimitiveValue>(value))
    return primitive->ComputeSeconds() * 1000.0;
  // TODO(crbug.com/1097041): Support 'auto' value.
  return base::nullopt;
}

}  // anonymous namespace

CSSScrollTimeline::Options::Options(Element* element,
                                    StyleRuleScrollTimeline& rule)
    : source_(ComputeScrollSource(element, rule.GetSource())),
      direction_(ComputeScrollDirection(rule.GetOrientation())),
      offsets_(ComputeScrollOffsets(rule.GetStart(), rule.GetEnd())),
      time_range_(ComputeTimeRange(rule.GetTimeRange())) {}

CSSScrollTimeline::CSSScrollTimeline(Document* document, const Options& options)
    : ScrollTimeline(document,
                     options.source_,
                     options.direction_,
                     options.offsets_,
                     *options.time_range_) {
  DCHECK(options.IsValid());
}

}  // namespace blink
