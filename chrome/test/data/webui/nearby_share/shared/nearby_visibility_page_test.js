// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
// #import 'chrome://nearby/shared/nearby_visibility_page.m.js';
// #import {setNearbyShareSettingsForTesting} from 'chrome://nearby/shared/nearby_share_settings.m.js';
// #import {FakeNearbyShareSettings} from './fake_nearby_share_settings.m.js';
// #import {assertEquals, assertTrue, assertFalse} from '../../chai_assert.js';
// clang-format on

suite('nearby-visibility-page', function() {
  /** @type {!NearbyVisibilityPageElement} */
  let visibility_page;

  setup(function() {
    document.body.innerHTML = '';

    visibility_page = /** @type {!NearbyVisibilityPageElement} */ (
        document.createElement('nearby-visibility-page'));
    visibility_page.settings = {
      enabled: false,
      deviceName: 'deviceName',
      dataUsage: 1,
      visibility: 1,
      allowedContacts: [],
    };
    document.body.appendChild(visibility_page);
  });

  test('Renders visibility page', async function() {
    assertFalse(visibility_page.settings.enabled);
    // Next button sets settings.enabled to true
    visibility_page.$$('#next-button').click();
    assertTrue(visibility_page.settings.enabled);
  });
});
