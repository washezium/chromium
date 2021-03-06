// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_browser_util.h"

#include "components/autofill/core/browser/autofill_client.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"

namespace {
// Matches the blink check for mixed content.
bool IsInsecureFormAction(const GURL& action_url) {
  if (action_url.SchemeIs(url::kBlobScheme) ||
      action_url.SchemeIs(url::kFileSystemScheme))
    return false;
  return !network::IsOriginPotentiallyTrustworthy(
      url::Origin::Create(action_url));
}
}  // namespace

namespace autofill {

bool IsFormOrClientNonSecure(AutofillClient* client, const FormData& form) {
  return !client->IsContextSecure() ||
         (form.action.is_valid() && form.action.SchemeIs("http"));
}

bool IsFormMixedContent(AutofillClient* client, const FormData& form) {
  return client->IsContextSecure() &&
         (form.action.is_valid() && IsInsecureFormAction(form.action));
}

bool ShouldAllowCreditCardFallbacks(AutofillClient* client,
                                    const FormData& form) {
  // Skip the form check if there wasn't a form yet:
  if (form.unique_renderer_id.is_null())
    return client->IsContextSecure();
  return !IsFormOrClientNonSecure(client, form);
}

}  // namespace autofill
