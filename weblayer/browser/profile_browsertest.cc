// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/bind_test_util.h"
#include "build/build_config.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "weblayer/browser/favicon/favicon_fetcher_impl.h"
#include "weblayer/browser/favicon/test_favicon_fetcher_delegate.h"
#include "weblayer/browser/profile_impl.h"
#include "weblayer/browser/tab_impl.h"
#include "weblayer/shell/browser/shell.h"
#include "weblayer/test/weblayer_browser_test.h"
#include "weblayer/test/weblayer_browser_test_utils.h"

namespace weblayer {

using ProfileBrowserTest = WebLayerBrowserTest;

// TODO(crbug.com/654704): Android does not support PRE_ tests.
#if !defined(OS_ANDROID)

// UKM enabling via Profile persists across restarts.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, PRE_PersistUKM) {
  GetProfile()->SetBooleanSetting(SettingType::UKM_ENABLED, true);
}

IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, PersistUKM) {
  ASSERT_TRUE(GetProfile()->GetBooleanSetting(SettingType::UKM_ENABLED));
}

#endif  // !defined(OS_ANDROID)

IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, GetCachedFaviconForPageUrl) {
  // Navigation to a page with a favicon.
  ASSERT_TRUE(embedded_test_server()->Start());
  TestFaviconFetcherDelegate fetcher_delegate;
  auto fetcher = shell()->tab()->CreateFaviconFetcher(&fetcher_delegate);
  const GURL url =
      embedded_test_server()->GetURL("/simple_page_with_favicon.html");
  NavigateAndWaitForCompletion(url, shell());
  fetcher_delegate.WaitForFavicon();
  EXPECT_FALSE(fetcher_delegate.last_image().IsEmpty());
  EXPECT_EQ(1, fetcher_delegate.on_favicon_changed_call_count());

  // Request the favicon.
  base::RunLoop run_loop;
  static_cast<TabImpl*>(shell()->tab())
      ->profile()
      ->GetCachedFaviconForPageUrl(
          url, base::BindLambdaForTesting([&](gfx::Image image) {
            // The last parameter is the max difference allowed for each color
            // component. As the image is encoded before saving to disk some
            // variance is expected.
            EXPECT_TRUE(gfx::test::AreImagesClose(
                image, fetcher_delegate.last_image(), 10));
            run_loop.Quit();
          }));
  run_loop.Run();
}

}  // namespace weblayer
