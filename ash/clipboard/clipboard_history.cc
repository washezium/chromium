// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/clipboard/clipboard_history.h"

#include "ash/clipboard/clipboard_history_controller.h"
#include "ash/session/session_controller_impl.h"
#include "ash/shell.h"
#include "base/stl_util.h"
#include "components/account_id/account_id.h"
#include "ui/base/clipboard/clipboard_monitor.h"
#include "ui/base/clipboard/clipboard_non_backed.h"

namespace ash {

namespace {

constexpr size_t kMaxClipboardItemsShared = 5;

AccountId GetActiveAccountId() {
  return Shell::Get()->session_controller()->GetActiveAccountId();
}

}  // namespace

ClipboardHistory::ScopedPause::ScopedPause(ClipboardHistory* clipboard_history)
    : clipboard_history_(clipboard_history) {
  clipboard_history_->PauseClipboardHistory();
}

ClipboardHistory::ScopedPause::~ScopedPause() {
  clipboard_history_->UnPauseClipboardHistory();
}

ClipboardHistory::ClipboardHistory() {
  ui::ClipboardMonitor::GetInstance()->AddObserver(this);
  Shell::Get()->session_controller()->AddObserver(this);
  Shell::Get()->AddShellObserver(this);
}

ClipboardHistory::~ClipboardHistory() {
  ui::ClipboardMonitor::GetInstance()->RemoveObserver(this);
}

void ClipboardHistory::ClearHistory() {
  history_with_duplicates_mappings_[GetActiveAccountId()] =
      std::deque<ui::ClipboardData>();
}

std::vector<ui::ClipboardData>
ClipboardHistory::GetRecentClipboardDataWithNoDuplicates() const {
  std::vector<ui::ClipboardData> history_without_duplicates;
  const std::deque<ui::ClipboardData>& history_with_duplicates =
      history_with_duplicates_mappings_.find(GetActiveAccountId())->second;
  for (auto it = history_with_duplicates.crbegin();
       it != history_with_duplicates.crend(); ++it) {
    if (history_without_duplicates.size() == kMaxClipboardItemsShared)
      break;
    const ui::ClipboardData& current_data = *it;
    if (!base::Contains(history_without_duplicates, current_data))
      history_without_duplicates.push_back(current_data);
  }
  return history_without_duplicates;
}

bool ClipboardHistory::IsEmpty() const {
  auto it = history_with_duplicates_mappings_.find(GetActiveAccountId());
  DCHECK(it != history_with_duplicates_mappings_.cend());
  return it->second.empty();
}

void ClipboardHistory::OnClipboardDataChanged() {
  // TODO(newcomer): Prevent Clipboard from recording metrics when pausing
  // observation.
  if (num_pause_clipboard_history_ > 0)
    return;

  auto* clipboard = ui::ClipboardNonBacked::GetForCurrentThread();
  CHECK(clipboard);

  const auto* clipboard_data = clipboard->GetClipboardData();
  CHECK(clipboard_data);

  CommitData(GetActiveAccountId(), ui::ClipboardData(*clipboard_data));
}

void ClipboardHistory::CommitData(const AccountId& account_id,
                                  ui::ClipboardData data) {
  const AccountId active_account_id = GetActiveAccountId();

  // CommitData() is called asynchronously when handling bitmap data.
  // Theoretically the active account may switch before CommitData() is handled.
  // Return early in this edge case.
  if (account_id != active_account_id)
    return;

  DCHECK(active_account_id.is_valid());

  std::deque<ui::ClipboardData>& history_with_duplicates =
      history_with_duplicates_mappings_[active_account_id];

  // Keep duplicates to maintain most recent ordering.
  history_with_duplicates.push_back(std::move(data));

  // Keep the history with duplicates to a reasonable limit, but greater than
  // kMaxClipboardItemsShared, so that repeated copies to not push off relevant
  // data.
  if (history_with_duplicates.size() > kMaxClipboardItemsShared * 2)
    history_with_duplicates.pop_front();
}

void ClipboardHistory::PauseClipboardHistory() {
  ++num_pause_clipboard_history_;
}

void ClipboardHistory::UnPauseClipboardHistory() {
  --num_pause_clipboard_history_;
}

void ClipboardHistory::OnActiveUserSessionChanged(
    const AccountId& active_account_id) {
  if (!base::Contains(history_with_duplicates_mappings_, active_account_id)) {
    history_with_duplicates_mappings_[active_account_id] =
        std::deque<ui::ClipboardData>();
  }
}

void ClipboardHistory::OnShellDestroying() {
  // ClipboardHistory depends on Shell to access the classes it observes. So
  // remove itself from observer lists before the Shell instance is destroyed.
  Shell::Get()->session_controller()->RemoveObserver(this);
  Shell::Get()->RemoveShellObserver(this);
}

}  // namespace ash
