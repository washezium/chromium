// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @type { MessagePipe|null }
 */
var untrustedMessagePipe = null;

document.addEventListener('DOMContentLoaded', async () => {
  let diagnosticsService = null;
  let probeService = null;

  /**
   * Lazy creates pointer to remote implementation of diagnostics service.
   * @return {!chromeos.health.mojom.DiagnosticsServiceRemote}
   */
  function getOrCreateDiagnosticsService() {
    if (diagnosticsService === null) {
      diagnosticsService = chromeos.health.mojom.DiagnosticsService.getRemote();
    }
    return /** @type {!chromeos.health.mojom.DiagnosticsServiceRemote} */ (
        diagnosticsService);
  }

  /**
   * Lazy creates pointer to remote implementation of probe service.
   * @return {!chromeos.health.mojom.ProbeServiceRemote}
   */
  function getOrCreateProbeService() {
    if (probeService === null) {
      probeService = chromeos.health.mojom.ProbeService.getRemote();
    }
    return /** @type {!chromeos.health.mojom.ProbeServiceRemote} */ (
        probeService);
  }

  /**
   * Proxying telemetry requests between TelemetryRequester on
   * chrome-untrusted:// side with WebIDL types and ProbeService on chrome://
   * side with Mojo types.
   */
  class TelemetryProxy {
    constructor() {
      const categoryEnum = chromeos.health.mojom.ProbeCategoryEnum;

      /**
       * @type { !Map<!string, !chromeos.health.mojom.ProbeCategoryEnum> }
       * @const
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
     * @param { Object } message
     * @return { dpsl_internal.ProbeTelemetryInfoResponse }
     */
    async handleProbeTelemetryInfo(message) {
      const request =
          /** @type {!dpsl_internal.ProbeTelemetryInfoRequest} */ (message);

      /** @type {!Array<!chromeos.health.mojom.ProbeCategoryEnum>} */
      let categories = [];
      try {
        categories = telemetryProxy.convertCategories(request);
      } catch (/** @type {!Error}*/ error) {
        return {error};
      }

      return await getOrCreateProbeService().probeTelemetryInfo(categories);
    }
  };

  const telemetryProxy = new TelemetryProxy();

  const untrustedMessagePipe =
      new MessagePipe('chrome-untrusted://telemetry-extension');

  untrustedMessagePipe.registerHandler(
      dpsl_internal.Message.DIAGNOSTICS_AVAILABLE_ROUTINES, async () => {
        return await getOrCreateDiagnosticsService().getAvailableRoutines();
      });

  untrustedMessagePipe.registerHandler(
      dpsl_internal.Message.PROBE_TELEMETRY_INFO,
      (message) => telemetryProxy.handleProbeTelemetryInfo(message));

  globalThis.untrustedMessagePipe = untrustedMessagePipe;
});
