// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {$$} from 'chrome://new-tab-page/new_tab_page.js';

suite('NewTabPageModuleWrapperTest', () => {
  /** @type {!ModuleWrapperElement} */
  let moduleWrapper;

  setup(() => {
    PolymerTest.clearBody();
    moduleWrapper = document.createElement('ntp-module-wrapper');
    document.body.appendChild(moduleWrapper);
  });

  test('renders module descriptor', async () => {
    // Arrange.
    const moduleElement = document.createElement('div');

    // Act.
    moduleWrapper.descriptor = {
      id: 'foo',
      name: 'Foo',
      title: 'Foo Title',
      element: moduleElement,
    };

    // Assert.
    assertEquals('Foo Title', moduleWrapper.$.title.textContent);
    assertEquals(' • Foo', moduleWrapper.$.name.textContent);
    assertDeepEquals(
        moduleElement, $$(moduleWrapper, '#moduleElement').children[0]);
  });
});
