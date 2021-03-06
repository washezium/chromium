// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/well_known_change_password_state.h"

#include "components/password_manager/core/browser/well_known_change_password_util.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

using password_manager::WellKnownChangePasswordState;
using password_manager::WellKnownChangePasswordStateDelegate;

namespace password_manager {

namespace {
// Creates a SimpleURLLoader for a request to the non existing resource path for
// a given |url|.
std::unique_ptr<network::SimpleURLLoader>
CreateResourceRequestToWellKnownNonExistingResourceFor(const GURL& url) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = CreateWellKnownNonExistingResourceURL(url);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags = net::LOAD_DISABLE_CACHE;
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation(
          "well_known_path_that_should_not_exist",
          R"(
        semantics {
          sender: "Password Manager"
          description:
            "Check whether the site supports .well-known 'special' URLs."
            "If the website does not support the spec we navigate to the "
            "fallback url. See also "
"https://wicg.github.io/change-password-url/response-code-reliability.html#iana"
          trigger:
            "When the user clicks 'Change password' on "
            "chrome://settings/passwords, or when they visit the "
            "[ORIGIN]/.well-known/change-password special URL, Chrome makes "
            "this additional request. Chrome Password manager shows a button "
            "with the link in the password checkup for compromised passwords "
            "view (chrome://settings/passwords/check) and in a dialog when the "
            "user signs in using compromised credentials."
          data:
            "The request body is empty. No user data is included."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled."
          policy_exception_justification: "Essential for navigation."
        })");
  return network::SimpleURLLoader::Create(std::move(resource_request),
                                          traffic_annotation);
}
}  // namespace

WellKnownChangePasswordState::WellKnownChangePasswordState(
    WellKnownChangePasswordStateDelegate* delegate)
    : delegate_(delegate) {}

WellKnownChangePasswordState::~WellKnownChangePasswordState() = default;

void WellKnownChangePasswordState::FetchNonExistingResource(
    network::SharedURLLoaderFactory* url_loader_factory,
    const GURL& url) {
  url_loader_ = CreateResourceRequestToWellKnownNonExistingResourceFor(url);
  // Binding the callback to |this| is safe, because the State exists until
  // OnProcessingFinished is called which can only be called after the response
  // arrives.
  url_loader_->DownloadHeadersOnly(
      url_loader_factory,
      base::BindOnce(
          &WellKnownChangePasswordState::FetchNonExistingResourceCallback,
          base::Unretained(this)));
}

void WellKnownChangePasswordState::SetChangePasswordResponseCode(
    int status_code) {
  change_password_response_code_ = status_code;
  ContinueProcessing();
}

void WellKnownChangePasswordState::FetchNonExistingResourceCallback(
    scoped_refptr<net::HttpResponseHeaders> headers) {
  non_existing_resource_response_code_ =
      headers ? headers->response_code() : -1;
  ContinueProcessing();
}

void WellKnownChangePasswordState::ContinueProcessing() {
  if (!BothRequestsFinished())
    return;
  delegate_->OnProcessingFinished(SupportsChangePasswordUrl());
}

bool WellKnownChangePasswordState::BothRequestsFinished() const {
  return non_existing_resource_response_code_ != 0 &&
         change_password_response_code_ != 0;
}

bool WellKnownChangePasswordState::SupportsChangePasswordUrl() const {
  DCHECK(BothRequestsFinished());
  return 200 <= change_password_response_code_ &&
         change_password_response_code_ < 300 &&
         non_existing_resource_response_code_ == net::HTTP_NOT_FOUND;
}

}  // namespace password_manager