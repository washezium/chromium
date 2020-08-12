// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
// #import 'chrome://os-settings/chromeos/os_settings.js';

// #import {AmbientModeBrowserProxyImpl} from 'chrome://os-settings/chromeos/os_settings.js';
// #import {TestBrowserProxy} from '../../test_browser_proxy.m.js';
// #import {assertEquals, assertFalse, assertTrue} from '../../chai_assert.js';
// #import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
// clang-format on

/**
 * @implements {settings.AmbientModeBrowserProxy}
 */
class TestAmbientModeBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'requestTopicSource',
      'requestAlbums',
      'setSelectedTopicSource',
      'setSelectedAlbums',
    ]);
  }

  /** @override */
  requestTopicSource() {
    this.methodCalled('requestTopicSource');
  }

  /** @override */
  requestAlbums(topicSource) {
    this.methodCalled('requestAlbums', [topicSource]);
  }

  /** @override */
  setSelectedTopicSource(topicSource) {
    this.methodCalled('setSelectedTopicSource', [topicSource]);
  }

  /** @override */
  setSelectedAlbums(settings) {
    this.methodCalled('setSelectedAlbums', [settings]);
  }
}

suite('AmbientModeHandler', function() {
  /** @type {SettingsAmbientModePhotosPageElement} */
  let ambientModePhotosPage = null;

  /** @type {?TestAmbientModeBrowserProxy} */
  let browserProxy = null;

  suiteSetup(function() {});

  setup(function() {
    browserProxy = new TestAmbientModeBrowserProxy();
    settings.AmbientModeBrowserProxyImpl.instance_ = browserProxy;
    PolymerTest.clearBody();

    ambientModePhotosPage =
        document.createElement('settings-ambient-mode-photos-page');
    document.body.appendChild(ambientModePhotosPage);
    Polymer.dom.flush();
  });

  teardown(function() {
    ambientModePhotosPage.remove();
  });

  test('hasAlbums', function() {
    ambientModePhotosPage.albums_ = [
      {albumId: 'id0', checked: true, title: 'album0'},
      {albumId: 'id1', checked: false, title: 'album1'}
    ];
    Polymer.dom.flush();

    const albumList = ambientModePhotosPage.$$('album-list');
    const ironList = albumList.$$('iron-list');
    const albumItems = ironList.querySelectorAll('album-item');
    assertEquals(2, albumItems.length);

    const album0 = albumItems[0];
    const album1 = albumItems[1];
    assertEquals('id0', album0.album.albumId);
    assertTrue(album0.album.checked);
    assertEquals('album0', album0.album.title);
    assertEquals('id1', album1.album.albumId);
    assertFalse(album1.album.checked);
    assertEquals('album1', album1.album.title);
  });

  test('toggleAlbumSelectionByClick', function() {
    ambientModePhotosPage.albums_ = [
      {albumId: 'id0', checked: true, title: 'album0'},
      {albumId: 'id1', checked: false, title: 'album1'}
    ];
    Polymer.dom.flush();

    const albumList = ambientModePhotosPage.$$('album-list');
    const ironList = albumList.$$('iron-list');
    const albumItems = ironList.querySelectorAll('album-item');
    assertEquals(2, albumItems.length);

    const album0 = albumItems[0];
    const album1 = albumItems[1];
    assertTrue(album0.checked);
    assertFalse(album1.checked);

    // Verify that the selected-albums-changed event is sent when the album
    // image is clicked.
    let selectedAlbumsChangedEventCalls = 0;
    albumList.addEventListener('selected-albums-changed', (event) => {
      selectedAlbumsChangedEventCalls++;
    });

    // Click album item image will toggle the check.
    album0.$.image.click();
    assertFalse(album0.checked);
    assertEquals(1, selectedAlbumsChangedEventCalls);

    // Click album item image will toggle the check.
    album0.$.image.click();
    assertTrue(album0.checked);
    assertEquals(2, selectedAlbumsChangedEventCalls);

    // Click album item image will toggle the check.
    album1.$.image.click();
    assertTrue(album1.checked);
    assertEquals(3, selectedAlbumsChangedEventCalls);

    // Click album item image will toggle the check.
    album1.$.image.click();
    assertFalse(album1.checked);
    assertEquals(4, selectedAlbumsChangedEventCalls);
  });

  test('setSelectedAlbums', async () => {
    ambientModePhotosPage.albums_ = [
      {albumId: 'id0', checked: true, title: 'album0'},
      {albumId: 'id1', checked: false, title: 'album1'}
    ];
    Polymer.dom.flush();

    const albumList = ambientModePhotosPage.$$('album-list');
    const ironList = albumList.$$('iron-list');
    const albumItems = ironList.querySelectorAll('album-item');
    assertEquals(2, albumItems.length);

    const album0 = albumItems[0];
    const album1 = albumItems[1];
    assertTrue(album0.checked);
    assertFalse(album1.checked);

    browserProxy.resetResolver('setSelectedAlbums');

    // Click album item image will toggle the check.
    album1.$.image.click();
    assertTrue(album1.checked);

    assertEquals(1, browserProxy.getCallCount('setSelectedAlbums'));
    let albumsArgs = await browserProxy.whenCalled('setSelectedAlbums');
    assertDeepEquals(
        [{albumId: 'id0'}, {albumId: 'id1'}], albumsArgs[0].albums);

    browserProxy.resetResolver('setSelectedAlbums');

    // Click album item image will toggle the check.
    album0.$.image.click();
    assertFalse(album0.checked);

    assertEquals(1, browserProxy.getCallCount('setSelectedAlbums'));
    albumsArgs = await browserProxy.whenCalled('setSelectedAlbums');
    assertDeepEquals([{albumId: 'id1'}], albumsArgs[0].albums);
  });

  test('notToggleAlbumSelection', function() {
    ambientModePhotosPage.albums_ = [
      {albumId: 'id0', checked: true, title: 'album0'},
    ];
    Polymer.dom.flush();

    const albumList = ambientModePhotosPage.$$('album-list');
    const ironList = albumList.$$('iron-list');
    const albumItems = ironList.querySelectorAll('album-item');
    assertEquals(1, albumItems.length);

    const album0 = albumItems[0];
    assertTrue(album0.checked);

    // Verify that the selected-albums-changed event is sent when the album
    // image is clicked.
    let selectedAlbumsChangedEventCalls = 0;
    albumList.addEventListener('selected-albums-changed', (event) => {
      selectedAlbumsChangedEventCalls++;
    });

    // Click outside album item image will not toggle the check.
    album0.$.albumInfo.click();
    assertTrue(album0.checked);
    assertEquals(0, selectedAlbumsChangedEventCalls);
  });

  test('toggleAlbumSelectionByKeypress', function() {
    ambientModePhotosPage.albums_ = [
      {albumId: 'id0', checked: true, title: 'album0'},
    ];
    Polymer.dom.flush();

    const albumList = ambientModePhotosPage.$$('album-list');
    const ironList = albumList.$$('iron-list');
    const albumItems = ironList.querySelectorAll('album-item');
    assertEquals(1, albumItems.length);

    const album0 = albumItems[0];
    assertTrue(album0.checked);

    // Verify that the selected-albums-changed event is sent when the album
    // image is clicked.
    let selectedAlbumsChangedEventCalls = 0;
    albumList.addEventListener('selected-albums-changed', (event) => {
      selectedAlbumsChangedEventCalls++;
    });

    // Keydown with Enter key on album item will toggle the selection.
    const enterEvent = new KeyboardEvent(
        'keydown', {cancelable: true, key: 'Enter', keyCode: 13});
    album0.dispatchEvent(enterEvent);
    assertFalse(album0.checked);
    assertEquals(1, selectedAlbumsChangedEventCalls);

    // Keydown with Enter key on album item will toggle the selection.
    album0.dispatchEvent(enterEvent);
    assertTrue(album0.checked);
    assertEquals(2, selectedAlbumsChangedEventCalls);
  });

  test('updateAlbumURL', function() {
    ambientModePhotosPage.albums_ = [
      {albumId: 'id0', checked: true, title: 'album0', url: ''},
    ];
    ambientModePhotosPage.topicSource_ = AmbientModeTopicSource.ART_GALLERY;
    Polymer.dom.flush();

    const albumList = ambientModePhotosPage.$$('album-list');
    const ironList = albumList.$$('iron-list');
    const albumItems = ironList.querySelectorAll('album-item');
    assertEquals(1, albumItems.length);

    const album0 = albumItems[0];
    assertEquals('', album0.album.url);

    // Update album URL.
    const url = 'chrome://ambient';
    cr.webUIListenerCallback('album-preview-changed', {
      topicSource: AmbientModeTopicSource.ART_GALLERY,
      albumId: 'id0',
      url: url
    });
    assertEquals(url, album0.album.url);
  });

  test('notUpdateAlbumURL', function() {
    ambientModePhotosPage.albums_ = [
      {albumId: 'id0', checked: true, title: 'album0', url: ''},
    ];
    ambientModePhotosPage.topicSource_ = AmbientModeTopicSource.ART_GALLERY;
    Polymer.dom.flush();

    const albumList = ambientModePhotosPage.$$('album-list');
    const ironList = albumList.$$('iron-list');
    const albumItems = ironList.querySelectorAll('album-item');
    assertEquals(1, albumItems.length);

    const album0 = albumItems[0];
    assertEquals('', album0.album.url);

    // Different topic source will no update album URL.
    const url = 'chrome://ambient';
    cr.webUIListenerCallback('album-preview-changed', {
      topicSource: AmbientModeTopicSource.GOOGLE_PHOTOS,
      albumId: 'id0',
      url: url
    });
    assertEquals('', album0.album.url);
  });
});
