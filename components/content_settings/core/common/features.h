// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_FEATURES_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_FEATURES_H_

#include "base/component_export.h"
#include "build/build_config.h"

namespace base {
struct Feature;
}  // namespace base

namespace content_settings {

#if defined(OS_IOS)
// Feature to enable a better cookie controls ui.
COMPONENT_EXPORT(CONTENT_SETTINGS_FEATURES)
extern const base::Feature kImprovedCookieControls;
#endif

// Feature to disallow wildcard pattern matching for plugin content settings
COMPONENT_EXPORT(CONTENT_SETTINGS_FEATURES)
extern const base::Feature kDisallowWildcardsInPluginContentSettings;

// Feature to remove the chrome.contentSettings.plugins.set() API in extensions
// for extensions.
COMPONENT_EXPORT(CONTENT_SETTINGS_FEATURES)
extern const base::Feature kDisallowExtensionsToSetPluginContentSettings;

}  // namespace content_settings

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_FEATURES_H_
