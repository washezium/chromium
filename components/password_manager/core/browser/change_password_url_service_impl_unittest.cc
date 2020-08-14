// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "components/password_manager/core/browser/change_password_url_service_impl.h"

#include "base/logging.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr char kMockResponse[] = R"({
  "google.com": "https://google.com/change-password",
  "a.netlify.com": "https://a.netlify.com/change-password",
  "web.app": "https://web.app/change-password"
})";
}  // namespace

namespace password_manager {

class ChangePasswordUrlServiceTest : public testing::Test {
 public:
  ChangePasswordUrlServiceTest() {
    test_url_loader_factory_.AddResponse(
        password_manager::ChangePasswordUrlServiceImpl::
            kChangePasswordUrlOverrideUrl,
        kMockResponse);
    // Password Manager is enabled by default.
    test_pref_service_.registry()->RegisterBooleanPref(
        password_manager::prefs::kCredentialsEnableService, true);
  }

  // Fetches the url overrides and waits until the response arrived.
  void PrefetchAndWaitUntilDone();

  void DisablePasswordManagerEnabledPolicy() {
    test_pref_service_.SetBoolean(
        password_manager::prefs::kCredentialsEnableService, false);
  }

  GURL GetChangePasswordUrl(const GURL& url) {
    return change_password_url_service_.GetChangePasswordUrl(url);
  }

  void ClearMockResponses() { test_url_loader_factory_.ClearResponses(); }

 private:
  base::test::SingleThreadTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_ =
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &test_url_loader_factory_);
  TestingPrefServiceSimple test_pref_service_;
  ChangePasswordUrlServiceImpl change_password_url_service_{
      test_shared_loader_factory_, &test_pref_service_};
};

void ChangePasswordUrlServiceTest::PrefetchAndWaitUntilDone() {
  change_password_url_service_.PrefetchURLs();
  task_environment_.RunUntilIdle();
}

TEST_F(ChangePasswordUrlServiceTest, eTLDLookup) {
  // TODO(crbug.com/1086141): If possible mock eTLD registry to ensure sites are
  // listed.
  PrefetchAndWaitUntilDone();

  EXPECT_EQ(GetChangePasswordUrl(GURL("https://google.com/foo")),
            GURL("https://google.com/change-password"));
  EXPECT_EQ(GetChangePasswordUrl(GURL("https://a.google.com/foo")),
            GURL("https://google.com/change-password"));

  EXPECT_EQ(GetChangePasswordUrl(GURL("https://web.app")), GURL());

  EXPECT_EQ(GetChangePasswordUrl(GURL("https://netlify.com")), GURL());
  EXPECT_EQ(GetChangePasswordUrl(GURL("https://a.netlify.com")),
            GURL("https://a.netlify.com/change-password"));
  EXPECT_EQ(GetChangePasswordUrl(GURL("https://b.netlify.com")), GURL());

  EXPECT_EQ(GetChangePasswordUrl(GURL("https://notlisted.com/foo")), GURL());
}

TEST_F(ChangePasswordUrlServiceTest, PassworManagerPolicyDisabled) {
  DisablePasswordManagerEnabledPolicy();

  PrefetchAndWaitUntilDone();

  EXPECT_EQ(GetChangePasswordUrl(GURL("https://google.com/foo")), GURL());
}

TEST_F(ChangePasswordUrlServiceTest, NetworkRequestFails) {
  ClearMockResponses();

  PrefetchAndWaitUntilDone();

  EXPECT_EQ(GetChangePasswordUrl(GURL("https://google.com/foo")), GURL());
}

}  // namespace password_manager
