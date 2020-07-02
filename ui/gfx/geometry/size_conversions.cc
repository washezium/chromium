// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/geometry/size_conversions.h"

#include "ui/gfx/geometry/safe_integer_conversions.h"

namespace gfx {

Size ToFlooredSize(const SizeF& size) {
  return Size(ToFlooredInt(size.width()), ToFlooredInt(size.height()));
}

Size ToCeiledSize(const SizeF& size) {
  return Size(ToCeiledInt(size.width()), ToCeiledInt(size.height()));
}

Size ToRoundedSize(const SizeF& size) {
  return Size(ToRoundedInt(size.width()), ToRoundedInt(size.height()));
}

}  // namespace gfx

