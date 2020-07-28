// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "cc/test/pixel_comparator.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/base/ui_base_features.h"

// TODO(crbug.com/958242): Move the baselines to skia gold for easier
//   rebaselining when all platforms are supported

// To rebaseline this test on all platforms:
// 1. Run a CQ+1 dry run.
// 2. Click the failing bots for android, windows, mac, and linux.
// 3. Find the failing interactive_ui_browsertests step.
// 4. Click the "Deterministic failure" link for the failing test case.
// 5. Copy the "Actual pixels" data url and paste into browser.
// 6. Save the image into your chromium checkout in
//    chrome/test/data/focus_rings.

class FocusRingBrowserTest : public InProcessBrowserTest {
 public:
  FocusRingBrowserTest() {
    feature_list_.InitWithFeatures({features::kFormControlsRefresh}, {});
  }

  void SetUp() override {
    EnablePixelOutput(/*force_device_scale_factor=*/1.f);
    InProcessBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);

    // The --disable-lcd-text flag helps text render more similarly on
    // different bots and platform.
    command_line->AppendSwitch(switches::kDisableLCDText);
  }

  void RunTest(const std::string& screenshot_filename,
               const std::string& body_html,
               int screenshot_width,
               int screenshot_height) {
    base::ScopedAllowBlockingForTesting allow_blocking;

    ASSERT_TRUE(features::IsFormControlsRefreshEnabled());

    base::FilePath dir_test_data;
    ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &dir_test_data));

    std::string platform_suffix;
#if defined(OS_MACOSX)
    platform_suffix = "_mac";
#elif defined(OS_WIN)
    platform_suffix = "_win";
#elif defined(OS_CHROMEOS)
    platform_suffix = "_chromeos";
#endif

    base::FilePath golden_filepath =
        dir_test_data.AppendASCII("focus_rings")
            .AppendASCII(screenshot_filename + ".png");

    base::FilePath golden_filepath_platform =
        golden_filepath.InsertBeforeExtensionASCII(platform_suffix);
    if (base::PathExists(golden_filepath_platform)) {
      golden_filepath = golden_filepath_platform;
    }

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(content::NavigateToURL(
        web_contents,
        GURL("data:text/html,<!DOCTYPE html><body>" + body_html + "</body>")));
    ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));

#if defined(OS_MACOSX)
    // This fuzzy pixel comparator handles slight differences between most
    // different versions of MacOS.
    cc::FuzzyPixelComparator comparator(
        /* discard_alpha */ true,
        /* error_pixels_percentage_limit */ 9.f,
        /* small_error_pixels_percentage_limit */ 0.f,
        /* avg_abs_error_limit */ 20.f,
        /* max_abs_error_limit */ 78.f,
        /* small_error_threshold */ 0);
#else
    cc::ExactPixelComparator comparator(/* disard_alpha */ true);
#endif

    EXPECT_TRUE(CompareWebContentsOutputToReference(
        web_contents, golden_filepath,
        gfx::Size(screenshot_width, screenshot_height), comparator));
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(FocusRingBrowserTest, Checkbox) {
  RunTest("focus_ring_browsertest_checkbox",
          "<input type=checkbox autofocus>"
          "<input type=checkbox>",
          /* screenshot_width */ 60,
          /* screenshot_height */ 40);
}

IN_PROC_BROWSER_TEST_F(FocusRingBrowserTest, Radio) {
  RunTest("focus_ring_browsertest_radio",
          "<input type=radio autofocus>"
          "<input type=radio>",
          /* screenshot_width */ 60,
          /* screenshot_height */ 40);
}

IN_PROC_BROWSER_TEST_F(FocusRingBrowserTest, Button) {
  RunTest("focus_ring_browsertest_button",
          "<button autofocus>button</button>"
          "<br>"
          "<br>"
          "<button>button</button>",
          /* screenshot_width */ 80,
          /* screenshot_height */ 80);
}

IN_PROC_BROWSER_TEST_F(FocusRingBrowserTest, Anchor) {
  RunTest("focus_ring_browsertest_anchor",
          "<div style='text-align: center; width: 80px;'>"
          "  <a href='foo' autofocus>line one<br>two</a>"
          "</div>"
          "<br>"
          "<div style='text-align: center; width: 80px;'>"
          "  <a href='foo'>line one<br>two</a>"
          "</div>",
          /* screenshot_width */ 90,
          /* screenshot_height */ 130);
}
