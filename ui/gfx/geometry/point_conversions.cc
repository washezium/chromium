// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/geometry/point_conversions.h"

#include "ui/gfx/geometry/safe_integer_conversions.h"

namespace gfx {

Point ToFlooredPoint(const PointF& point) {
  return Point(ToFlooredInt(point.x()), ToFlooredInt(point.y()));
}

Point ToCeiledPoint(const PointF& point) {
  return Point(ToCeiledInt(point.x()), ToCeiledInt(point.y()));
}

Point ToRoundedPoint(const PointF& point) {
  return Point(ToRoundedInt(point.x()), ToRoundedInt(point.y()));
}

}  // namespace gfx

