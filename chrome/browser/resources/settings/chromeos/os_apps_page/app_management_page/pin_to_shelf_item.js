// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
Polymer({
  is: 'app-management-pin-to-shelf-item',

  properties: {
    /**
     * @type {App}
     */
    app: Object,

    /**
     * @type {boolean}
     */
    hidden: {
      type: Boolean,
      computed: 'isAvailable_(app)',
      reflectToAttribute: true,
    },

    /**
     * @type {boolean}
     */
    disabled: {
      type: Boolean,
      computed: 'isManaged_(app)',
      reflectToAttribute: true,
    },
  },

  listeners: {
    click: 'onClick_',
    change: 'toggleSetting_',
  },

  /**
   * @param {App} app
   * @returns {boolean} true if the app is pinned
   * @private
   */
  getValue_(app) {
    if (app === undefined) {
      return false;
    }
    assert(app);
    return app.isPinned === OptionalBool.kTrue;
  },

  /**
   * @param {App} app
   * @returns {boolean} true if pinning is available.
   */
  isAvailable_(app) {
    if (app === undefined) {
      return false;
    }
    assert(app);
    return app.hidePinToShelf;
  },

  /**
   * @param {App} app
   * @returns {boolean} true if the pinning is managed by policy.
   * @private
   */
  isManaged_(app) {
    if (app === undefined) {
      return false;
    }
    assert(app);
    return app.isPolicyPinned === OptionalBool.kTrue;
  },

  toggleSetting_() {
    const newState =
        assert(app_management.util.toggleOptionalBool(this.app.isPinned));
    const newStateBool =
        app_management.util.convertOptionalBoolToBool(newState);
    assert(newStateBool === this.$['toggle-row'].isChecked());
    app_management.BrowserProxy.getInstance().handler.setPinned(
        this.app.id,
        newState,
    );
    settings.recordSettingChange();
    const userAction = newStateBool ?
        AppManagementUserAction.PinToShelfTurnedOn :
        AppManagementUserAction.PinToShelfTurnedOff;
    app_management.util.recordAppManagementUserAction(
        this.app.type, userAction);
  },

  /**
   * @private
   */
  onClick_() {
    this.$['toggle-row'].click();
  },
});
