// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Root element for the cellular setup flow. This element wraps
 * the psim setup flow, esim setup flow, and setup flow selection page.
 */
Polymer({
  is: 'cellular-setup',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * Name of the currently displayed sub-page.
     * @private {!cellularSetup.CellularSetupPageName|null}
     */
    currentPageName_: {
      type: String,
      value: cellularSetup.CellularSetupPageName.SETUP_FLOW_SELECTION,
    },

    /**
     * Current user selected setup flow page name.
     * @private {!cellularSetup.CellularSetupPageName|null}
     */
    selectedFlow_: {
      type: String,
      value: null,
    },

    /**
     * Button bar button state.
     * @private {!cellularSetup.ButtonBarState}
     */
    buttonState_: {
      type: Object,
      notify: true,
    },

    /**
     * DOM Element corresponding to the visible page.
     *
     * @private {!SetupSelectionFlowElement|!PsimFlowUiElement|
     *           !EsimFlowUiElement}
     */
    currentPage_: {
      type: Object,
      observer: 'onPageChange_',
    }
  },

  listeners: {
    'backward-nav-requested': 'onBackwardNavRequested_',
    'retry-requested': 'onRetryRequested_',
    'complete-flow-requested': 'onCompleteFlowRequested_',
    'forward-nav-requested': 'onForwardNavRequested_',
  },

  /** @private */
  onPageChange_() {
    this.currentPage_.initSubflow();
  },

  /** @private */
  onBackwardNavRequested_() {
    const isNavHandled = this.currentPage_.attemptBackwardNavigation();

    // Subflow returns false in a state where it cannot perform backward
    // navigation any more. Switch back to the selection flow in this case so
    // that the user can select a flow again.
    if (!isNavHandled) {
      this.currentPageName_ =
          cellularSetup.CellularSetupPageName.SETUP_FLOW_SELECTION;
    }
  },

  /** @private */
  onRetryRequested_() {
    // TODO(crbug.com/1093185): Add try again logic.
  },

  /** @private */
  onCompleteFlowRequested_() {
    // TODO(crbug.com/1093185): Add completion logic.
  },

  /** @private */
  onForwardNavRequested_() {
    // Switch current page to user selected flow when navigating forward from
    // setup selection.
    if (this.currentPageName_ ===
        cellularSetup.CellularSetupPageName.SETUP_FLOW_SELECTION) {
      this.currentPageName_ = this.selectedFlow_;
      return;
    }
    this.currentPage_.navigateForward();
  },

  /**
   * @param {string} currentPage
   * @private
   */
  isPSimSelected_(currentPage) {
    return currentPage === cellularSetup.CellularSetupPageName.PSIM_FLOW_UI;
  },

  /**
   * @param {string} currentPage
   * @private
   */
  isESimSelected_(currentPage) {
    return currentPage === cellularSetup.CellularSetupPageName.ESIM_FLOW_UI;
  }
});
