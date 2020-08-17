// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/command_line_policy_provider.h"

#include <memory>

#include "base/values.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_switches.h"
#include "components/policy/core/common/policy_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

void VerifyPolicyProvider(ConfigurationPolicyProvider* provider) {
  const base::Value* policy_value =
      provider->policies()
          .Get(PolicyNamespace(POLICY_DOMAIN_CHROME, std::string()))
          .GetValue("policy");
  ASSERT_TRUE(policy_value);
  ASSERT_TRUE(policy_value->is_int());
  EXPECT_EQ(10, policy_value->GetInt());
}

}  // namespace

class CommandLinePolicyProviderTest : public ::testing::Test {
 public:
  CommandLinePolicyProviderTest() {
    command_line_.AppendSwitchASCII(switches::kChromePolicy,
                                    R"({"policy":10})");
  }

  std::unique_ptr<CommandLinePolicyProvider> CreatePolicyProvider() {
    return std::make_unique<CommandLinePolicyProvider>(command_line_);
  }

 private:
  base::CommandLine command_line_{base::CommandLine::NO_PROGRAM};
};

TEST_F(CommandLinePolicyProviderTest, LoadAndRefresh) {
  std::unique_ptr<CommandLinePolicyProvider> policy_provider =
      CreatePolicyProvider();
  VerifyPolicyProvider(policy_provider.get());

  policy_provider->RefreshPolicies();
  VerifyPolicyProvider(policy_provider.get());
}
}  // namespace policy
