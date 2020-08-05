// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A collection of names for the custom events to be emitted by the page's
// components (in vue_components/).
const CUSTOM_EVENTS = {
  DETAILS_CHECK_NODE: 'details-check-node',
  DETAILS_UNCHECK_NODE: 'details-uncheck-node',
  FILTER_REMOVE: 'filter-remove',
  FILTER_CHECK_ALL: 'filter-check-all',
  FILTER_UNCHECK_ALL: 'filter-uncheck-all',
  FILTER_SUBMITTED: 'filter-submitted',
  NODE_CLICKED: 'node-clicked',
};

export {
  CUSTOM_EVENTS,
};
