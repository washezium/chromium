// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @type {?Promise<void>} */
let lazyLoadPromise = null;

/** @return {!Promise<void>} Resolves when the lazy load module is imported. */
export function ensureLazyLoaded() {
  if (!lazyLoadPromise) {
    const script = document.createElement('script');
    script.type = 'module';
    script.src = './profile_creation_flow/lazy_load.js';
    document.body.appendChild(script);

    lazyLoadPromise = Promise.all([
      'profile-type-choice',
    ].map(name => customElements.whenDefined(name)));
  }
  return lazyLoadPromise;
}
