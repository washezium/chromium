// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_service/file_utils.h"

#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "storage/browser/file_system/file_system_context.h"
#include "storage/browser/file_system/file_system_url.h"
#include "url/gurl.h"

namespace apps {

std::vector<storage::FileSystemURL> GetFileSystemURL(
    Profile* profile,
    const std::vector<GURL>& file_urls) {
  storage::FileSystemContext* file_system_context =
      file_manager::util::GetFileSystemContextForExtensionId(
          profile, file_manager::kFileManagerAppId);

  std::vector<storage::FileSystemURL> file_system_urls;
  for (const GURL& file_url : file_urls) {
    file_system_urls.push_back(file_system_context->CrackURL(file_url));
  }
  return file_system_urls;
}

}  // namespace apps
