// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_BLOOM_PUBLIC_CPP_BLOOM_INTERACTION_RESOLUTION_H_
#define CHROMEOS_COMPONENTS_BLOOM_PUBLIC_CPP_BLOOM_INTERACTION_RESOLUTION_H_

#include <string>

namespace chromeos {
namespace bloom {

enum class BloomInteractionResolution {
  // Bloom interaction completed normally.
  kNormal = 0,
  // Bloom interaction failed to fetch an access token.
  kNoAccessToken = 1,
  // Bloom interaction failed to take a screenshot
  // (or the user aborted while taking a screenshot).
  kNoScreenshot = 2,
};

std::string ToString(BloomInteractionResolution resolution);

}  // namespace bloom
}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_BLOOM_PUBLIC_CPP_BLOOM_INTERACTION_RESOLUTION_H_
