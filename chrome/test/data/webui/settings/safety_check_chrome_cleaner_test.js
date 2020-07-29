// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
import {webUIListenerCallback} from 'chrome://resources/js/cr.m.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {ChromeCleanupProxy, ChromeCleanupProxyImpl} from 'chrome://settings/lazy_load.js';
import {MetricsBrowserProxyImpl, Router, routes, SafetyCheckCallbackConstants, SafetyCheckChromeCleanerStatus, SafetyCheckIconStatus, SafetyCheckInteractions} from 'chrome://settings/settings.js';

import {assertEquals, assertFalse, assertTrue} from '../chai_assert.js';
import {TestBrowserProxy} from '../test_browser_proxy.m.js';

import {TestMetricsBrowserProxy} from './test_metrics_browser_proxy.js';

// clang-format on

const testDisplayString = 'Test display string';

/**
 * Fire a safety check Chrome cleaner event.
 * @param {!SafetyCheckChromeCleanerStatus} state
 */
function fireSafetyCheckChromeCleanerEvent(state) {
  const event = {
    newState: state,
    displayString: testDisplayString,
  };
  webUIListenerCallback(
      SafetyCheckCallbackConstants.CHROME_CLEANER_CHANGED, event);
}

/**
 * Verify that the safety check child inside the page has been configured as
 * specified.
 * @param {!{
 *   page: !PolymerElement,
 *   iconStatus: !SafetyCheckIconStatus,
 *   label: string,
 *   buttonLabel: (string|undefined),
 *   buttonAriaLabel: (string|undefined),
 *   buttonClass: (string|undefined),
 *   managedIcon: (boolean|undefined),
 * }} destructured1
 */
function assertSafetyCheckChild({
  page,
  iconStatus,
  label,
  buttonLabel,
  buttonAriaLabel,
  buttonClass,
  managedIcon
}) {
  const safetyCheckChild = page.$$('#safetyCheckChild');
  assertTrue(safetyCheckChild.iconStatus === iconStatus);
  assertTrue(safetyCheckChild.label === label);
  assertTrue(safetyCheckChild.subLabel === testDisplayString);
  assertTrue(!buttonLabel || safetyCheckChild.buttonLabel === buttonLabel);
  assertTrue(
      !buttonAriaLabel || safetyCheckChild.buttonAriaLabel === buttonAriaLabel);
  assertTrue(!buttonClass || safetyCheckChild.buttonClass === buttonClass);
  assertTrue(!!managedIcon === !!safetyCheckChild.managedIcon);
}

suite('SafetyCheckChromeCleanerUiTests', function() {
  /**
   * @implements {BrowserProxy}
   * @extends {TestBrowserProxy}
   */
  let chromeCleanupBrowserProxy = null;

  /** @type {?TestMetricsBrowserProxy} */
  let metricsBrowserProxy = null;

  /** @type {!SettingsSafetyCheckExtensionsChildElement} */
  let page;

  setup(function() {
    chromeCleanupBrowserProxy = TestBrowserProxy.fromClass(ChromeCleanupProxy);
    chromeCleanupBrowserProxy.setResultFor(
        'restartComputer', Promise.resolve(0));
    ChromeCleanupProxyImpl.instance_ = chromeCleanupBrowserProxy;

    metricsBrowserProxy = new TestMetricsBrowserProxy();
    MetricsBrowserProxyImpl.instance_ = metricsBrowserProxy;

    document.body.innerHTML = '';
    page = /** @type {!SettingsSafetyCheckExtensionsChildElement} */ (
        document.createElement('settings-safety-check-chrome-cleaner-child'));
    document.body.appendChild(page);
    flush();
  });

  teardown(function() {
    page.remove();
  });

  /** @return {!Promise} */
  async function expectChromeCleanerRouteButtonClickActions() {
    // User clicks review extensions button.
    page.$$('#safetyCheckChild').$$('#button').click();
    // // TODO(crbug.com/1087263): Ensure UMA is logged.
    // Ensure the correct Settings page is shown.
    assertEquals(routes.CHROME_CLEANUP, Router.getInstance().getCurrentRoute());
  }

  test('chromeCleanerCheckingUiTest', function() {
    fireSafetyCheckChromeCleanerEvent(SafetyCheckChromeCleanerStatus.CHECKING);
    flush();
    assertSafetyCheckChild({
      page: page,
      iconStatus: SafetyCheckIconStatus.RUNNING,
      label: 'Device software',
    });
  });

  test('chromeCleanerSafeStatesUiTest', function() {
    for (const state of Object.values(SafetyCheckChromeCleanerStatus)) {
      switch (state) {
        case SafetyCheckChromeCleanerStatus.INITIAL:
        case SafetyCheckChromeCleanerStatus.REPORTER_FOUND_NOTHING:
        case SafetyCheckChromeCleanerStatus.SCANNING_FOUND_NOTHING:
        case SafetyCheckChromeCleanerStatus.CLEANING_SUCCEEDED:
        case SafetyCheckChromeCleanerStatus.REPORTER_RUNNING:
        case SafetyCheckChromeCleanerStatus.SCANNING:
          fireSafetyCheckChromeCleanerEvent(state);
          flush();
          assertSafetyCheckChild({
            page: page,
            iconStatus: SafetyCheckIconStatus.SAFE,
            label: 'Device software',
            buttonLabel: 'More',
            buttonAriaLabel: 'Show details of the device software check',
          });
          expectChromeCleanerRouteButtonClickActions();
          break;
        default:
          // Not covered by this test.
          break;
      }
    }
  });

  test('chromeCleanerErrorStates', function() {
    for (const state of Object.values(SafetyCheckChromeCleanerStatus)) {
      switch (state) {
        case SafetyCheckChromeCleanerStatus.REPORTER_FAILED:
        case SafetyCheckChromeCleanerStatus.SCANNING_FAILED:
        case SafetyCheckChromeCleanerStatus.CLEANING_FAILED:
        case SafetyCheckChromeCleanerStatus.CLEANER_DOWNLOAD_FAILED:
          fireSafetyCheckChromeCleanerEvent(state);
          flush();
          assertSafetyCheckChild({
            page: page,
            iconStatus: SafetyCheckIconStatus.INFO,
            label: 'Device software',
            buttonLabel: 'Details',
            buttonAriaLabel: 'Review error details',
          });
          expectChromeCleanerRouteButtonClickActions();
          break;
        default:
          // Not covered by this test.
          break;
      }
    }
  });

  test('chromeCleanerInfoWithDefaultButtonStatesUiTest', function() {
    for (const state of Object.values(SafetyCheckChromeCleanerStatus)) {
      switch (state) {
        case SafetyCheckChromeCleanerStatus.CLEANING:
          fireSafetyCheckChromeCleanerEvent(state);
          flush();
          assertSafetyCheckChild({
            page: page,
            iconStatus: SafetyCheckIconStatus.INFO,
            label: 'Device software',
            buttonLabel: 'Review',
            buttonAriaLabel: 'Review device software',
          });
          expectChromeCleanerRouteButtonClickActions();
          break;
        default:
          // Not covered by this test.
          break;
      }
    }
  });

  test('chromeCleanerWarningStatesUiTest', function() {
    for (const state of Object.values(SafetyCheckChromeCleanerStatus)) {
      switch (state) {
        case SafetyCheckChromeCleanerStatus.USER_DECLINED_CLEANUP:
        case SafetyCheckChromeCleanerStatus.INFECTED:
        case SafetyCheckChromeCleanerStatus.CONNECTION_LOST:
          fireSafetyCheckChromeCleanerEvent(state);
          flush();
          assertSafetyCheckChild({
            page: page,
            iconStatus: SafetyCheckIconStatus.WARNING,
            label: 'Device software',
            buttonLabel: 'Review',
            buttonAriaLabel: 'Review device software',
            buttonClass: 'action-button',
          });
          expectChromeCleanerRouteButtonClickActions();
          break;
        default:
          // Not covered by this test.
          break;
      }
    }
  });

  test('chromeCleanerRebootRequiredUiTest', function() {
    fireSafetyCheckChromeCleanerEvent(
        SafetyCheckChromeCleanerStatus.REBOOT_REQUIRED);
    flush();
    assertSafetyCheckChild({
      page: page,
      iconStatus: SafetyCheckIconStatus.INFO,
      label: 'Device software',
      buttonLabel: 'Restart computer',
      buttonAriaLabel: 'Restart computer',
      buttonClass: 'action-button',
    });
    // User clicks review extensions button.
    page.$$('#safetyCheckChild').$$('#button').click();
    // TODO(crbug.com/1087263): Ensure UMA is logged.
    // Ensure the browser proxy call is done.
    return chromeCleanupBrowserProxy.whenCalled('restartComputer');
  });

  test('chromeCleanerDisabledByAdminUiTest', function() {
    fireSafetyCheckChromeCleanerEvent(
        SafetyCheckChromeCleanerStatus.DISABLED_BY_ADMIN);
    flush();
    assertSafetyCheckChild({
      page: page,
      iconStatus: SafetyCheckIconStatus.INFO,
      label: 'Device software',
    });
  });
});
