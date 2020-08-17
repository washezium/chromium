// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/well_known_change_password_navigation_throttle.h"

#include "base/logging.h"
#include "chrome/browser/password_manager/change_password_url_service_factory.h"
#include "components/password_manager/core/browser/change_password_url_service.h"
#include "components/password_manager/core/browser/well_known_change_password_state.h"
#include "components/password_manager/core/browser/well_known_change_password_util.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

using content::NavigationHandle;
using content::NavigationThrottle;
using content::WebContents;
using password_manager::IsWellKnownChangePasswordUrl;
using password_manager::WellKnownChangePasswordState;

// Used to scope the posted navigation task to the lifetime of |web_contents|.
class WebContentsLifetimeHelper
    : public content::WebContentsUserData<WebContentsLifetimeHelper> {
 public:
  explicit WebContentsLifetimeHelper(WebContents* web_contents)
      : web_contents_(web_contents) {}

  base::WeakPtr<WebContentsLifetimeHelper> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void NavigateTo(const content::OpenURLParams& url_params) {
    web_contents_->OpenURL(url_params);
  }

 private:
  friend class content::WebContentsUserData<WebContentsLifetimeHelper>;

  WebContents* const web_contents_;
  base::WeakPtrFactory<WebContentsLifetimeHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsLifetimeHelper)

}  // namespace

// static
std::unique_ptr<WellKnownChangePasswordNavigationThrottle>
WellKnownChangePasswordNavigationThrottle::MaybeCreateThrottleFor(
    NavigationHandle* handle) {
  const GURL& url = handle->GetURL();
  // The order is important. We have to check if it as a well-known change
  // password url first. We should only check the feature flag when the feature
  // would be used. Otherwise the we would not see a difference between control
  // and experiment groups on the dashboards.
  if (IsWellKnownChangePasswordUrl(url) &&
      base::FeatureList::IsEnabled(
          password_manager::features::kWellKnownChangePassword)) {
    return base::WrapUnique(
        new WellKnownChangePasswordNavigationThrottle(handle));
  }
  return nullptr;
}

WellKnownChangePasswordNavigationThrottle::
    WellKnownChangePasswordNavigationThrottle(NavigationHandle* handle)
    : NavigationThrottle(handle),
      change_password_url_service_(
          ChangePasswordUrlServiceFactory::GetForBrowserContext(
              handle->GetWebContents()->GetBrowserContext())) {
  change_password_url_service_->PrefetchURLs();
}

WellKnownChangePasswordNavigationThrottle::
    ~WellKnownChangePasswordNavigationThrottle() = default;

NavigationThrottle::ThrottleCheckResult
WellKnownChangePasswordNavigationThrottle::WillStartRequest() {
  auto url_loader_factory =
      content::BrowserContext::GetDefaultStoragePartition(
          navigation_handle()->GetWebContents()->GetBrowserContext())
          ->GetURLLoaderFactoryForBrowserProcess();
  well_known_change_password_state_.FetchNonExistingResource(
      url_loader_factory.get(), navigation_handle()->GetURL());
  return NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
WellKnownChangePasswordNavigationThrottle::WillFailRequest() {
  return NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
WellKnownChangePasswordNavigationThrottle::WillProcessResponse() {
  // PostTask because the Throttle needs to be deferred before the status code
  // is set. After setting the status code Resume() can be called synchronous
  // and thereby before the throttle is deferred. This would result in a crash.
  // Unretained is safe because the NavigationThrottle is deferred and can only
  // be continued after the callback finished.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &WellKnownChangePasswordState::SetChangePasswordResponseCode,
          base::Unretained(&well_known_change_password_state_),
          navigation_handle()->GetResponseHeaders()->response_code()));
  return NavigationThrottle::DEFER;
}

const char* WellKnownChangePasswordNavigationThrottle::GetNameForLogging() {
  return "WellKnownChangePasswordNavigationThrottle";
}

void WellKnownChangePasswordNavigationThrottle::OnProcessingFinished(
    bool is_supported) {
  if (is_supported) {
    Resume();
  } else {
    GURL url = navigation_handle()->GetURL();
    GURL redirect_url = change_password_url_service_->GetChangePasswordUrl(url);
    Redirect(redirect_url.is_valid() ? redirect_url : url.GetOrigin());
    CancelDeferredNavigation(NavigationThrottle::CANCEL);
  }
}

void WellKnownChangePasswordNavigationThrottle::Redirect(const GURL& url) {
  content::OpenURLParams params =
      content::OpenURLParams::FromNavigationHandle(navigation_handle());
  params.url = url;
  params.transition = ui::PAGE_TRANSITION_CLIENT_REDIRECT;

  WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents)
    return;

  WebContentsLifetimeHelper::CreateForWebContents(web_contents);
  WebContentsLifetimeHelper* helper =
      WebContentsLifetimeHelper::FromWebContents(web_contents);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&WebContentsLifetimeHelper::NavigateTo,
                                helper->GetWeakPtr(), std::move(params)));
}

