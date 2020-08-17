// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/command_line_policy_provider.h"

#include <memory>
#include <utility>

#include "components/policy/core/common/policy_bundle.h"

namespace policy {

CommandLinePolicyProvider::CommandLinePolicyProvider(
    const base::CommandLine& command_line)
    : loader_(command_line) {
  RefreshPolicies();
}
CommandLinePolicyProvider::~CommandLinePolicyProvider() = default;

void CommandLinePolicyProvider::RefreshPolicies() {
  std::unique_ptr<PolicyBundle> bundle = loader_.Load();
  UpdatePolicy(std::move(bundle));
}

}  // namespace policy
