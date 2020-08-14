// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NEARBY_SHARING_CERTIFICATES_NEARBY_SHARE_CERTIFICATE_MANAGER_IMPL_H_
#define CHROME_BROWSER_NEARBY_SHARING_CERTIFICATES_NEARBY_SHARE_CERTIFICATE_MANAGER_IMPL_H_

#include <memory>
#include <vector>

#include "chrome/browser/nearby_sharing/certificates/nearby_share_certificate_manager.h"
#include "chrome/browser/nearby_sharing/certificates/nearby_share_encrypted_metadata_key.h"
#include "chrome/browser/nearby_sharing/certificates/nearby_share_private_certificate.h"
#include "chrome/browser/nearby_sharing/certificates/nearby_share_visibility.h"
#include "chrome/browser/nearby_sharing/proto/rpc_resources.pb.h"

// TODO(nohle): Add description after class is fully implemented.
class NearbyShareCertificateManagerImpl : public NearbyShareCertificateManager {
 public:
  class Factory {
   public:
    static std::unique_ptr<NearbyShareCertificateManager> Create();
    static void SetFactoryForTesting(Factory* test_factory);

   protected:
    virtual ~Factory();
    virtual std::unique_ptr<NearbyShareCertificateManager> CreateInstance() = 0;

   private:
    static Factory* test_factory_;
  };

  ~NearbyShareCertificateManagerImpl() override;

 private:
  NearbyShareCertificateManagerImpl();

  // NearbyShareCertificateManager:
  NearbySharePrivateCertificate GetValidPrivateCertificate(
      NearbyShareVisibility visibility) override;
  std::vector<nearbyshare::proto::PublicCertificate>
  GetPrivateCertificatesAsPublicCertificates(
      NearbyShareVisibility visibility) override;
  void GetDecryptedPublicCertificate(
      NearbyShareEncryptedMetadataKey encrypted_metadata_key,
      CertDecryptedCallback callback) override;
  void DownloadPublicCertificates() override;
  void OnStart() override;
  void OnStop() override;
};

#endif  // CHROME_BROWSER_NEARBY_SHARING_CERTIFICATES_NEARBY_SHARE_CERTIFICATE_MANAGER_IMPL_H_
