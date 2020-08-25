// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAINT_PREVIEW_PLAYER_COMPOSITOR_STATUS_H_
#define COMPONENTS_PAINT_PREVIEW_PLAYER_COMPOSITOR_STATUS_H_

namespace paint_preview {

// GENERATED_JAVA_ENUM_PACKAGE: (
//   org.chromium.components.paintpreview.player)
enum class CompositorStatus : int {
  OK,
  URL_MISMATCH,
  COMPOSITOR_SERVICE_DISCONNECT,
  COMPOSITOR_CLIENT_DISCONNECT,
  PROTOBUF_DESERIALIZATION_ERROR,
  COMPOSITOR_DESERIALIZATION_ERROR,
  INVALID_ROOT_FRAME_SKP,
  INVALID_REQUEST,
  COUNT,
};

}  // namespace paint_preview

#endif  // COMPONENTS_PAINT_PREVIEW_PLAYER_COMPOSITOR_STATUS_H_
