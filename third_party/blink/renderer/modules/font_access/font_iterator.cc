// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/font_access/font_iterator.h"

#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_throw_dom_exception.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_font_iterator_entry.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/modules/font_access/font_metadata.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

FontIterator::FontIterator(ExecutionContext* context)
    : ExecutionContextLifecycleObserver(context) {
  context->GetBrowserInterfaceBroker().GetInterface(
      remote_manager_.BindNewPipeAndPassReceiver());
  remote_manager_.set_disconnect_handler(
      WTF::Bind(&FontIterator::OnDisconnect, WrapWeakPersistent(this)));
}

ScriptPromise FontIterator::next(ScriptState* script_state) {
  if (permission_status_ == PermissionStatus::ASK) {
    if (!pending_resolver_) {
      remote_manager_->RequestPermission(WTF::Bind(
          &FontIterator::DidGetPermissionResponse, WrapWeakPersistent(this)));
      pending_resolver_ =
          MakeGarbageCollected<ScriptPromiseResolver>(script_state);
    }
    return pending_resolver_->Promise();
  }

  if (permission_status_ == PermissionStatus::DENIED) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        MakeGarbageCollected<DOMException>(DOMExceptionCode::kNotAllowedError,
                                           "Permission Error"));
  }

  return ScriptPromise::Cast(script_state, ToV8(GetNextEntry(), script_state));
}

void FontIterator::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
  visitor->Trace(entries_);
  visitor->Trace(pending_resolver_);
}

FontIteratorEntry* FontIterator::GetNextEntry() {
  auto* result = FontIteratorEntry::Create();
  if (entries_.IsEmpty()) {
    result->setDone(true);
    return result;
  }

  FontMetadata* entry = entries_.TakeFirst();
  result->setValue(entry);

  return result;
}

void FontIterator::DidGetPermissionResponse(PermissionStatus status) {
  permission_status_ = status;

  if (permission_status_ != PermissionStatus::GRANTED) {
    pending_resolver_->Reject(MakeGarbageCollected<DOMException>(
        DOMExceptionCode::kNotAllowedError, "Permission Error"));
    pending_resolver_.Clear();
    return;
  }

  FontCache* font_cache = FontCache::GetFontCache();
  auto metadata = font_cache->EnumerateAvailableFonts();
  for (const auto& entry : metadata) {
    entries_.push_back(FontMetadata::Create(entry));
  }

  pending_resolver_->Resolve(GetNextEntry());
  pending_resolver_.Clear();
}

void FontIterator::ContextDestroyed() {
  remote_manager_.reset();
}

void FontIterator::OnDisconnect() {
  remote_manager_.reset();
}

}  // namespace blink
