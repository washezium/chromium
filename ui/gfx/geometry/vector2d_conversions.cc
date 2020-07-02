// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/geometry/vector2d_conversions.h"

#include "ui/gfx/geometry/safe_integer_conversions.h"

namespace gfx {

Vector2d ToFlooredVector2d(const Vector2dF& vector2d) {
  return Vector2d(ToFlooredInt(vector2d.x()), ToFlooredInt(vector2d.y()));
}

Vector2d ToCeiledVector2d(const Vector2dF& vector2d) {
  return Vector2d(ToCeiledInt(vector2d.x()), ToCeiledInt(vector2d.y()));
}

Vector2d ToRoundedVector2d(const Vector2dF& vector2d) {
  return Vector2d(ToRoundedInt(vector2d.x()), ToRoundedInt(vector2d.y()));
}

}  // namespace gfx

