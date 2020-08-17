// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {ViewerDocumentOutlineElement} from 'chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai/elements/viewer-document-outline.js';
import {ViewerPdfSidenavElement} from 'chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai/elements/viewer-pdf-sidenav.js';
import {ViewerThumbnailBarElement} from 'chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai/elements/viewer-thumbnail-bar.js';

/** @return {!ViewerPdfSidenavElement} */
function createSidenav() {
  document.body.innerHTML = '';
  const sidenav = /** @type {!ViewerPdfSidenavElement} */ (
      document.createElement('viewer-pdf-sidenav'));
  document.body.appendChild(sidenav);
  return sidenav;
}

// Unit tests for the viewer-pdf-sidenav element.
const tests = [
  /**
   * Test that the sidenav toggles between outline and thumbnail view.
   */
  function testViewToggle() {
    const sidenav = createSidenav();
    const content = sidenav.shadowRoot.querySelector('#content');
    const buttons = /** @type {!NodeList<!CrIconButtonElement>} */ (
        sidenav.shadowRoot.querySelectorAll('cr-icon-button'));

    const thumbnailBar =
        /** @type {!ViewerThumbnailBarElement} */ (
            content.querySelector('viewer-thumbnail-bar'));
    const outline = /** @type {!ViewerDocumentOutlineElement} */ (
        content.querySelector('viewer-document-outline'));


    // Sidebar starts on thumbnail view.
    chrome.test.assertTrue(
        buttons[0].parentNode.classList.contains('selected'));

    // Outline button is hidden since there are no bookmarks.
    chrome.test.assertTrue(buttons[1].parentNode.hidden);

    // Add some dummy bookmarks so that the button appears.
    sidenav.bookmarks = [
      {title: 'Foo', page: 1},
      {title: 'Bar', page: 2},
    ];
    chrome.test.assertFalse(buttons[1].parentNode.hidden);
    chrome.test.assertFalse(
        buttons[1].parentNode.classList.contains('selected'));
    chrome.test.assertFalse(thumbnailBar.hidden);
    chrome.test.assertTrue(outline.hidden);

    // Click on outline view.
    buttons[1].click();
    chrome.test.assertFalse(
        buttons[0].parentNode.classList.contains('selected'));
    chrome.test.assertTrue(
        buttons[1].parentNode.classList.contains('selected'));
    chrome.test.assertTrue(thumbnailBar.hidden);
    chrome.test.assertFalse(outline.hidden);

    // Return to thumbnail view.
    buttons[0].click();
    chrome.test.assertTrue(
        buttons[0].parentNode.classList.contains('selected'));
    chrome.test.assertFalse(
        buttons[1].parentNode.classList.contains('selected'));
    chrome.test.assertFalse(thumbnailBar.hidden);
    chrome.test.assertTrue(outline.hidden);

    chrome.test.succeed();
  },
];

chrome.test.runTests(tests);
