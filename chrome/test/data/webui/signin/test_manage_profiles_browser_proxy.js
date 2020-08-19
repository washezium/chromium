// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {ManageProfilesBrowserProxy} from 'chrome://profile-picker/manage_profiles_browser_proxy.js';

import {TestBrowserProxy} from '../test_browser_proxy.m.js';

/** @implements {ManageProfilesBrowserProxy} */
export class TestManageProfilesBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'initializeMainView',
      'launchGuestProfile',
      'openManageProfileSettingsSubPage',
      'launchSelectedProfile',
      'askOnStartupChanged',
      'getNewProfileSuggestedThemeInfo',
      'removeProfile',
      'getProfileStatistics',
      'loadSignInProfileCreationFlow',
    ]);
  }

  /** @override */
  initializeMainView() {
    this.methodCalled('initializeMainView');
  }

  /** @override */
  launchGuestProfile() {
    this.methodCalled('launchGuestProfile');
  }

  /** @override */
  openManageProfileSettingsSubPage(profilePath) {
    this.methodCalled('openManageProfileSettingsSubPage', profilePath);
  }

  /** @override */
  launchSelectedProfile(profilePath) {
    this.methodCalled('launchSelectedProfile', profilePath);
  }

  /** @override */
  askOnStartupChanged(shouldShow) {
    this.methodCalled('askOnStartupChanged', shouldShow);
  }

  /** @override */
  getNewProfileSuggestedThemeInfo() {
    this.methodCalled('getNewProfileSuggestedThemeInfo');
    return Promise.resolve({
      colorId: 0,
      themeFrameColor: '',
      themeShapeColor: '',
      themeFrameTextColor: '',
      themeGenericAvatar: ''
    });
  }

  /** @override */
  removeProfile(profilePath) {
    this.methodCalled('removeProfile', profilePath);
  }

  /** @override */
  getProfileStatistics(profilePath) {
    this.methodCalled('getProfileStatistics', profilePath);
  }

  /** @override */
  loadSignInProfileCreationFlow() {
    this.methodCalled('loadSignInProfileCreationFlow');
  }
}
