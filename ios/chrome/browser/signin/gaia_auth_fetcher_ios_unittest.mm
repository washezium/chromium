// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/gaia_auth_fetcher_ios.h"

#include <memory>

#include "base/run_loop.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/web/public/test/web_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class MockGaiaAuthFetcherIOSBridge : public GaiaAuthFetcherIOSBridge {
 public:
  MockGaiaAuthFetcherIOSBridge(
      GaiaAuthFetcherIOSBridge::GaiaAuthFetcherIOSBridgeDelegate* delegate,
      web::BrowserState* browser_state)
      : GaiaAuthFetcherIOSBridge(delegate, browser_state) {}

  MOCK_METHOD0(Cancel, void());
  MOCK_METHOD0(FetchPendingRequest, void());
};

class MockGaiaConsumer : public GaiaAuthConsumer {
 public:
  MockGaiaConsumer() {}
  ~MockGaiaConsumer() {}

  MOCK_METHOD1(OnMergeSessionSuccess, void(const std::string& data));
  MOCK_METHOD1(OnClientLoginFailure, void(const GoogleServiceAuthError& error));
  MOCK_METHOD1(OnLogOutFailure, void(const GoogleServiceAuthError& error));
  MOCK_METHOD1(OnGetCheckConnectionInfoSuccess, void(const std::string& data));
};
}

// Tests fixture for GaiaAuthFetcherIOS
class GaiaAuthFetcherIOSTest : public PlatformTest {
 protected:
  GaiaAuthFetcherIOSTest() {
    browser_state_ = TestChromeBrowserState::Builder().Build();

    gaia_auth_fetcher_.reset(new GaiaAuthFetcherIOS(
        &consumer_, gaia::GaiaSource::kChrome,
        test_url_loader_factory_.GetSafeWeakWrapper(), browser_state_.get()));
    gaia_auth_fetcher_->bridge_.reset(new MockGaiaAuthFetcherIOSBridge(
        gaia_auth_fetcher_.get(), browser_state_.get()));
  }

  ~GaiaAuthFetcherIOSTest() override {
    gaia_auth_fetcher_.reset();
  }

  MockGaiaAuthFetcherIOSBridge* GetBridge() {
    return static_cast<MockGaiaAuthFetcherIOSBridge*>(
        gaia_auth_fetcher_->bridge_.get());
  }

  web::WebTaskEnvironment task_environment_;
  std::unique_ptr<ChromeBrowserState> browser_state_;
  MockGaiaConsumer consumer_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  std::unique_ptr<GaiaAuthFetcherIOS> gaia_auth_fetcher_;
};

// Tests that the cancel mechanism works properly by cancelling an OAuthLogin
// request and controlling that the consumer is properly called.
TEST_F(GaiaAuthFetcherIOSTest, StartOAuthLoginCancelled) {
  MockGaiaAuthFetcherIOSBridge* bridge = GetBridge();

  EXPECT_CALL(*bridge, FetchPendingRequest());
  gaia_auth_fetcher_->StartOAuthLogin("fake_token", "gaia");

  GoogleServiceAuthError expected_error =
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED);
  EXPECT_CALL(consumer_, OnClientLoginFailure(expected_error));
  EXPECT_CALL(*bridge, Cancel()).WillOnce([&bridge]() {
    bridge->OnURLFetchFailure(net::ERR_ABORTED, 0);
  });
  gaia_auth_fetcher_->CancelRequest();
}

// Tests that the successful case works properly by starting a MergeSession
// request, making it succeed and controlling that the consumer is properly
// called.
TEST_F(GaiaAuthFetcherIOSTest, StartMergeSession) {
  MockGaiaAuthFetcherIOSBridge* bridge = GetBridge();

  EXPECT_CALL(*bridge, FetchPendingRequest()).WillOnce([&bridge]() {
    bridge->OnURLFetchSuccess("data", 200);
  });
  EXPECT_CALL(consumer_, OnMergeSessionSuccess("data"));
  gaia_auth_fetcher_->StartMergeSession("uber_token", "");
}

// Tests that the failure case works properly by starting a LogOut request,
// making it fail, and controlling that the consumer is properly called.
TEST_F(GaiaAuthFetcherIOSTest, StartLogOutError) {
  MockGaiaAuthFetcherIOSBridge* bridge = GetBridge();

  GoogleServiceAuthError expected_error =
      GoogleServiceAuthError(GoogleServiceAuthError::CONNECTION_FAILED);
  EXPECT_CALL(consumer_, OnLogOutFailure(expected_error));
  EXPECT_CALL(*bridge, FetchPendingRequest()).WillOnce([&bridge]() {
    bridge->OnURLFetchFailure(net::ERR_FAILED, 500);
  });
  gaia_auth_fetcher_->StartLogOut();
}

// Tests that requests that do not require cookies are using the original
// GaiaAuthFetcher and not the GaiaAuthFetcherIOS specialization.
TEST_F(GaiaAuthFetcherIOSTest, StartGetCheckConnectionInfo) {
  std::string data(
      "[{\"carryBackToken\": \"token1\", \"url\": \"http://www.google.com\"}]");
  EXPECT_CALL(consumer_, OnGetCheckConnectionInfoSuccess(data)).Times(1);

  // Set up the fake response.
  test_url_loader_factory_.AddResponse(
      GaiaUrls::GetInstance()
          ->GetCheckConnectionInfoURLWithSource(GaiaConstants::kChromeSource)
          .spec(),
      data);

  gaia_auth_fetcher_->StartGetCheckConnectionInfo();
  base::RunLoop().RunUntilIdle();
}
