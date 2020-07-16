// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Test suite for message_pipe.js
 */

GEN('#include "chromeos/components/system_apps/public/js/message_pipe_browsertest.h"');
GEN('#include "content/public/test/browser_test.h"');

var MessagePipeBrowserTest = class extends testing.Test {
  /** @override */
  get browsePreload() {
    return 'chrome://system-app-test/message_pipe_test.html';
  }

  /** @override */
  get runAccessibilityChecks() {
    return false;
  }

  /** @override */
  get typedefCppFixture() {
    return 'MessagePipeBrowserTestBase';
  }

  /** @override */
  get isAsync() {
    return true;
  }
};

TEST_F('MessagePipeBrowserTest', 'Empty', () => {
  testDone();
});
