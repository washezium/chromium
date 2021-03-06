// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_SECURE_PAYMENT_CONFIRMATION_APP_H_
#define COMPONENTS_PAYMENTS_CONTENT_SECURE_PAYMENT_CONFIRMATION_APP_H_

#include "components/payments/content/payment_app.h"

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "components/payments/content/secure_payment_confirmation_controller.h"
#include "third_party/blink/public/mojom/payments/payment_request.mojom.h"
#include "third_party/blink/public/mojom/webauthn/authenticator.mojom.h"
#include "url/origin.h"

class SkBitmap;

namespace autofill {
class InternalAuthenticator;
}  // namespace autofill

namespace payments {

class SecurePaymentConfirmationApp : public PaymentApp {
 public:
  SecurePaymentConfirmationApp(
      const std::string& effective_relying_party_identity,
      std::unique_ptr<SkBitmap> icon,
      const base::string16& label,
      std::vector<std::unique_ptr<std::vector<uint8_t>>> credential_ids,
      const url::Origin& merchant_origin,
      const mojom::PaymentCurrencyAmountPtr& total,
      mojom::SecurePaymentConfirmationRequestPtr request,
      std::unique_ptr<autofill::InternalAuthenticator> authenticator);
  ~SecurePaymentConfirmationApp() override;

  SecurePaymentConfirmationApp(const SecurePaymentConfirmationApp& other) =
      delete;
  SecurePaymentConfirmationApp& operator=(
      const SecurePaymentConfirmationApp& other) = delete;

  // PaymentApp implementation.
  void InvokePaymentApp(Delegate* delegate) override;
  bool IsCompleteForPayment() const override;
  uint32_t GetCompletenessScore() const override;
  bool CanPreselect() const override;
  base::string16 GetMissingInfoLabel() const override;
  bool HasEnrolledInstrument() const override;
  void RecordUse() override;
  bool NeedsInstallation() const override;
  std::string GetId() const override;
  base::string16 GetLabel() const override;
  base::string16 GetSublabel() const override;
  const SkBitmap* icon_bitmap() const override;
  bool IsValidForModifier(
      const std::string& method,
      bool supported_networks_specified,
      const std::set<std::string>& supported_networks) const override;
  base::WeakPtr<PaymentApp> AsWeakPtr() override;
  bool HandlesShippingAddress() const override;
  bool HandlesPayerName() const override;
  bool HandlesPayerEmail() const override;
  bool HandlesPayerPhone() const override;
  bool IsWaitingForPaymentDetailsUpdate() const override;
  void UpdateWith(
      mojom::PaymentRequestDetailsUpdatePtr details_update) override;
  void OnPaymentDetailsNotUpdated() override;
  void AbortPaymentApp(base::OnceCallback<void(bool)> abort_callback) override;

 private:
  void OnGetAssertion(
      Delegate* delegate,
      blink::mojom::AuthenticatorStatus status,
      blink::mojom::GetAssertionAuthenticatorResponsePtr response);

  const std::string effective_relying_party_identity_;
  const std::unique_ptr<SkBitmap> icon_;
  const base::string16 label_;
  const std::vector<std::unique_ptr<std::vector<uint8_t>>> credential_ids_;
  const url::Origin merchant_origin_;
  const mojom::PaymentCurrencyAmountPtr total_;
  const mojom::SecurePaymentConfirmationRequestPtr request_;
  const std::unique_ptr<autofill::InternalAuthenticator> authenticator_;

  base::WeakPtrFactory<SecurePaymentConfirmationApp> weak_ptr_factory_{this};
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_SECURE_PAYMENT_CONFIRMATION_APP_H_
