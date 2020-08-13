// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/android_app_communication.h"

#include <utility>

#include "base/callback.h"
#include "base/optional.h"

namespace payments {
namespace {

class AndroidAppCommunicationStub : public AndroidAppCommunication {
 public:
  explicit AndroidAppCommunicationStub(content::BrowserContext* context)
      : AndroidAppCommunication(context) {}

  ~AndroidAppCommunicationStub() override = default;

  // AndroidAppCommunication implementation.
  void GetAppDescriptions(const std::string& twa_package_name,
                          GetAppDescriptionsCallback callback) override {
    std::move(callback).Run(/*error_message=*/base::nullopt,
                            /*app_descriptions=*/{});
  }

  // AndroidAppCommunication implementation.
  void SetForTesting() override {}
};

}  // namespace

// Declared in cross-platform header file. See:
// components/payments/content/android_app_communication.h
// static
std::unique_ptr<AndroidAppCommunication> AndroidAppCommunication::Create(
    content::BrowserContext* context) {
  return std::make_unique<AndroidAppCommunicationStub>(context);
}

}  // namespace payments
