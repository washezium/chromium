// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_WEB_APPLICATIONS_TEST_JS_LIBRARY_TEST_H_
#define CHROMEOS_COMPONENTS_WEB_APPLICATIONS_TEST_JS_LIBRARY_TEST_H_

#include <memory>

#include "chrome/test/base/mojo_web_ui_browser_test.h"

namespace base {
class FilePath;
}  // namespace base

namespace content {
class WebUIControllerFactory;
}  // namespace content

// Base test class used to test JS libraries for System Apps. It runs tests from
// chrome://system-app-test and loads files from |root_dir|.
class JsLibraryTest : public MojoWebUIBrowserTest {
 public:
  explicit JsLibraryTest(const base::FilePath& root_dir);
  ~JsLibraryTest() override;

  JsLibraryTest(const JsLibraryTest&) = delete;
  JsLibraryTest& operator=(const JsLibraryTest&) = delete;

 private:
  std::unique_ptr<content::WebUIControllerFactory> factory_;
};

#endif  // CHROMEOS_COMPONENTS_WEB_APPLICATIONS_TEST_JS_LIBRARY_TEST_H_
