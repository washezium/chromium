// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/task/post_task.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "chromeos/printing/ppd_cache.h"
#include "chromeos/printing/ppd_metadata_manager.h"
#include "chromeos/printing/ppd_provider_v3.h"
#include "chromeos/printing/printer_config_cache.h"

namespace chromeos {
namespace {

// This class implements the PpdProvider interface for the v3 metadata
// (https://crbug.com/888189).

class PpdProviderImpl : public PpdProvider {
 public:
  PpdProviderImpl(base::StringPiece browser_locale,
                  const base::Version& current_version,
                  scoped_refptr<PpdCache> cache,
                  std::unique_ptr<PpdMetadataManager> metadata_manager,
                  std::unique_ptr<PrinterConfigCache> config_cache)
      : browser_locale_(std::string(browser_locale)),
        version_(current_version),
        cache_(cache),
        metadata_manager_(std::move(metadata_manager)),
        config_cache_(std::move(config_cache)),
        file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
            {base::TaskPriority::USER_VISIBLE, base::MayBlock(),
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {}

  void ResolveManufacturers(ResolveManufacturersCallback cb) override {
    // TODO(crbug.com/888189): implement this.
  }

  void ResolvePrinters(const std::string& manufacturer,
                       ResolvePrintersCallback cb) override {
    // TODO(crbug.com/888189): implement this.
  }

  void ResolvePpdReference(const PrinterSearchData& search_data,
                           ResolvePpdReferenceCallback cb) override {
    // TODO(crbug.com/888189): implement this.
  }

  void ResolvePpd(const Printer::PpdReference& reference,
                  ResolvePpdCallback cb) override {
    // TODO(crbug.com/888189): implement this.
  }

  void ReverseLookup(const std::string& effective_make_and_model,
                     ReverseLookupCallback cb) override {
    // TODO(crbug.com/888189): implement this.
  }

  void ResolvePpdLicense(base::StringPiece effective_make_and_model,
                         ResolvePpdLicenseCallback cb) override {
    // TODO(crbug.com/888189): implement this.
  }

 protected:
  ~PpdProviderImpl() override = default;

 private:
  // Locale of the browser, as returned by
  // BrowserContext::GetApplicationLocale();
  const std::string browser_locale_;

  // Current version used to filter restricted ppds
  const base::Version version_;

  // Provides PPD storage on-device.
  scoped_refptr<PpdCache> cache_;

  // Interacts with and controls PPD metadata.
  std::unique_ptr<PpdMetadataManager> metadata_manager_;

  // Fetches PPDs from the Chrome OS Printing team's serving root.
  std::unique_ptr<PrinterConfigCache> config_cache_;

  // Where to run disk operations.
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  base::WeakPtrFactory<PpdProviderImpl> weak_factory_{this};
};

}  // namespace

PrinterSearchData::PrinterSearchData() = default;
PrinterSearchData::PrinterSearchData(const PrinterSearchData& other) = default;
PrinterSearchData::~PrinterSearchData() = default;

// static
scoped_refptr<PpdProvider> PpdProvider::Create(
    const std::string& browser_locale,
    network::mojom::URLLoaderFactory* loader_factory,
    scoped_refptr<PpdCache> ppd_cache,
    const base::Version& current_version,
    const PpdProvider::Options& options) {
  NOTREACHED();  // TODO(crbug.com/888189): deprecate this Create().
  return nullptr;
}

// free function; _not_ static
scoped_refptr<PpdProvider> CreateV3Provider(
    base::StringPiece browser_locale,
    const base::Version& current_version,
    scoped_refptr<PpdCache> cache,
    std::unique_ptr<PpdMetadataManager> metadata_manager,
    std::unique_ptr<PrinterConfigCache> config_cache) {
  return base::MakeRefCounted<PpdProviderImpl>(
      browser_locale, current_version, cache, std::move(metadata_manager),
      std::move(config_cache));
}

}  // namespace chromeos
