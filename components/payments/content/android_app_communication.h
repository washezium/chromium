// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_ANDROID_APP_COMMUNICATION_H_
#define COMPONENTS_PAYMENTS_CONTENT_ANDROID_APP_COMMUNICATION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/supports_user_data.h"
#include "components/payments/core/android_app_description.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace payments {

// Invokes Android payment apps. This object is owned by BrowserContext, so it
// should only be accessed on UI thread, where BrowserContext lives.
class AndroidAppCommunication : public base::SupportsUserData::Data {
 public:
  using GetAppDescriptionsCallback = base::OnceCallback<void(
      const base::Optional<std::string>& error_message,
      std::vector<std::unique_ptr<AndroidAppDescription>> app_descriptions)>;

  // Returns a weak pointer to the instance of AndroidAppCommunication that is
  // owned by the given |context|, which should not be null.
  static base::WeakPtr<AndroidAppCommunication> GetForBrowserContext(
      content::BrowserContext* context);

  ~AndroidAppCommunication() override;

  // Disallow copy and assign.
  AndroidAppCommunication(const AndroidAppCommunication& other) = delete;
  AndroidAppCommunication& operator=(const AndroidAppCommunication& other) =
      delete;

  // Looks up installed Android apps that support making payments. If running in
  // TWA mode, the |twa_package_name| parameter is the name of the Android
  // package of the TWA that invoked Chrome, or an empty string otherwise.
  virtual void GetAppDescriptions(const std::string& twa_package_name,
                                  GetAppDescriptionsCallback callback) = 0;

  // Enables the testing mode.
  virtual void SetForTesting() = 0;

 protected:
  explicit AndroidAppCommunication(content::BrowserContext* context);

  content::BrowserContext* context() { return context_; }

 private:
  // Defined in platform-specific implementation files. See:
  // components/payments/content/android_app_communication_chromeos.cc
  // components/payments/content/android_app_communication_stub.cc
  static std::unique_ptr<AndroidAppCommunication> Create(
      content::BrowserContext* context);

  // Owns this object, so always valid.
  content::BrowserContext* context_;

  base::WeakPtrFactory<AndroidAppCommunication> weak_ptr_factory_{this};
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_ANDROID_APP_COMMUNICATION_H_
