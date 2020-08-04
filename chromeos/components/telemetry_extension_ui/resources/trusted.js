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

  const untrustedMessagePipe =
      new MessagePipe('chrome-untrusted://telemetry-extension');

  untrustedMessagePipe.registerHandler(
      dpsl_internal.Message.DIAGNOSTICS_AVAILABLE_ROUTINES, async () => {
        return await getOrCreateDiagnosticsService().getAvailableRoutines();
      });

  untrustedMessagePipe.registerHandler(
      dpsl_internal.Message.PROBE_TELEMETRY_INFO, async (message) => {
        const request =
            /** @type { !dpsl_internal.ProbeTelemetryInfoRequest } */ (message);
        return await getOrCreateProbeService().probeTelemetryInfo(request);
      });

  globalThis.untrustedMessagePipe = untrustedMessagePipe;
});
