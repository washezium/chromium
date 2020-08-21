// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.keyboard_accessory.all_passwords_bottom_sheet;

import static org.chromium.chrome.browser.keyboard_accessory.all_passwords_bottom_sheet.AllPasswordsBottomSheetProperties.VISIBLE;

import org.chromium.ui.modelutil.PropertyModel;

/**
 * Contains the logic for the AllPasswordsBottomSheet. It sets the state of the model and reacts to
 * events like clicks.
 */
class AllPasswordsBottomSheetMediator {
    private AllPasswordsBottomSheetCoordinator.Delegate mDelegate;
    private PropertyModel mModel;

    void initialize(AllPasswordsBottomSheetCoordinator.Delegate delegate, PropertyModel model) {
        assert delegate != null;
        mDelegate = delegate;
        mModel = model;
    }

    void showCredentials(Credential[] credentials) {
        assert credentials != null;
        mModel.set(VISIBLE, true);
    }

    void onCredentialSelected(Credential credential) {
        mModel.set(VISIBLE, false);
        mDelegate.onCredentialSelected(credential);
    }

    void onDismissed(Integer integer) {
        if (!mModel.get(VISIBLE)) return; // Dismiss only if not dismissed yet.
        mModel.set(VISIBLE, false);
        mDelegate.onDismissed();
    }
}
