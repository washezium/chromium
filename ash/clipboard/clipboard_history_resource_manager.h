// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_CLIPBOARD_CLIPBOARD_HISTORY_RESOURCE_MANAGER_H_
#define ASH_CLIPBOARD_CLIPBOARD_HISTORY_RESOURCE_MANAGER_H_

#include <vector>

#include "ash/ash_export.h"
#include "ash/clipboard/clipboard_history.h"
#include "ash/clipboard/clipboard_history_item.h"
#include "base/strings/string16.h"
#include "base/unguessable_token.h"
#include "ui/base/models/image_model.h"

namespace ash {

class ASH_EXPORT ClipboardHistoryResourceManager
    : public ClipboardHistory::Observer {
 public:
  explicit ClipboardHistoryResourceManager(
      const ClipboardHistory* clipboard_history);
  ClipboardHistoryResourceManager(const ClipboardHistoryResourceManager&) =
      delete;
  ClipboardHistoryResourceManager& operator=(
      const ClipboardHistoryResourceManager&) = delete;
  ~ClipboardHistoryResourceManager() override;

  // Returns the image to display for the specified clipboard history |item|.
  ui::ImageModel GetImageModel(const ClipboardHistoryItem& item) const;

  // Returns the label to display for the specified clipboard history |item|.
  base::string16 GetLabel(const ClipboardHistoryItem& item) const;

  // Returns the main format of the specified clipboard history |item|. Note
  // that one `ClipboardHistoryItem` instance may own multiple formats.
  ui::ClipboardInternalFormat CalculateMainFormat(
      const ClipboardHistoryItem& item) const;

 private:
  struct CachedImageModel {
    CachedImageModel();
    CachedImageModel(const CachedImageModel&);
    CachedImageModel& operator=(const CachedImageModel&);
    ~CachedImageModel();
    // Unique identifier.
    base::UnguessableToken id;
    // ImageModel that was created by ClipboardImageModelFactory.
    ui::ImageModel image_model;
    // ClipboardHistoryItem id's which utilize this CachedImageModel.
    std::vector<base::UnguessableToken> clipboard_history_item_ids;
  };

  // Caches the specified |image_model| with the specified |id|.
  void CacheImageModel(const base::UnguessableToken& id,
                       ui::ImageModel image_model);

  // Finds the cached image model associated with the specified |id|.
  std::vector<ClipboardHistoryResourceManager::CachedImageModel>::const_iterator
  FindCachedImageModelForId(const base::UnguessableToken& id) const;

  // Finds the cached image model associated with the specified |item|.
  std::vector<ClipboardHistoryResourceManager::CachedImageModel>::const_iterator
  FindCachedImageModelForItem(const ClipboardHistoryItem& item) const;

  // Cancels all unfinished requests.
  void CancelUnfinishedRequests();

  // ClipboardHistory::Observer:
  void OnClipboardHistoryItemAdded(const ClipboardHistoryItem& item) override;
  void OnClipboardHistoryItemRemoved(const ClipboardHistoryItem& item) override;
  void OnClipboardHistoryCleared() override;

  // Owned by ClipboardHistoryController.
  const ClipboardHistory* const clipboard_history_;

  std::vector<CachedImageModel> cached_image_models_;

  base::WeakPtrFactory<ClipboardHistoryResourceManager> weak_factory_{this};
};

}  // namespace ash

#endif  // ASH_CLIPBOARD_CLIPBOARD_HISTORY_RESOURCE_MANAGER_H_
