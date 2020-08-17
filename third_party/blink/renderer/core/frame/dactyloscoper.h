// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_DACTYLOSCOPER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_DACTYLOSCOPER_H_

#include "third_party/blink/public/common/privacy_budget/identifiable_token.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/svg/svg_string_list_tear_off.h"
#include "third_party/blink/renderer/core/typed_arrays/array_buffer_view_helpers.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExecutionContext;

class CORE_EXPORT Dactyloscoper {
  DISALLOW_NEW();

 public:
  Dactyloscoper();

  void Record(WebFeature);

  static void Record(ExecutionContext*, WebFeature);

  static void RecordDirectSurface(ExecutionContext*,
                                  WebFeature,
                                  IdentifiableToken);
  static void RecordDirectSurface(ExecutionContext*, WebFeature, String);
  static void RecordDirectSurface(ExecutionContext*,
                                  WebFeature,
                                  Vector<String>);
  static void RecordDirectSurface(ExecutionContext*,
                                  WebFeature,
                                  NotShared<DOMFloat32Array>);
  static void RecordDirectSurface(ExecutionContext*,
                                  WebFeature,
                                  SVGStringListTearOff*);

 private:
  DISALLOW_COPY_AND_ASSIGN(Dactyloscoper);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_DACTYLOSCOPER_H_
