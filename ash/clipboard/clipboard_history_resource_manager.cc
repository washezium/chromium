// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/clipboard/clipboard_history_resource_manager.h"

#include <array>
#include <string>

#include "ash/public/cpp/clipboard_image_model_factory.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "base/bind.h"
#include "base/notreached.h"
#include "base/stl_util.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/clipboard/clipboard_data.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/strings/grit/ui_strings.h"

namespace ash {

namespace {

constexpr char kFileSystemSourcesType[] = "fs/sources";

// The array of formats in the order of decreasing priority.
constexpr std::array<ui::ClipboardInternalFormat, 7> kPrioritizedFormats = {
    ui::ClipboardInternalFormat::kBitmap,   ui::ClipboardInternalFormat::kText,
    ui::ClipboardInternalFormat::kHtml,     ui::ClipboardInternalFormat::kRtf,
    ui::ClipboardInternalFormat::kBookmark, ui::ClipboardInternalFormat::kWeb,
    ui::ClipboardInternalFormat::kCustom};

// Helpers ---------------------------------------------------------------------

// Returns true if |data| contains the specified |format|.
bool ContainsFormat(const ui::ClipboardData& data,
                    ui::ClipboardInternalFormat format) {
  return data.format() & static_cast<int>(format);
}

// Returns the localized string for the specified |resource_id|.
base::string16 GetLocalizedString(int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().GetLocalizedString(
      resource_id);
}

// TODO(crbug/1108902): Handle fallback case.
// Returns the label to display for the custom data contained within |data|.
base::string16 GetLabelForCustomData(const ui::ClipboardData& data) {
  DCHECK(ContainsFormat(data, ui::ClipboardInternalFormat::kCustom));

  // Attempt to read file system sources in the custom data.
  base::string16 sources;
  ui::ReadCustomDataForType(
      data.custom_data_data().c_str(), data.custom_data_data().size(),
      base::UTF8ToUTF16(kFileSystemSourcesType), &sources);

  if (sources.empty()) {
    // TODO(https://crbug.com/1119931): Move this to a grd file to make sure it
    // is internationalized.
    return base::UTF8ToUTF16("<Custom Data>");
  }

  // Split sources into a list.
  std::vector<base::StringPiece16> source_list =
      base::SplitStringPiece(sources, base::UTF8ToUTF16("\n"),
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // Strip path information, so all that's left are file names.
  for (auto it = source_list.begin(); it != source_list.end(); ++it)
    *it = it->substr(it->find_last_of(base::UTF8ToUTF16("/")) + 1);

  // Join file names, unescaping encoded character sequences for display. This
  // ensures that "My%20File.txt" will display as "My File.txt".
  return base::UTF8ToUTF16(base::UnescapeURLComponent(
      base::UTF16ToUTF8(base::JoinString(source_list, base::UTF8ToUTF16(", "))),
      base::UnescapeRule::SPACES));
}

}  // namespace

// ClipboardHistoryResourceManager ---------------------------------------------

ClipboardHistoryResourceManager::ClipboardHistoryResourceManager(
    const ClipboardHistory* clipboard_history)
    : clipboard_history_(clipboard_history) {
  clipboard_history_->AddObserver(this);
}

ClipboardHistoryResourceManager::~ClipboardHistoryResourceManager() {
  clipboard_history_->RemoveObserver(this);

  CancelUnfinishedRequests();
}

ui::ImageModel ClipboardHistoryResourceManager::GetImageModel(
    const ClipboardHistoryItem& item) const {
  // Use a cached image model when possible.
  auto cached_image_model = FindCachedImageModelForItem(item);
  if (cached_image_model != cached_image_models_.end())
    return cached_image_model->image_model;

  // TODO(newcomer): Show a smaller version of the bitmap.
  if (ContainsFormat(item.data(), ui::ClipboardInternalFormat::kBitmap))
    return ui::ImageModel();
  if (ContainsFormat(item.data(), ui::ClipboardInternalFormat::kWeb))
    return ui::ImageModel::FromVectorIcon(ash::kWebSmartPasteIcon);
  if (ContainsFormat(item.data(), ui::ClipboardInternalFormat::kBookmark))
    return ui::ImageModel::FromVectorIcon(ash::kWebBookmarkIcon);
  if (ContainsFormat(item.data(), ui::ClipboardInternalFormat::kHtml))
    return ui::ImageModel::FromVectorIcon(ash::kHtmlIcon);
  if (ContainsFormat(item.data(), ui::ClipboardInternalFormat::kRtf))
    return ui::ImageModel::FromVectorIcon(ash::kRtfIcon);
  if (ContainsFormat(item.data(), ui::ClipboardInternalFormat::kText))
    return ui::ImageModel::FromVectorIcon(ash::kTextIcon);
  // TODO(crbug/1108901): Handle file manager case.
  // TODO(crbug/1108902): Handle fallback case.
  if (ContainsFormat(item.data(), ui::ClipboardInternalFormat::kCustom))
    return ui::ImageModel();
  NOTREACHED();
  return ui::ImageModel();
}

base::string16 ClipboardHistoryResourceManager::GetLabel(
    const ClipboardHistoryItem& item) const {
  const ui::ClipboardData& data = item.data();
  switch (CalculateMainFormat(item)) {
    case ui::ClipboardInternalFormat::kBitmap:
      return GetLocalizedString(IDS_CLIPBOARD_MENU_IMAGE);
    case ui::ClipboardInternalFormat::kText:
      return base::UTF8ToUTF16(data.text());
    case ui::ClipboardInternalFormat::kHtml:
      return base::UTF8ToUTF16(data.markup_data());
    case ui::ClipboardInternalFormat::kRtf:
      return GetLocalizedString(IDS_CLIPBOARD_MENU_RTF_CONTENT);
    case ui::ClipboardInternalFormat::kBookmark:
      return base::UTF8ToUTF16(data.bookmark_title());
    case ui::ClipboardInternalFormat::kWeb:
      return GetLocalizedString(IDS_CLIPBOARD_MENU_WEB_SMART_PASTE);
    case ui::ClipboardInternalFormat::kCustom:
      return GetLabelForCustomData(data);
  }
}

ui::ClipboardInternalFormat
ClipboardHistoryResourceManager::CalculateMainFormat(
    const ClipboardHistoryItem& item) const {
  const ui::ClipboardData& data = item.data();
  for (const auto& format : kPrioritizedFormats) {
    if (ContainsFormat(data, format))
      return format;
  }

  NOTREACHED();
  return ui::ClipboardInternalFormat::kText;
}

ClipboardHistoryResourceManager::CachedImageModel::CachedImageModel() {}

ClipboardHistoryResourceManager::CachedImageModel::CachedImageModel(
    const CachedImageModel& other) = default;

ClipboardHistoryResourceManager::CachedImageModel&
ClipboardHistoryResourceManager::CachedImageModel::operator=(
    const CachedImageModel&) = default;

ClipboardHistoryResourceManager::CachedImageModel::~CachedImageModel() =
    default;

void ClipboardHistoryResourceManager::CacheImageModel(
    const base::UnguessableToken& id,
    ui::ImageModel image_model) {
  auto cached_image_model = base::ConstCastIterator(
      cached_image_models_, FindCachedImageModelForId(id));
  if (cached_image_model != cached_image_models_.end())
    cached_image_model->image_model = std::move(image_model);
}

std::vector<ClipboardHistoryResourceManager::CachedImageModel>::const_iterator
ClipboardHistoryResourceManager::FindCachedImageModelForId(
    const base::UnguessableToken& id) const {
  return std::find_if(cached_image_models_.cbegin(),
                      cached_image_models_.cend(),
                      [&](const auto& cached_image_model) {
                        return cached_image_model.id == id;
                      });
}

std::vector<ClipboardHistoryResourceManager::CachedImageModel>::const_iterator
ClipboardHistoryResourceManager::FindCachedImageModelForItem(
    const ClipboardHistoryItem& item) const {
  return std::find_if(
      cached_image_models_.cbegin(), cached_image_models_.cend(),
      [&](const auto& cached_image_model) {
        return base::Contains(cached_image_model.clipboard_history_item_ids,
                              item.id());
      });
}

void ClipboardHistoryResourceManager::CancelUnfinishedRequests() {
  for (const auto& cached_image_model : cached_image_models_) {
    if (cached_image_model.image_model.IsEmpty())
      ClipboardImageModelFactory::Get()->CancelRequest(cached_image_model.id);
  }
}

void ClipboardHistoryResourceManager::OnClipboardHistoryItemAdded(
    const ClipboardHistoryItem& item) {
  // For items that will be represented by their rendered HTML, we need to do
  // some prep work to pre-render and cache an image model.
  if (!item.data().bitmap().isNull() || item.data().markup_data().empty())
    return;

  const auto& items = clipboard_history_->GetItems();

  // See if we have an |existing| item that will render the same as |item|.
  auto it = std::find_if(items.begin(), items.end(), [&](const auto& existing) {
    return &existing != &item && existing.data().bitmap().isNull() &&
           existing.data().markup_data() == item.data().markup_data();
  });

  // If we don't have an existing image model in the cache, create one and
  // instruct ClipboardImageModelFactory to render it. Note that the factory may
  // or may not start rendering immediately depending on its activation status.
  if (it == items.end()) {
    base::UnguessableToken id = base::UnguessableToken::Create();
    CachedImageModel cached_image_model;
    cached_image_model.id = id;
    cached_image_model.clipboard_history_item_ids.push_back(item.id());
    cached_image_models_.push_back(std::move(cached_image_model));

    ClipboardImageModelFactory::Get()->Render(
        id, item.data().markup_data(),
        base::BindOnce(&ClipboardHistoryResourceManager::CacheImageModel,
                       weak_factory_.GetWeakPtr(), id));
    return;
  }
  // If we do have an existing model, we need only to update its usages.
  auto cached_image_model = base::ConstCastIterator(
      cached_image_models_, FindCachedImageModelForItem(*it));
  DCHECK(cached_image_model != cached_image_models_.end());
  cached_image_model->clipboard_history_item_ids.push_back(item.id());
}

void ClipboardHistoryResourceManager::OnClipboardHistoryItemRemoved(
    const ClipboardHistoryItem& item) {
  // For items that will not be represented by their rendered HTML, do nothing.
  if (!item.data().bitmap().isNull() || item.data().markup_data().empty())
    return;

  // We should have an image model in the cache.
  auto cached_image_model = base::ConstCastIterator(
      cached_image_models_, FindCachedImageModelForItem(item));

  DCHECK(cached_image_model != cached_image_models_.end());

  // Update usages.
  base::Erase(cached_image_model->clipboard_history_item_ids, item.id());
  if (!cached_image_model->clipboard_history_item_ids.empty())
    return;

  // If the ImageModel was never rendered, cancel the request.
  if (cached_image_model->image_model.IsEmpty())
    ClipboardImageModelFactory::Get()->CancelRequest(cached_image_model->id);

  // If the cached image model is no longer in use, it can be erased.
  cached_image_models_.erase(cached_image_model);
}

void ClipboardHistoryResourceManager::OnClipboardHistoryCleared() {
  CancelUnfinishedRequests();
  cached_image_models_ = std::vector<CachedImageModel>();
}

}  // namespace ash
