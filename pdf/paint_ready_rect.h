// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PAINT_READY_RECT_H_
#define PDF_PAINT_READY_RECT_H_

#include "ppapi/cpp/image_data.h"
#include "ui/gfx/geometry/rect.h"

namespace pp {
class Rect;
}  // namespace pp

namespace chrome_pdf {

// Stores information about a rectangle that has finished painting. The
// `PaintManager` will paint it only when everything else on the screen is also
// ready.
struct PaintReadyRect {
  PaintReadyRect(const pp::Rect& rect,
                 const pp::ImageData& image_data,
                 bool flush_now = false);
  PaintReadyRect(const PaintReadyRect& other);
  PaintReadyRect& operator=(const PaintReadyRect& other);

  gfx::Rect rect;
  pp::ImageData image_data;

  // Whether to flush to screen immediately; otherwise, when the rest of the
  // plugin viewport is ready.
  bool flush_now;
};

}  // namespace chrome_pdf

#endif  // PDF_PAINT_READY_RECT_H_
