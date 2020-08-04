// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 *
 * Diagnostic Processor Support Library (DPSL) is a collection of telemetry and
 * diagnostics interfaces exposed to third-parties:
 *
 *   - chromeos.diagnostics
 *     | Diagnostics interface for running device diagnostics routines (tests).
 *
 *   - chromeos.telemetry
 *     | Telemetry (a.k.a. Probe) interface for getting device telemetry
 *     | information.
 */

chromeos.diagnostics = null;

chromeos.telemetry = null;

/**
 * This is only for testing purposes. Please don't use it in the production,
 * because we may silently change or remove it.
 */
chromeos.test_support = {};

(() => {
  const messagePipe =
      new MessagePipe('chrome://telemetry-extension', window.parent);

  /**
   * DPSL Diagnostics Requester.
   */
  class DiagnosticsRequester {
    constructor() {}

    /**
     * Requests a list of available routines.
     * @return {!Promise<!Array<!chromeos.health.mojom.DiagnosticRoutineEnum>>}
     * @public
     */
    async getAvailableRoutines() {
      const response =
          /** @type {dpsl_internal.DiagnosticsGetAvailableRoutinesResponse} */ (
              await messagePipe.sendMessage(
                  dpsl_internal.Message.DIAGNOSTICS_AVAILABLE_ROUTINES));
      return response.availableRoutines;
    }
  };

  /**
   * DPSL Telemetry Requester.
   */
  class TelemetryRequester {
    constructor() {
      const categoryEnum = chromeos.health.mojom.ProbeCategoryEnum;

      /**
       * @type { !Map<!string, !chromeos.health.mojom.ProbeCategoryEnum> }
       * @const
       * @private
       */
      this.categoryToEnum_ = new Map([
        ['battery', categoryEnum.kBattery],
        ['non-removable-block-devices', categoryEnum.kNonRemovableBlockDevices],
        ['cached-vpd-data', categoryEnum.kCachedVpdData],
        ['cpu', categoryEnum.kCpu], ['timezone', categoryEnum.kTimezone],
        ['memory', categoryEnum.kMemory],
        ['backlight', categoryEnum.kBacklight], ['fan', categoryEnum.kFan],
        ['stateful-partition', categoryEnum.kStatefulPartition],
        ['bluetooth', categoryEnum.kBluetooth]
      ]);
    }

    /**
     * @param { !Array<!string> } categories
     * @return { !Array<!chromeos.health.mojom.ProbeCategoryEnum> }
     * @private
     */
    convertCategories(categories) {
      return categories.map((category) => {
        if (!this.categoryToEnum_.has(category)) {
          throw TypeError(`Telemetry category '${category}' is unknown.`);
        }
        return this.categoryToEnum_.get(category);
      });
    }

    /**
     * Requests telemetry info.
     * @param { !Array<!string> } categories
     * @return { !Promise<!chromeos.health.mojom.TelemetryInfo> }
     * @public
     */
    async probeTelemetryInfo(categories) {
      const response =
          /** @type {dpsl_internal.ProbeTelemetryInfoResponse} */ (
              await messagePipe.sendMessage(
                  dpsl_internal.Message.PROBE_TELEMETRY_INFO,
                  this.convertCategories(categories)));
      return response.telemetryInfo;
    }
  };

  globalThis.chromeos.diagnostics = new DiagnosticsRequester();
  globalThis.chromeos.telemetry = new TelemetryRequester();

  globalThis.chromeos.test_support.messagePipe = function() {
    console.warn(
        'messagePipe() is a method for testing purposes only. Please',
        'do not use it, otherwise your app may be broken in the future.');
    return messagePipe;
  }
})();
