// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/sharing/nearby/nearby_connections.h"

#include "base/run_loop.h"
#include "third_party/nearby/src/cpp/core_v2/core.h"

namespace location {
namespace nearby {
namespace connections {

// Should only be accessed by objects within lifetime of NearbyConnections.
NearbyConnections* g_instance = nullptr;

// static
NearbyConnections& NearbyConnections::GetInstance() {
  DCHECK(g_instance);
  return *g_instance;
}

NearbyConnections::NearbyConnections(
    mojo::PendingReceiver<mojom::NearbyConnections> nearby_connections,
    mojom::NearbyConnectionsDependenciesPtr dependencies,
    base::OnceClosure on_disconnect)
    : nearby_connections_(this, std::move(nearby_connections)),
      on_disconnect_(std::move(on_disconnect)) {
  nearby_connections_.set_disconnect_handler(base::BindOnce(
      &NearbyConnections::OnDisconnect, weak_ptr_factory_.GetWeakPtr()));

  if (dependencies->bluetooth_adapter) {
    bluetooth_adapter_.Bind(std::move(dependencies->bluetooth_adapter));
    bluetooth_adapter_.set_disconnect_handler(base::BindOnce(
        &NearbyConnections::OnDisconnect, weak_ptr_factory_.GetWeakPtr()));
  }

  socket_manager_.Bind(
      std::move(dependencies->webrtc_dependencies->socket_manager));
  socket_manager_.set_disconnect_handler(base::BindOnce(
      &NearbyConnections::OnDisconnect, weak_ptr_factory_.GetWeakPtr()));

  mdns_responder_.Bind(
      std::move(dependencies->webrtc_dependencies->mdns_responder));
  mdns_responder_.set_disconnect_handler(base::BindOnce(
      &NearbyConnections::OnDisconnect, weak_ptr_factory_.GetWeakPtr()));

  ice_config_fetcher_.Bind(
      std::move(dependencies->webrtc_dependencies->ice_config_fetcher));
  ice_config_fetcher_.set_disconnect_handler(base::BindOnce(
      &NearbyConnections::OnDisconnect, weak_ptr_factory_.GetWeakPtr()));

  webrtc_signaling_messenger_.Bind(
      std::move(dependencies->webrtc_dependencies->messenger));
  webrtc_signaling_messenger_.set_disconnect_handler(base::BindOnce(
      &NearbyConnections::OnDisconnect, weak_ptr_factory_.GetWeakPtr()));

  // There should only be one instance of NearbyConnections in a process.
  DCHECK(!g_instance);
  g_instance = this;
  core_ = std::make_unique<Core>();
}

NearbyConnections::~NearbyConnections() {
  core_.reset();
  g_instance = nullptr;
}

void NearbyConnections::OnDisconnect() {
  if (on_disconnect_)
    std::move(on_disconnect_).Run();
  // Note: |this| might be destroyed here.
}

bluetooth::mojom::Adapter* NearbyConnections::GetBluetoothAdapter() {
  if (!bluetooth_adapter_.is_bound())
    return nullptr;

  return bluetooth_adapter_.get();
}

network::mojom::P2PSocketManager*
NearbyConnections::GetWebRtcP2PSocketManager() {
  if (!socket_manager_.is_bound())
    return nullptr;

  return socket_manager_.get();
}

network::mojom::MdnsResponder* NearbyConnections::GetWebRtcMdnsResponder() {
  if (!mdns_responder_.is_bound())
    return nullptr;

  return mdns_responder_.get();
}

sharing::mojom::IceConfigFetcher*
NearbyConnections::GetWebRtcIceConfigFetcher() {
  if (!ice_config_fetcher_.is_bound())
    return nullptr;

  return ice_config_fetcher_.get();
}

sharing::mojom::WebRtcSignalingMessenger*
NearbyConnections::GetWebRtcSignalingMessenger() {
  if (!webrtc_signaling_messenger_.is_bound())
    return nullptr;

  return webrtc_signaling_messenger_.get();
}

}  // namespace connections
}  // namespace nearby
}  // namespace location
