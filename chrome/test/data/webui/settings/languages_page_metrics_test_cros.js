// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {LanguagesBrowserProxyImpl, LanguagesMetricsProxyImpl, LanguagesPageInteraction} from 'chrome://settings/lazy_load.js';
import {CrSettingsPrefs} from 'chrome://settings/settings.js';

import {fakeDataBind} from '../test_util.m.js';

import {getFakeLanguagePrefs} from './fake_language_settings_private.m.js';
import {FakeSettingsPrivate} from './fake_settings_private.m.js';
import {TestLanguagesBrowserProxy} from './test_languages_browser_proxy.m.js';
import {TestLanguagesMetricsProxy} from './test_languages_metrics_proxy.js';

suite('LanguagesPageMetricsChromeOS', function() {
  /** @type {?LanguageHelper} */
  let languageHelper = null;
  /** @type {?SettingsLanguagesPageElement} */
  let languagesPage = null;
  /** @type {?LanguagesBrowserProxy} */
  let browserProxy = null;
  /** @type {?LanguagesMetricsProxy} */
  let languagesMetricsProxy = null;

  suiteSetup(function() {
    PolymerTest.clearBody();
    CrSettingsPrefs.deferInitialization = true;
  });

  setup(function() {
    const settingsPrefs = document.createElement('settings-prefs');
    const settingsPrivate = new FakeSettingsPrivate(getFakeLanguagePrefs());
    settingsPrefs.initialize(settingsPrivate);
    document.body.appendChild(settingsPrefs);
    return CrSettingsPrefs.initialized.then(function() {
      // Sets up test browser proxy.
      browserProxy = new TestLanguagesBrowserProxy();
      LanguagesBrowserProxyImpl.instance_ = browserProxy;

      // Sets up test browser proxy.
      languagesMetricsProxy = new TestLanguagesMetricsProxy();
      LanguagesMetricsProxyImpl.instance_ = languagesMetricsProxy;

      // Sets up fake languageSettingsPrivate API.
      const languageSettingsPrivate = browserProxy.getLanguageSettingsPrivate();
      languageSettingsPrivate.setSettingsPrefs(settingsPrefs);

      languagesPage = document.createElement('settings-languages-page');

      // Prefs would normally be data-bound to settings-languages-page.
      languagesPage.prefs = settingsPrefs.prefs;
      fakeDataBind(settingsPrefs, languagesPage, 'prefs');

      document.body.appendChild(languagesPage);
      languageHelper = languagesPage.languageHelper;
      return languageHelper.whenReady();
    });
  });

  teardown(function() {
    PolymerTest.clearBody();
  });

  test('records when adding languages', async () => {
    languagesPage.$.addLanguages.click();
    flush();

    await languagesMetricsProxy.whenCalled('recordAddLanguages');
  });

  test('records when clicking edit dictionary', async () => {
    languagesPage.$.spellCheckSubpageTrigger.click();
    flush();

    assertEquals(
        LanguagesPageInteraction.OPEN_CUSTOM_SPELL_CHECK,
        await languagesMetricsProxy.whenCalled('recordInteraction'));
  });

  test('records when disabling translate.enable toggle', async () => {
    languageHelper.setPrefValue('translate.enabled', true);
    languagesPage.$.offerTranslateOtherLanguages.click();
    flush();

    assertFalse(
        await languagesMetricsProxy.whenCalled('recordToggleTranslate'));
  });

  test('records when enabling translate.enable toggle', async () => {
    languageHelper.setPrefValue('translate.enabled', false);
    languagesPage.$.offerTranslateOtherLanguages.click();
    flush();

    assertTrue(await languagesMetricsProxy.whenCalled('recordToggleTranslate'));
  });

  test('records when disabling spell check toggle', async () => {
    languageHelper.setPrefValue('browser.enable_spellchecking', true);
    languagesPage.$.enableSpellcheckingToggle.click();
    flush();

    assertFalse(
        await languagesMetricsProxy.whenCalled('recordToggleSpellCheck'));
  });

  test('records when enabling spell check toggle', async () => {
    languageHelper.setPrefValue('browser.enable_spellchecking', false);
    languagesPage.$.enableSpellcheckingToggle.click();
    flush();

    assertTrue(
        await languagesMetricsProxy.whenCalled('recordToggleSpellCheck'));
  });

  test('records when switching system language and restarting', async () => {
    // Adds several languages.
    for (const language of ['en-CA', 'en-US', 'tk', 'no']) {
      languageHelper.enableLanguage(language);
    }

    flush();

    const languagesCollapse = languagesPage.$.languagesCollapse;
    languagesCollapse.opened = true;

    const menuButtons = languagesCollapse.querySelectorAll(
        '.list-item cr-icon-button.icon-more-vert');

    // Chooses the second language to switch system language,
    // as first language is the default language.
    menuButtons[1].click();
    const actionMenu = languagesPage.$.menu.get();
    assertTrue(actionMenu.open);
    const menuItems = actionMenu.querySelectorAll('.dropdown-item');
    for (const item of menuItems) {
      if (item.id === 'uiLanguageItem') {
        assertFalse(item.checked);
        item.click();

        assertEquals(
            LanguagesPageInteraction.SWITCH_SYSTEM_LANGUAGE,
            await languagesMetricsProxy.whenCalled('recordInteraction'));
        return;
      }
    }
    actionMenu.close();

    // Chooses restart button after switching system language.
    const restartButton = languagesPage.$.restartButton;
    assertTrue(!!restartButton);
    restartButton.click();

    assertEquals(
        LanguagesPageInteraction.RESTART,
        await languagesMetricsProxy.whenCalled('recordInteraction'));
  });
});
