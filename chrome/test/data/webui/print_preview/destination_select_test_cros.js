// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {Destination, DestinationConnectionStatus, DestinationOrigin, DestinationType, getSelectDropdownBackground, NativeLayer, NativeLayerImpl, PrinterState, PrinterStatus, PrinterStatusReason, PrinterStatusSeverity, SAVE_TO_DRIVE_CROS_DESTINATION_KEY} from 'chrome://print/print_preview.js';
import {assert} from 'chrome://resources/js/assert.m.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {Base, flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {assertEquals, assertFalse, assertTrue} from '../chai_assert.js';
import {waitBeforeNextRender} from '../test_util.m.js';

import {NativeLayerStub} from './native_layer_stub.js';
import {getGoogleDriveDestination, getSaveAsPdfDestination, selectOption} from './print_preview_test_utils.js';

window.printer_status_test_cros = {};
const printer_status_test_cros = window.printer_status_test_cros;
printer_status_test_cros.suiteName = 'PrinterStatusTestCros';
/** @enum {string} */
printer_status_test_cros.TestNames = {
  PrinterStatusUpdatesColor: 'printer status updates color',
  SendStatusRequestOnce: 'send status request once',
  HiddenStatusText: 'hidden status text',
  ChangeIcon: 'change icon',
};

suite(printer_status_test_cros.suiteName, function() {
  /** @type {!PrintPreviewDestinationSelectCrosElement} */
  let destinationSelect;

  const account = 'foo@chromium.org';

  /** @type {?NativeLayerStub} */
  let nativeLayer = null;

  function setNativeLayerPrinterStatusMap() {
    [
     {
       printerId: 'ID1',
       statusReasons: [{
         reason: PrinterStatusReason.NO_ERROR,
         severity: PrinterStatusSeverity.UNKNOWN_SEVERITY
       }],
     },
     {
       printerId: 'ID2',
       statusReasons: [
         {
           reason: PrinterStatusReason.NO_ERROR,
           severity: PrinterStatusSeverity.UNKNOWN_SEVERITY
         },
         {
           reason: PrinterStatusReason.LOW_ON_PAPER,
           severity: PrinterStatusSeverity.UNKNOWN_SEVERITY
         }
       ],
     },
     {
       printerId: 'ID3',
       statusReasons: [
         {
           reason: PrinterStatusReason.NO_ERROR,
           severity: PrinterStatusSeverity.UNKNOWN_SEVERITY
         },
         {
           reason: PrinterStatusReason.LOW_ON_PAPER,
           severity: PrinterStatusSeverity.REPORT
         }
       ],
     },
     {
       printerId: 'ID4',
       statusReasons: [
         {
           reason: PrinterStatusReason.NO_ERROR,
           severity: PrinterStatusSeverity.UNKNOWN_SEVERITY
         },
         {
           reason: PrinterStatusReason.LOW_ON_PAPER,
           severity: PrinterStatusSeverity.WARNING
         }
       ],
     },
     {
       printerId: 'ID5',
       statusReasons: [
         {
           reason: PrinterStatusReason.NO_ERROR,
           severity: PrinterStatusSeverity.UNKNOWN_SEVERITY
         },
         {
           reason: PrinterStatusReason.LOW_ON_PAPER,
           severity: PrinterStatusSeverity.ERROR
         }
       ],
     },
     {
       printerId: 'ID6',
       statusReasons: [
         {
           reason: PrinterStatusReason.DEVICE_ERROR,
           severity: PrinterStatusSeverity.UNKNOWN_SEVERITY
         },
         {
           reason: PrinterStatusReason.PRINTER_QUEUE_FULL,
           severity: PrinterStatusSeverity.ERROR
         }
       ],
     },
     {
       printerId: 'ID7',
       statusReasons: [
         {
           reason: PrinterStatusReason.DEVICE_ERROR,
           severity: PrinterStatusSeverity.REPORT
         },
         {
           reason: PrinterStatusReason.PRINTER_QUEUE_FULL,
           severity: PrinterStatusSeverity.REPORT
         }
       ],
     }].forEach(status =>
                  nativeLayer.addPrinterStatusToMap(status.printerId, status));
  }

  /**
   * @param {string} id
   * @param {string} displayName
   * @param {!DestinationOrigin} destinationOrigin
   * @return {!Destination}
   */
  function createDestination(id, displayName, destinationOrigin) {
    return new Destination(
        id, DestinationType.LOCAL, destinationOrigin, displayName,
        DestinationConnectionStatus.ONLINE);
  }

  /**
   * @param {string} value
   * @return {string}
   */
  function escapeForwardSlahes(value) {
    return value.replace(/\//g, '\\/');
  }

  setup(function() {
    document.body.innerHTML = '';

    // Stub out native layer.
    nativeLayer = new NativeLayerStub();
    NativeLayerImpl.instance_ = nativeLayer;
    setNativeLayerPrinterStatusMap();

    destinationSelect =
        /** @type {!PrintPreviewDestinationSelectCrosElement} */
        (document.createElement('print-preview-destination-select-cros'));
    document.body.appendChild(destinationSelect);
  });

  test(
      assert(printer_status_test_cros.TestNames.PrinterStatusUpdatesColor),
      function() {
        const destination1 =
            createDestination('ID1', 'One', DestinationOrigin.CROS);
        const destination2 =
            createDestination('ID2', 'Two', DestinationOrigin.CROS);
        const destination3 =
            createDestination('ID3', 'Three', DestinationOrigin.CROS);
        const destination4 =
            createDestination('ID4', 'Four', DestinationOrigin.CROS);
        const destination5 =
            createDestination('ID5', 'Five', DestinationOrigin.CROS);
        const destination6 =
            createDestination('ID6', 'Six', DestinationOrigin.CROS);
        const destination7 =
            createDestination('ID7', 'Seven', DestinationOrigin.CROS);

        return waitBeforeNextRender(destinationSelect).then(() => {
          const whenStatusRequestsDone =
              nativeLayer.waitForMultiplePrinterStatusRequests(7);

          destinationSelect.recentDestinationList = [
            destination1,
            destination2,
            destination3,
            destination4,
            destination5,
            destination6,
            destination7,
          ];

          const dropdown = destinationSelect.$$('#dropdown');
          return whenStatusRequestsDone.then(() => {
            assertEquals(
                PrinterState.GOOD,
                dropdown.$$(`#${escapeForwardSlahes(destination1.key)}`)
                    .firstChild.printerState);
            assertEquals(
                PrinterState.GOOD,
                dropdown.$$(`#${escapeForwardSlahes(destination2.key)}`)
                    .firstChild.printerState);
            assertEquals(
                PrinterState.GOOD,
                dropdown.$$(`#${escapeForwardSlahes(destination3.key)}`)
                    .firstChild.printerState);
            assertEquals(
                PrinterState.ERROR,
                dropdown.$$(`#${escapeForwardSlahes(destination4.key)}`)
                    .firstChild.printerState);
            assertEquals(
                PrinterState.ERROR,
                dropdown.$$(`#${escapeForwardSlahes(destination5.key)}`)
                    .firstChild.printerState);
            assertEquals(
                PrinterState.ERROR,
                dropdown.$$(`#${escapeForwardSlahes(destination6.key)}`)
                    .firstChild.printerState);
            assertEquals(
                PrinterState.UNKNOWN,
                dropdown.$$(`#${escapeForwardSlahes(destination7.key)}`)
                    .firstChild.printerState);
          });
        });
      });

  test(
      assert(printer_status_test_cros.TestNames.SendStatusRequestOnce),
      function() {
        return waitBeforeNextRender(destinationSelect).then(() => {
          destinationSelect.recentDestinationList = [
            createDestination('ID1', 'One', DestinationOrigin.CROS),
            createDestination('ID2', 'Two', DestinationOrigin.CROS),
            createDestination('ID3', 'Three', DestinationOrigin.PRIVET),
            createDestination('ID4', 'Four', DestinationOrigin.EXTENSION),
          ];
          assertEquals(
              2, nativeLayer.getCallCount('requestPrinterStatusUpdate'));

          // Update list with 2 existing destinations and one new destination.
          // Make sure the requestPrinterStatusUpdate only gets called for the
          // new destination.
          destinationSelect.recentDestinationList = [
            createDestination('ID1', 'One', DestinationOrigin.CROS),
            createDestination('ID2', 'Two', DestinationOrigin.CROS),
            createDestination('ID5', 'Five', DestinationOrigin.CROS),
          ];
          assertEquals(
              3, nativeLayer.getCallCount('requestPrinterStatusUpdate'));
        });
      });

  test(assert(printer_status_test_cros.TestNames.HiddenStatusText), function() {
    return waitBeforeNextRender(destinationSelect).then(() => {
      const destinationWithoutErrorStatus =
          createDestination('ID1', 'One', DestinationOrigin.CROS);
      // Destination with ID4 will return an error printer status that will
      // trigger the error text being populated.
      const destinationWithErrorStatus =
          createDestination('ID4', 'Four', DestinationOrigin.CROS);
      const cloudPrintDestination = new Destination(
          'ID2', DestinationType.GOOGLE, DestinationOrigin.COOKIES, 'Two',
          DestinationConnectionStatus.OFFLINE, {account: account});

      destinationSelect.recentDestinationList = [
        destinationWithoutErrorStatus,
        destinationWithErrorStatus,
        cloudPrintDestination,
      ];

      const destinationStatus =
          destinationSelect.$$('.destination-additional-info');
      const destinationEulaWrapper =
          destinationSelect.$$('#destinationEulaWrapper');

      destinationSelect.destination = cloudPrintDestination;
      assertFalse(destinationStatus.hidden);
      assertTrue(destinationEulaWrapper.hidden);

      destinationSelect.destination = destinationWithoutErrorStatus;
      assertTrue(destinationStatus.hidden);
      assertTrue(destinationEulaWrapper.hidden);

      destinationSelect.set('destination.eulaUrl', 'chrome://os-credits/eula');
      assertFalse(destinationEulaWrapper.hidden);

      destinationSelect.destination = destinationWithErrorStatus;
      return nativeLayer.whenCalled('requestPrinterStatusUpdate').then(() => {
        assertFalse(destinationStatus.hidden);
      });
    });
  });

  test(assert(printer_status_test_cros.TestNames.ChangeIcon), function() {
    return waitBeforeNextRender(destinationSelect).then(() => {
      const localCrosPrinter =
          createDestination('ID1', 'One', DestinationOrigin.CROS);
      const saveToDrive = getGoogleDriveDestination('account');
      const saveAsPdf = getSaveAsPdfDestination();

      destinationSelect.recentDestinationList = [
        localCrosPrinter,
        saveToDrive,
        saveAsPdf,
      ];
      const dropdown = destinationSelect.$$('#dropdown');

      destinationSelect.destination = localCrosPrinter;
      destinationSelect.updateDestination();
      assertEquals('print-preview:print', dropdown.destinationIcon);

      destinationSelect.destination = saveToDrive;
      destinationSelect.updateDestination();
      assertEquals('print-preview:save-to-drive', dropdown.destinationIcon);

      destinationSelect.destination = saveAsPdf;
      destinationSelect.updateDestination();
      assertEquals('cr:insert-drive-file', dropdown.destinationIcon);
    });
  });
});

window.destination_select_test_cros = {};
const destination_select_test_cros = window.destination_select_test_cros;
destination_select_test_cros.suiteName = 'DestinationSelectTestCros';
/** @enum {string} */
destination_select_test_cros.TestNames = {
  UpdateStatus: 'update status',
  ChangeIcon: 'change icon',
  EulaIsDisplayed: 'eula is displayed',
  SelectDriveDestination: 'select drive destination',
};

suite(destination_select_test_cros.suiteName, function() {
  /** @type {!PrintPreviewDestinationSelectCrosElement} */
  let destinationSelect;

  const account = 'foo@chromium.org';

  let recentDestinationList = [];

  const meta = /** @type {!IronMetaElement} */ (
      Base.create('iron-meta', {type: 'iconset'}));

  /** @override */
  setup(function() {
    document.body.innerHTML = '';

    destinationSelect =
        /** @type {!PrintPreviewDestinationSelectCrosElement} */
        (document.createElement('print-preview-destination-select-cros'));
    destinationSelect.activeUser = account;
    destinationSelect.appKioskMode = false;
    destinationSelect.disabled = false;
    destinationSelect.loaded = false;
    destinationSelect.noDestinations = false;
    populateRecentDestinationList();
    destinationSelect.recentDestinationList = recentDestinationList;

    document.body.appendChild(destinationSelect);
  });

  // Create three different destinations and use them to populate
  // |recentDestinationList|.
  function populateRecentDestinationList() {
    recentDestinationList = [
      new Destination(
          'ID1', DestinationType.LOCAL, DestinationOrigin.LOCAL, 'One',
          DestinationConnectionStatus.ONLINE),
      new Destination(
          'ID2', DestinationType.GOOGLE, DestinationOrigin.COOKIES, 'Two',
          DestinationConnectionStatus.OFFLINE, {account: account}),
      new Destination(
          'ID3', DestinationType.GOOGLE, DestinationOrigin.COOKIES, 'Three',
          DestinationConnectionStatus.ONLINE,
          {account: account, isOwned: true}),
    ];
  }

  function compareIcon(selectEl, expectedIcon) {
    const icon = selectEl.style['background-image'].replace(/ /gi, '');
    const expected = getSelectDropdownBackground(
        /** @type {!IronIconsetSvgElement} */
        (meta.byKey('print-preview')), expectedIcon, destinationSelect);
    assertEquals(expected, icon);
  }

  /**
   * Test that changing different destinations results in the correct icon being
   * shown.
   * @return {!Promise} Promise that resolves when the test finishes.
   */
  function testChangeIcon() {
    const cookieOrigin = DestinationOrigin.COOKIES;
    let selectEl;

    return waitBeforeNextRender(destinationSelect)
        .then(() => {
          const destination = recentDestinationList[0];
          destinationSelect.destination = destination;
          destinationSelect.updateDestination();
          destinationSelect.loaded = true;
          selectEl = destinationSelect.$$('.md-select');
          compareIcon(selectEl, 'print');
          const driveKey =
              `${Destination.GooglePromotedId.DOCS}/${cookieOrigin}/${account}`;
          destinationSelect.driveDestinationKey = driveKey;

          return selectOption(destinationSelect, driveKey);
        })
        .then(() => {
          // Icon updates early based on the ID.
          compareIcon(selectEl, 'save-to-drive');

          // Update the destination.
          destinationSelect.destination = getGoogleDriveDestination(account);

          // Still Save to Drive icon.
          compareIcon(selectEl, 'save-to-drive');

          // Select a destination with the shared printer icon.
          return selectOption(
              destinationSelect, `ID2/${cookieOrigin}/${account}`);
        })
        .then(() => {
          // Should already be updated.
          compareIcon(selectEl, 'printer-shared');

          // Update destination.
          destinationSelect.destination = recentDestinationList[1];
          compareIcon(selectEl, 'printer-shared');

          // Select a destination with a standard printer icon.
          return selectOption(
              destinationSelect, `ID3/${cookieOrigin}/${account}`);
        })
        .then(() => {
          compareIcon(selectEl, 'print');
        });
  }

  test(assert(destination_select_test_cros.TestNames.UpdateStatus), function() {
    return waitBeforeNextRender(destinationSelect).then(() => {
      assertFalse(destinationSelect.$$('.throbber-container').hidden);
      assertTrue(destinationSelect.$$('.md-select').hidden);

      destinationSelect.loaded = true;
      assertTrue(destinationSelect.$$('.throbber-container').hidden);
      assertFalse(destinationSelect.$$('.md-select').hidden);

      destinationSelect.destination = recentDestinationList[0];
      destinationSelect.updateDestination();
      assertTrue(destinationSelect.$$('.destination-additional-info').hidden);

      destinationSelect.destination = recentDestinationList[1];
      destinationSelect.updateDestination();
      assertFalse(destinationSelect.$$('.destination-additional-info').hidden);
    });
  });

  test(assert(destination_select_test_cros.TestNames.ChangeIcon), function() {
    return testChangeIcon();
  });

  /**
   * Tests that destinations with a EULA will display the EULA URL.
   */
  test(
      assert(destination_select_test_cros.TestNames.EulaIsDisplayed),
      function() {
        destinationSelect.destination = recentDestinationList[0];
        destinationSelect.loaded = true;
        const destinationEulaWrapper =
            destinationSelect.$$('#destinationEulaWrapper');
        assertTrue(destinationEulaWrapper.hidden);

        destinationSelect.set(
            'destination.eulaUrl', 'chrome://os-credits/eula');
        assertFalse(destinationEulaWrapper.hidden);
      });

  // Tests that the correct drive destination is in the select based on value of
  // printSaveToDrive flag.
  test(
      assert(destination_select_test_cros.TestNames.SelectDriveDestination),
      function() {
        const driveDestinationKey = `${Destination.GooglePromotedId.DOCS}/${
            DestinationOrigin.COOKIES}/${account}`;
        const printSaveToDriveEnabled =
            loadTimeData.getBoolean('printSaveToDrive');
        const expectedKey = printSaveToDriveEnabled ?
            SAVE_TO_DRIVE_CROS_DESTINATION_KEY :
            driveDestinationKey;
        const wrongKey = !printSaveToDriveEnabled ?
            SAVE_TO_DRIVE_CROS_DESTINATION_KEY :
            driveDestinationKey;

        return waitBeforeNextRender(destinationSelect)
            .then(() => {
              destinationSelect.driveDestinationKey = driveDestinationKey;
              destinationSelect.destination = recentDestinationList[0];
              destinationSelect.updateDestination();

              assertTrue(
                  !!Array.from(destinationSelect.$$('.md-select').options)
                        .find(option => option.value === expectedKey));
              assertTrue(!Array.from(destinationSelect.$$('.md-select').options)
                              .find(option => option.value === wrongKey));
              return selectOption(destinationSelect, expectedKey);
            })
            .then(() => {
              assertEquals(
                  expectedKey, destinationSelect.$$('.md-select').value);
            });
      });
});
