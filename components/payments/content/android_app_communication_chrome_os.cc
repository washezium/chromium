// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/android_app_communication.h"

#include <utility>

#include "components/arc/mojom/payment_app.mojom.h"
#include "components/arc/pay/arc_payment_app_bridge.h"
#include "components/payments/core/android_app_description.h"
#include "components/payments/core/chrome_os_error_strings.h"
#include "components/payments/core/method_strings.h"
#include "components/payments/core/native_error_strings.h"
#include "content/public/browser/browser_thread.h"

namespace payments {
namespace {

void OnIsImplemented(
    const std::string& twa_package_name,
    AndroidAppCommunication::GetAppDescriptionsCallback callback,
    arc::mojom::IsPaymentImplementedResultPtr response) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!twa_package_name.empty());

  if (response.is_null()) {
    std::move(callback).Run(errors::kEmptyResponse, /*app_descriptions=*/{});
    return;
  }

  if (response->is_error()) {
    std::move(callback).Run(response->get_error(), /*app_descriptions=*/{});
    return;
  }

  if (!response->is_valid()) {
    std::move(callback).Run(errors::kInvalidResponse, /*app_descriptions=*/{});
    return;
  }

  if (response->get_valid()->activity_names.empty()) {
    // If a TWA does not implement PAY intent in any of its activities, then
    // |activity_names| is empty, which is not an error.
    std::move(callback).Run(/*error_message=*/base::nullopt,
                            /*app_descriptions=*/{});
    return;
  }

  if (response->get_valid()->activity_names.size() != 1U) {
    std::move(callback).Run(errors::kMoreThanOneActivity,
                            /*app_descriptions=*/{});
    return;
  }

  if (response->get_valid()->service_names.size() > 1U) {
    std::move(callback).Run(errors::kMoreThanOneService,
                            /*app_descriptions=*/{});
    return;
  }

  auto activity = std::make_unique<AndroidActivityDescription>();
  activity->name = response->get_valid()->activity_names.front();

  // The only available payment method identifier in a Chrome OS TWA at this
  // time.
  activity->default_payment_method = methods::kGooglePlayBilling;

  auto app = std::make_unique<AndroidAppDescription>();
  app->package = twa_package_name;
  app->activities.emplace_back(std::move(activity));
  app->service_names = response->get_valid()->service_names;

  std::vector<std::unique_ptr<AndroidAppDescription>> app_descriptions;
  app_descriptions.emplace_back(std::move(app));

  std::move(callback).Run(/*error_message=*/base::nullopt,
                          std::move(app_descriptions));
}

// Invokes the TWA Android app in Android subsystem on Chrome OS.
class AndroidAppCommunicationChromeOS : public AndroidAppCommunication {
 public:
  explicit AndroidAppCommunicationChromeOS(content::BrowserContext* context)
      : AndroidAppCommunication(context),
        get_app_service_(base::BindRepeating(
            &arc::ArcPaymentAppBridge::GetForBrowserContext)) {}

  ~AndroidAppCommunicationChromeOS() override = default;

  // Disallow copy and assign.
  AndroidAppCommunicationChromeOS(
      const AndroidAppCommunicationChromeOS& other) = delete;
  AndroidAppCommunicationChromeOS& operator=(
      const AndroidAppCommunicationChromeOS& other) = delete;

  // AndroidAppCommunication implementation:
  void GetAppDescriptions(const std::string& twa_package_name,
                          GetAppDescriptionsCallback callback) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (twa_package_name.empty()) {
      // Chrome OS supports Android app payment only through a TWA. An empty
      // |twa_package_name| indicates that Chrome was not launched from a TWA,
      // so there're no payment apps available.
      std::move(callback).Run(/*error_message=*/base::nullopt,
                              /*app_descriptions=*/{});
      return;
    }

    auto* payment_app_service = get_app_service_.Run(context());
    if (!payment_app_service) {
      std::move(callback).Run(errors::kUnableToInvokeAndroidPaymentApps,
                              /*app_descriptions=*/{});
      return;
    }

    payment_app_service->IsPaymentImplemented(
        twa_package_name, base::BindOnce(&OnIsImplemented, twa_package_name,
                                         std::move(callback)));
  }

  // AndroidAppCommunication implementation:
  void SetForTesting() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    get_app_service_ = base::BindRepeating(
        &arc::ArcPaymentAppBridge::GetForBrowserContextForTesting);
  }

 private:
  base::RepeatingCallback<arc::ArcPaymentAppBridge*(content::BrowserContext*)>
      get_app_service_;
};

}  // namespace

// Declared in cross-platform header file. See:
// //components/payments/content/android_app_communication.h
// static
std::unique_ptr<AndroidAppCommunication> AndroidAppCommunication::Create(
    content::BrowserContext* context) {
  return std::make_unique<AndroidAppCommunicationChromeOS>(context);
}

}  // namespace payments
