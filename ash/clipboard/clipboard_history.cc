// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/clipboard/clipboard_history.h"

#include "ash/clipboard/clipboard_history_util.h"
#include "base/stl_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "ui/base/clipboard/clipboard_data_endpoint.h"
#include "ui/base/clipboard/clipboard_monitor.h"
#include "ui/base/clipboard/clipboard_non_backed.h"

namespace ash {

namespace {

constexpr size_t kMaxClipboardItemsShared = 5;

}  // namespace

ClipboardHistory::ScopedPause::ScopedPause(ClipboardHistory* clipboard_history)
    : clipboard_history_(clipboard_history) {
  clipboard_history_->Pause();
}

ClipboardHistory::ScopedPause::~ScopedPause() {
  clipboard_history_->Resume();
}

ClipboardHistory::ClipboardHistory() {
  ui::ClipboardMonitor::GetInstance()->AddObserver(this);
}

ClipboardHistory::~ClipboardHistory() {
  ui::ClipboardMonitor::GetInstance()->RemoveObserver(this);
}

void ClipboardHistory::AddObserver(Observer* observer) const {
  observers_.AddObserver(observer);
}

void ClipboardHistory::RemoveObserver(Observer* observer) const {
  observers_.RemoveObserver(observer);
}

const std::list<ClipboardHistoryItem>& ClipboardHistory::GetItems() const {
  return history_list_;
}

void ClipboardHistory::Clear() {
  history_list_ = std::list<ClipboardHistoryItem>();
  for (auto& observer : observers_)
    observer.OnClipboardHistoryCleared();
}

bool ClipboardHistory::IsEmpty() const {
  return GetItems().empty();
}

void ClipboardHistory::OnClipboardDataChanged() {
  // TODO(newcomer): Prevent Clipboard from recording metrics when pausing
  // observation.
  if (num_pause_ > 0)
    return;

  auto* clipboard = ui::ClipboardNonBacked::GetForCurrentThread();
  CHECK(clipboard);

  ui::ClipboardDataEndpoint data_dst(ui::EndpointType::kClipboardHistory);
  const auto* clipboard_data = clipboard->GetClipboardData(&data_dst);
  CHECK(clipboard_data);

  // We post commit |clipboard_data| at the end of the current task sequence to
  // debounce the case where multiple copies are programmatically performed.
  // Since only the most recent copy will be at the top of the clipboard, the
  // user will likely be unaware of the intermediate copies that took place
  // opaquely in the same task sequence and would be confused to see them in
  // history. A real world example would be copying the URL from the address bar
  // in the browser. First a short form of the URL is copied, followed
  // immediately by the long form URL.
  commit_data_weak_factory_.InvalidateWeakPtrs();
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&ClipboardHistory::MaybeCommitData,
                     commit_data_weak_factory_.GetWeakPtr(), *clipboard_data));
}

void ClipboardHistory::MaybeCommitData(ui::ClipboardData data) {
  if (!ClipboardHistoryUtil::IsSupported(data))
    return;

  history_list_.emplace_front(std::move(data));
  for (auto& observer : observers_)
    observer.OnClipboardHistoryItemAdded(history_list_.front());

  if (history_list_.size() > kMaxClipboardItemsShared) {
    auto removed = std::move(history_list_.back());
    history_list_.pop_back();
    for (auto& observer : observers_)
      observer.OnClipboardHistoryItemRemoved(removed);
  }
}

void ClipboardHistory::Pause() {
  ++num_pause_;
}

void ClipboardHistory::Resume() {
  --num_pause_;
}

}  // namespace ash
