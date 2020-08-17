// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_COMMAND_LINE_POLICY_PROVIDER_H_
#define COMPONENTS_POLICY_CORE_COMMON_COMMAND_LINE_POLICY_PROVIDER_H_

#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/core/common/policy_loader_command_line.h"
#include "components/policy/policy_export.h"

namespace policy {

// The policy provider for the Command Line Policy which is used for development
// and testing purposes.
class POLICY_EXPORT CommandLinePolicyProvider
    : public ConfigurationPolicyProvider {
 public:
  explicit CommandLinePolicyProvider(const base::CommandLine& command_line);
  CommandLinePolicyProvider(const CommandLinePolicyProvider&) = delete;
  CommandLinePolicyProvider& operator=(const CommandLinePolicyProvider&) =
      delete;

  ~CommandLinePolicyProvider() override;

  // ConfigurationPolicyProvider implementation.
  void RefreshPolicies() override;

 private:
  PolicyLoaderCommandLine loader_;
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_COMMAND_LINE_POLICY_PROVIDER_H_
