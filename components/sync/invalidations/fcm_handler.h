// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_INVALIDATIONS_FCM_HANDLER_H_
#define COMPONENTS_SYNC_INVALIDATIONS_FCM_HANDLER_H_

#include <string>

#include "base/sequence_checker.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/keyed_service/core/keyed_service.h"

namespace gcm {
class GCMDriver;
}

namespace instance_id {
class InstanceIDDriver;
}

namespace syncer {

// This handler is used to register with FCM and to process incoming messages.
class FCMHandler : public gcm::GCMAppHandler {
 public:
  FCMHandler(gcm::GCMDriver* gcm_driver,
             instance_id::InstanceIDDriver* instance_id_driver,
             const std::string& sender_id,
             const std::string& app_id);
  ~FCMHandler() override;
  FCMHandler(const FCMHandler&) = delete;
  FCMHandler& operator=(const FCMHandler&) = delete;

  // Used to start handling incoming invalidations from the server and to obtain
  // an FCM token. Before StartListening() is called for the first time, the
  // FCM registration token will be empty.
  void StartListening();

  // Stop handling incoming invalidations. It doesn't cleanup the FCM
  // registration token and doesn't unsubscribe from FCM. All incoming
  // invalidations will be dropped.
  void StopListening();

  // Used to get an obtained FCM token. Returns empty string if it hasn't
  // received yet.
  const std::string& GetFCMRegistrationToken() const;

  // GCMAppHandler overrides.
  void ShutdownHandler() override;
  void OnStoreReset() override;
  void OnMessage(const std::string& app_id,
                 const gcm::IncomingMessage& message) override;
  void OnMessagesDeleted(const std::string& app_id) override;
  void OnSendError(const std::string& app_id,
                   const gcm::GCMClient::SendErrorDetails& details) override;
  void OnSendAcknowledged(const std::string& app_id,
                          const std::string& message_id) override;

 private:
  bool IsListening() const;

  // Called when a subscription token is obtained from the GCM server.
  void DidRetrieveToken(const std::string& subscription_token,
                        instance_id::InstanceID::Result result);

  SEQUENCE_CHECKER(sequence_checker_);

  gcm::GCMDriver* gcm_driver_ = nullptr;
  instance_id::InstanceIDDriver* instance_id_driver_ = nullptr;
  const std::string sender_id_;
  const std::string app_id_;

  // Contains an FCM registration token if not empty.
  std::string fcm_registration_token_;

  base::WeakPtrFactory<FCMHandler> weak_ptr_factory_{this};
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_INVALIDATIONS_FCM_HANDLER_H_
