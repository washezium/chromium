// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(crbug.com/1057147): add tests for chrome-untrusted://new-tab-page files.
document.addEventListener('DOMContentLoaded', () => {
  // Remove the <style> from the raw promo since we want to style the promo
  // ourselves.
  const styleElement = document.querySelector('body > style');
  if (styleElement) {
    styleElement.remove();
  }
  // The <a> elements need to open in the top frame since the promo is loaded
  // in an <iframe>.
  document.body.querySelectorAll('a').forEach(el => {
    if (el.target !== '_blank') {
      el.target = '_top';
    }
    el.addEventListener('click', (event) => {
      const browserCommandFound =
          event.target.getAttribute('href').match(/^command:(\d+)$/);
      if (browserCommandFound) {
        event.preventDefault();  // Prevent navigation attempt.
        window.parent.postMessage(
            {
              frameType: 'promo',
              messageType: 'execute-browser-command',
              commandId: parseInt(browserCommandFound[1], 10),
              clickInfo: {
                middleButton: event.button === 1,
                altKey: event.altKey,
                ctrlKey: event.ctrlKey,
                metaKey: event.metaKey,
                shiftKey: event.shiftKey
              }
            },
            'chrome://new-tab-page');
      }

      window.parent.postMessage(
          {frameType: 'promo', messageType: 'link-clicked'},
          'chrome://new-tab-page');
    });
  });
  // Inform the embedder that the promo has loaded and can be displayed.
  window.parent.postMessage(
      {frameType: 'promo', messageType: 'loaded'}, 'chrome://new-tab-page');
});
