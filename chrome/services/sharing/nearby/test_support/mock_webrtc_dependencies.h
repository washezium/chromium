// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_SHARING_NEARBY_TEST_SUPPORT_MOCK_WEBRTC_DEPENDENCIES_H_
#define CHROME_SERVICES_SHARING_NEARBY_TEST_SUPPORT_MOCK_WEBRTC_DEPENDENCIES_H_

#include "chrome/services/sharing/public/mojom/webrtc.mojom.h"
#include "chrome/services/sharing/public/mojom/webrtc_signaling_messenger.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/mojom/mdns_responder.mojom.h"
#include "services/network/public/mojom/p2p.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace sharing {

// Mimics browser process and network service implementations.
class MockWebRtcDependencies : public network::mojom::P2PSocketManager,
                               public network::mojom::MdnsResponder,
                               public sharing::mojom::IceConfigFetcher,
                               public sharing::mojom::WebRtcSignalingMessenger {
 public:
  MockWebRtcDependencies();
  ~MockWebRtcDependencies() override;

  // network::mojom::P2PSocketManager overrides:
  MOCK_METHOD(void,
              StartNetworkNotifications,
              (mojo::PendingRemote<network::mojom::P2PNetworkNotificationClient>
                   client),
              (override));
  MOCK_METHOD(
      void,
      GetHostAddress,
      (const std::string& host_name,
       bool enable_mdns,
       network::mojom::P2PSocketManager::GetHostAddressCallback callback),
      (override));
  MOCK_METHOD(void,
              CreateSocket,
              (network::P2PSocketType type,
               const net::IPEndPoint& local_address,
               const network::P2PPortRange& port_range,
               const network::P2PHostAndIPEndPoint& remote_address,
               mojo::PendingRemote<network::mojom::P2PSocketClient> client,
               mojo::PendingReceiver<network::mojom::P2PSocket> receiver),
              (override));

  // network::mojom::MdnsResponder overrides:
  MOCK_METHOD(
      void,
      CreateNameForAddress,
      (const net::IPAddress& address,
       network::mojom::MdnsResponder::CreateNameForAddressCallback callback),
      (override));
  MOCK_METHOD(
      void,
      RemoveNameForAddress,
      (const net::IPAddress& address,
       network::mojom::MdnsResponder::RemoveNameForAddressCallback callback),
      (override));

  // sharing::mojom::IceConfigFetcher overrides:
  MOCK_METHOD(void,
              GetIceServers,
              (GetIceServersCallback callback),
              (override));

  // sharing::mojom::WebRtcSignalingMessenger overrides:
  MOCK_METHOD(void,
              SendMessage,
              (const std::string& self_id,
               const std::string& peer_id,
               const std::string& message,
               SendMessageCallback callback),
              (override));

  MOCK_METHOD(void,
              StartReceivingMessages,
              (const std::string& self_id,
               mojo::PendingRemote<sharing::mojom::IncomingMessagesListener>
                   incoming_messages_listener,
               StartReceivingMessagesCallback callback),
              (override));
  MOCK_METHOD(void, StopReceivingMessages, (), (override));

  mojo::Receiver<network::mojom::P2PSocketManager> socket_manager_{this};
  mojo::Receiver<network::mojom::MdnsResponder> mdns_responder_{this};
  mojo::Receiver<sharing::mojom::IceConfigFetcher> ice_config_fetcher_{this};
  mojo::Receiver<sharing::mojom::WebRtcSignalingMessenger> messenger_{this};
};

}  // namespace sharing

#endif  // CHROME_SERVICES_SHARING_NEARBY_TEST_SUPPORT_MOCK_WEBRTC_DEPENDENCIES_H_
