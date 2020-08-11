// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/reputation/url_elision_policy.h"

#include "base/test/scoped_feature_list.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/url_formatter/spoof_checks/common_words/common_words_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace test {
#include "components/url_formatter/spoof_checks/common_words/common_words_test-inc.cc"
}

class UrlElisionPolicyTest : public testing::Test {
 public:
  UrlElisionPolicyTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{omnibox::kMaybeElideToRegistrableDomain,
          // Note: This number is arbitrary; we just need to be able to build
          // hostnames that are reliably shorter/longer than the limit.
          {{"max_unelided_host_length", "20"}}}},
        {});
    url_formatter::common_words::SetCommonWordDAFSAForTesting(
        test::kDafsa, sizeof(test::kDafsa));
  }

  ~UrlElisionPolicyTest() override = default;

  void TearDown() override {
    url_formatter::common_words::ResetCommonWordDAFSAForTesting();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(UrlElisionPolicyTest);
};

// Ensure that long domains are elided.
TEST_F(UrlElisionPolicyTest, ElidesLongDomains) {
  GURL kUrl = GURL("http://abcdefghijklmnopqrs.tu/xyz");
  EXPECT_TRUE(ShouldElideToRegistrableDomain(kUrl));
}

// Ensure that short domains are not elided.
TEST_F(UrlElisionPolicyTest, DoesntElideShortDomains) {
  GURL kUrl = GURL("http://abc.d/xyz");
  EXPECT_FALSE(ShouldElideToRegistrableDomain(kUrl));
}

// Ensure that sensitive keywords cause elision when expected and not otherwise.
// Note further tests in SafetyTipHeuristicsTest.SensitiveKeywordsTest.
TEST_F(UrlElisionPolicyTest, ElidesKeywordedDomainsAppropriately) {
  // Note: "google" is a sensitive keyword, taken from top500-domains-inc.cc.
  EXPECT_TRUE(
      ShouldElideToRegistrableDomain(GURL("http://google-evil.com/xyz")));
  EXPECT_TRUE(
      ShouldElideToRegistrableDomain(GURL("http://google.evil.com/xyz")));

  // Elision shouldn't happen when the keyword is the e2LD.
  EXPECT_FALSE(
      ShouldElideToRegistrableDomain(GURL("http://www.google.com/xyz")));
  EXPECT_FALSE(ShouldElideToRegistrableDomain(GURL("http://google.com/xyz")));
  // But those domains aren't exempt in other parts of the domain.
  EXPECT_TRUE(
      ShouldElideToRegistrableDomain(GURL("http://google.google.com/xyz")));

  // If there are no keywords, there should be no elision.
  EXPECT_FALSE(
      ShouldElideToRegistrableDomain(GURL("http://cupcake-login.com/xyz")));
  EXPECT_FALSE(
      ShouldElideToRegistrableDomain(GURL("http://cupcake.login.com/xyz")));

  // We don't elide on invalid/missing TLDs.
  EXPECT_FALSE(ShouldElideToRegistrableDomain(GURL("http://google/xyz")));
  EXPECT_FALSE(
      ShouldElideToRegistrableDomain(GURL("http://google.notreal/xyz")));
  // Nor on non-HTTP(s)
  EXPECT_FALSE(
      ShouldElideToRegistrableDomain(GURL("ftp://google.login.com/xyz")));
}
