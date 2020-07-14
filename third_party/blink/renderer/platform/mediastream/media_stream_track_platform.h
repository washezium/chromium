// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_MEDIASTREAM_MEDIA_STREAM_TRACK_PLATFORM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_MEDIASTREAM_MEDIA_STREAM_TRACK_PLATFORM_H_

#include <string>

#include "base/callback.h"
#include "third_party/blink/public/platform/modules/mediastream/web_media_stream_track.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

// MediaStreamTrackPlatform is a low-level object backing a
// WebMediaStreamTrack.
class PLATFORM_EXPORT MediaStreamTrackPlatform {
 public:
  explicit MediaStreamTrackPlatform(bool is_local_track);
  virtual ~MediaStreamTrackPlatform();

  static MediaStreamTrackPlatform* GetTrack(const WebMediaStreamTrack& track);

  virtual void SetEnabled(bool enabled) = 0;

  virtual void SetContentHint(
      WebMediaStreamTrack::ContentHintType content_hint) = 0;

  // If |callback| is not null, it is invoked when the track has stopped.
  virtual void StopAndNotify(base::OnceClosure callback) = 0;

  void Stop() { StopAndNotify(base::OnceClosure()); }

  // TODO(hta): Make method pure virtual when all tracks have the method.
  virtual void GetSettings(WebMediaStreamTrack::Settings& settings) {}

  bool is_local_track() const { return is_local_track_; }

 private:
  const bool is_local_track_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaStreamTrackPlatform);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_MEDIASTREAM_MEDIA_STREAM_TRACK_PLATFORM_H_
