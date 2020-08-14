// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://profile-picker/profile_picker_app.js';

import {ManageProfilesBrowserProxyImpl} from 'chrome://profile-picker/manage_profiles_browser_proxy.js';
import {navigateTo, Routes} from 'chrome://profile-picker/navigation_behavior.js';
import {ensureLazyLoaded} from 'chrome://profile-picker/profile_creation_flow/ensure_lazy_loaded.js';

import {assertTrue} from '../chai_assert.js';
import {waitBeforeNextRender} from '../test_util.m.js';

import {TestManageProfilesBrowserProxy} from './test_manage_profiles_browser_proxy.js';

suite('ProfilePickerAppTest', function() {
  /** @type {!ProfilePickerAppElement} */
  let app;

  /** @type {!TestManageProfilesBrowserProxy} */
  let browserProxy;

  setup(function() {
    browserProxy = new TestManageProfilesBrowserProxy();
    ManageProfilesBrowserProxyImpl.instance_ = browserProxy;

    document.body.innerHTML = '';
    app = /** @type {!ProfilePickerAppElement} */ (
        document.createElement('profile-picker-app'));
    document.body.appendChild(app);
  });

  /**
   * @return {!Promise} Promise that resolves when initialization is complete
   *     and the lazy loaded module has been loaded.
   */
  function waitForLoad() {
    return Promise.all([
      browserProxy.whenCalled('getNewProfileSuggestedThemeInfo'),
      ensureLazyLoaded(),
    ]);
  }

  test('signInButtonImplementation', function() {
    navigateTo(Routes.NEW_PROFILE);
    return waitForLoad()
        .then(() => {
          return waitBeforeNextRender(app);
        })
        .then(() => {
          const choice = /** @type {!ProfileTypeChoiceElement} */ (
              app.$$('profile-type-choice'));
          assertTrue(!!choice);
          choice.$$('#signInButton').click();
          return browserProxy.whenCalled('loadSignInProfileCreationFlow');
        });
  });
});
