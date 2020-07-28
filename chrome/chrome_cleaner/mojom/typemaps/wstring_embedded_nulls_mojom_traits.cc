// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/chrome_cleaner/mojom/typemaps/wstring_embedded_nulls_mojom_traits.h"

#include "mojo/public/cpp/bindings/array_data_view.h"

namespace mojo {

using chrome_cleaner::WStringEmbeddedNulls;
using chrome_cleaner::mojom::NullValueDataView;
using chrome_cleaner::mojom::WStringEmbeddedNullsDataView;

// static
bool StructTraits<NullValueDataView, nullptr_t>::Read(NullValueDataView data,
                                                      nullptr_t* value) {
  *value = nullptr;
  return true;
}

// static
base::span<const uint16_t>
UnionTraits<WStringEmbeddedNullsDataView, WStringEmbeddedNulls>::value(
    const WStringEmbeddedNulls& str) {
  DCHECK_EQ(WStringEmbeddedNullsDataView::Tag::VALUE, GetTag(str));

  // This should only be called by Mojo to get the data to be send through the
  // pipe. When called by Mojo in this case, str will outlive the returned span.
  return base::make_span(str.CastAsUInt16Array(), str.size());
}

// static
nullptr_t
UnionTraits<WStringEmbeddedNullsDataView, WStringEmbeddedNulls>::null_value(
    const chrome_cleaner::WStringEmbeddedNulls& str) {
  DCHECK_EQ(WStringEmbeddedNullsDataView::Tag::NULL_VALUE, GetTag(str));

  return nullptr;
}

// static
chrome_cleaner::mojom::WStringEmbeddedNullsDataView::Tag
UnionTraits<WStringEmbeddedNullsDataView, WStringEmbeddedNulls>::GetTag(
    const chrome_cleaner::WStringEmbeddedNulls& str) {
  return str.size() == 0 ? WStringEmbeddedNullsDataView::Tag::NULL_VALUE
                         : WStringEmbeddedNullsDataView::Tag::VALUE;
}

// static
bool UnionTraits<WStringEmbeddedNullsDataView, WStringEmbeddedNulls>::Read(
    WStringEmbeddedNullsDataView str_view,
    WStringEmbeddedNulls* out) {
  if (str_view.is_null_value()) {
    *out = WStringEmbeddedNulls();
    return true;
  }

  ArrayDataView<uint16_t> view;
  str_view.GetValueDataView(&view);
  // Note: Casting is intentional, since the data view represents the string as
  //       a uint16_t array, whereas WStringEmbeddedNulls's constructor expects
  //       a wchar_t array.
  *out = WStringEmbeddedNulls(reinterpret_cast<const wchar_t*>(view.data()),
                              view.size());
  return true;
}

}  // namespace mojo
