// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/protobuf_http_request.h"

#include "remoting/base/protobuf_http_request_config.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/protobuf/src/google/protobuf/message_lite.h"

namespace remoting {

namespace {
constexpr int kMaxResponseSizeKb = 512;
}  // namespace

ProtobufHttpRequest::ProtobufHttpRequest(
    std::unique_ptr<ProtobufHttpRequestConfig> config)
    : ProtobufHttpRequestBase(std::move(config)) {}

ProtobufHttpRequest::~ProtobufHttpRequest() = default;

void ProtobufHttpRequest::SetTimeoutDuration(base::TimeDelta timeout_duration) {
  timeout_duration_ = timeout_duration;
}

void ProtobufHttpRequest::OnAuthFailed(const ProtobufHttpStatus& status) {
  std::move(response_callback_).Run(status);
}

void ProtobufHttpRequest::StartRequestInternal(
    network::mojom::URLLoaderFactory* loader_factory) {
  DCHECK(response_callback_);

  // Safe to use unretained as callback will not be called once |url_loader_| is
  // deleted.
  url_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&ProtobufHttpRequest::OnResponse, base::Unretained(this)),
      kMaxResponseSizeKb);
}

base::TimeDelta ProtobufHttpRequest::GetRequestTimeoutDuration() const {
  return timeout_duration_;
}

void ProtobufHttpRequest::OnResponse(
    std::unique_ptr<std::string> response_body) {
  ProtobufHttpStatus status = GetUrlLoaderStatus();
  std::move(response_callback_)
      .Run(status.ok() ? ParseResponse(std::move(response_body)) : status);
  std::move(invalidator_).Run();
}

ProtobufHttpStatus ProtobufHttpRequest::ParseResponse(
    std::unique_ptr<std::string> response_body) {
  if (!response_body) {
    LOG(ERROR) << "Server returned no response body";
    return ProtobufHttpStatus(net::ERR_EMPTY_RESPONSE);
  }
  if (!response_message_->ParseFromString(*response_body)) {
    LOG(ERROR) << "Failed to parse response body";
    return ProtobufHttpStatus(net::ERR_INVALID_RESPONSE);
  }
  return ProtobufHttpStatus::OK;
}

}  // namespace remoting
