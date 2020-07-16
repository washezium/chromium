// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_SYSTEM_APPS_PUBLIC_JS_MESSAGE_PIPE_BROWSERTEST_H_
#define CHROMEOS_COMPONENTS_SYSTEM_APPS_PUBLIC_JS_MESSAGE_PIPE_BROWSERTEST_H_

#include "chromeos/components/web_applications/test/js_library_test.h"

class MessagePipeBrowserTestBase : public JsLibraryTest {
 public:
  MessagePipeBrowserTestBase();
  ~MessagePipeBrowserTestBase() override;

  MessagePipeBrowserTestBase(const MessagePipeBrowserTestBase&) = delete;
  MessagePipeBrowserTestBase& operator=(const MessagePipeBrowserTestBase&) =
      delete;
};

#endif  // CHROMEOS_COMPONENTS_SYSTEM_APPS_PUBLIC_JS_MESSAGE_PIPE_BROWSERTEST_H_
