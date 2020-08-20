// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.password_check;

import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.CompromisedCredentialProperties.COMPROMISED_CREDENTIAL;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.CompromisedCredentialProperties.CREDENTIAL_HANDLER;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.CompromisedCredentialProperties.HAS_MANUAL_CHANGE_BUTTON;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.DELETION_CONFIRMATION_HANDLER;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.DELETION_ORIGIN;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.CHECK_PROGRESS;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.CHECK_STATUS;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.CHECK_TIMESTAMP;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.COMPROMISED_CREDENTIALS_COUNT;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.LAUNCH_ACCOUNT_CHECKUP_ACTION;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.RESTART_BUTTON_ACTION;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.UNKNOWN_PROGRESS;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.ITEMS;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.VIEW_CREDENTIAL;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.VIEW_DIALOG_HANDLER;

import android.content.DialogInterface;
import android.util.Pair;

import androidx.appcompat.app.AlertDialog;

import org.chromium.chrome.browser.password_check.helper.PasswordCheckChangePasswordHelper;
import org.chromium.chrome.browser.password_check.helper.PasswordCheckReauthenticationHelper;
import org.chromium.chrome.browser.password_check.helper.PasswordCheckReauthenticationHelper.ReauthReason;
import org.chromium.ui.modelutil.ListModel;
import org.chromium.ui.modelutil.MVCListAdapter.ListItem;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;

/**
 * Contains the logic for the PasswordCheck component. It sets the state of the model and reacts to
 * events like clicks.
 */
class PasswordCheckMediator
        implements PasswordCheckCoordinator.CredentialEventHandler, PasswordCheck.Observer {
    private final PasswordCheckReauthenticationHelper mReauthenticationHelper;
    private final PasswordCheckChangePasswordHelper mChangePasswordDelegate;
    private PropertyModel mModel;
    private PasswordCheckComponentUi.Delegate mDelegate;
    private Runnable mLaunchCheckupInAccount;
    private HashSet<CompromisedCredential> mPreCheckSet;

    PasswordCheckMediator(PasswordCheckChangePasswordHelper changePasswordDelegate,
            PasswordCheckReauthenticationHelper reauthenticationHelper) {
        mChangePasswordDelegate = changePasswordDelegate;
        mReauthenticationHelper = reauthenticationHelper;
    }

    void initialize(PropertyModel model, PasswordCheckComponentUi.Delegate delegate,
            @PasswordCheckReferrer int passwordCheckReferrer, Runnable launchCheckupInAccount) {
        mModel = model;
        mDelegate = delegate;
        mLaunchCheckupInAccount = launchCheckupInAccount;

        // If a run is scheduled to happen soon, initialize the UI as running to prevent flickering.
        // Otherwise, initialize the UI with last known state (defaults to IDLE before first run).
        boolean shouldRunCheck = passwordCheckReferrer != PasswordCheckReferrer.SAFETY_CHECK;
        onPasswordCheckStatusChanged(shouldRunCheck ? PasswordCheckUIStatus.RUNNING
                                                    : getPasswordCheck().getCheckStatus());
        getPasswordCheck().addObserver(this, true);
        if (shouldRunCheck) getPasswordCheck().startCheck();
    }

    void destroy() {
        getPasswordCheck().removeObserver(this);
    }

    @Override
    public void onCompromisedCredentialsFetchCompleted() {
        if (!getPasswordCheck().areScriptsRefreshed()) {
            return;
        }
        CompromisedCredential[] credentials = getPasswordCheck().getCompromisedCredentials();
        assert credentials != null;

        List<CompromisedCredential> credentialsList = Arrays.asList(credentials);
        sortCredentials(credentialsList);

        ListModel<ListItem> items = mModel.get(ITEMS);
        if (items.size() == 0) {
            items.add(new ListItem(PasswordCheckProperties.ItemType.HEADER,
                    new PropertyModel.Builder(PasswordCheckProperties.HeaderProperties.ALL_KEYS)
                            .with(CHECK_STATUS, PasswordCheckUIStatus.RUNNING)
                            .with(LAUNCH_ACCOUNT_CHECKUP_ACTION, mLaunchCheckupInAccount)
                            .with(RESTART_BUTTON_ACTION, this::runCheck)
                            .build()));
        }
        if (items.size() > 1) items.removeRange(1, items.size() - 1);

        updateStatusHeaderWhenCredentialsChange();
        for (CompromisedCredential credential : credentialsList) {
            items.add(createEntryForCredential(credential));
        }
    }

    @Override
    public void onSavedPasswordsFetchCompleted() {}

    @Override
    public void onPasswordCheckStatusChanged(@PasswordCheckUIStatus int status) {
        ListModel<ListItem> items = mModel.get(ITEMS);
        PropertyModel header;
        if (items.size() == 0) {
            header = new PropertyModel.Builder(PasswordCheckProperties.HeaderProperties.ALL_KEYS)
                             .with(CHECK_PROGRESS, UNKNOWN_PROGRESS)
                             .with(CHECK_STATUS, PasswordCheckUIStatus.RUNNING)
                             .with(CHECK_TIMESTAMP, null)
                             .with(COMPROMISED_CREDENTIALS_COUNT, null)
                             .with(LAUNCH_ACCOUNT_CHECKUP_ACTION, mLaunchCheckupInAccount)
                             .with(RESTART_BUTTON_ACTION, this::runCheck)
                             .build();
        } else {
            header = items.get(0).model;
        }
        header.set(CHECK_STATUS, status);
        header.set(
                CHECK_PROGRESS, status == PasswordCheckUIStatus.RUNNING ? UNKNOWN_PROGRESS : null);
        Long checkTimestamp = null;
        Integer compromisedCredentialCount = null;
        if (status == PasswordCheckUIStatus.IDLE) {
            compromisedCredentialCount = getPasswordCheck().getCompromisedCredentialsCount();
            checkTimestamp = getPasswordCheck().getLastCheckTimestamp();
        }
        header.set(CHECK_TIMESTAMP, checkTimestamp);
        header.set(COMPROMISED_CREDENTIALS_COUNT, compromisedCredentialCount);

        if (items.size() == 0) {
            items.add(new ListItem(PasswordCheckProperties.ItemType.HEADER, header));
        }
    }

    @Override
    public void onPasswordCheckProgressChanged(int alreadyProcessed, int remainingInQueue) {
        ListModel<ListItem> items = mModel.get(ITEMS);
        assert items.size() >= 1;
        assert alreadyProcessed >= 0;
        assert remainingInQueue >= 0;

        PropertyModel header = items.get(0).model;
        header.set(CHECK_STATUS, PasswordCheckUIStatus.RUNNING);
        header.set(
                CHECK_PROGRESS, new Pair<>(alreadyProcessed, alreadyProcessed + remainingInQueue));
        header.set(CHECK_TIMESTAMP, null);
        header.set(COMPROMISED_CREDENTIALS_COUNT, null);
    }

    @Override
    public void onCompromisedCredentialFound(CompromisedCredential leakedCredential) {
        assert leakedCredential != null;
        ListModel<ListItem> items = mModel.get(ITEMS);
        assert items.size() >= 1 : "Needs to initialize list with header before adding items!";
        updateStatusHeaderWhenCredentialsChange();
        items.add(createEntryForCredential(leakedCredential));
    }

    @Override
    public void onEdit(CompromisedCredential credential) {
        if (!mReauthenticationHelper.canReauthenticate()) {
            mReauthenticationHelper.showScreenLockToast();
            return;
        }

        mReauthenticationHelper.reauthenticate(ReauthReason.EDIT_PASSWORD, reauthSucceeded -> {
            if (reauthSucceeded) mChangePasswordDelegate.launchEditPage(credential);
        });
    }

    @Override
    public void onRemove(CompromisedCredential credential) {
        mModel.set(DELETION_ORIGIN, credential.getDisplayOrigin());
        mModel.set(
                DELETION_CONFIRMATION_HANDLER, new PasswordCheckDeletionDialogFragment.Handler() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (which != AlertDialog.BUTTON_POSITIVE) return;
                        mDelegate.removeCredential(credential);
                        mModel.set(DELETION_CONFIRMATION_HANDLER, null);
                        mModel.set(DELETION_ORIGIN, null);
                    }

                    @Override
                    public void onDismiss() {
                        mModel.set(DELETION_CONFIRMATION_HANDLER, null);
                    }
                });
    }

    @Override
    public void onView(CompromisedCredential credential) {
        if (!mReauthenticationHelper.canReauthenticate()) {
            mReauthenticationHelper.showScreenLockToast();
            return;
        }

        mReauthenticationHelper.reauthenticate(ReauthReason.VIEW_PASSWORD, reauthSucceeded -> {
            if (reauthSucceeded) {
                mModel.set(VIEW_CREDENTIAL, credential);
                mModel.set(VIEW_DIALOG_HANDLER, new PasswordCheckViewDialogFragment.Handler() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        mModel.set(VIEW_CREDENTIAL, null);
                        mModel.set(VIEW_DIALOG_HANDLER, null);
                    }

                    @Override
                    public void onDismiss() {
                        mModel.set(VIEW_DIALOG_HANDLER, null);
                    }
                });
            }
        });
    }

    @Override
    public void onChangePasswordButtonClick(CompromisedCredential credential) {
        mChangePasswordDelegate.launchAppOrCctWithChangePasswordUrl(credential);
    }

    @Override
    public void onChangePasswordWithScriptButtonClick(CompromisedCredential credential) {
        assert credential.hasScript();
        mChangePasswordDelegate.launchCctWithScript(credential);
    }

    private void updateStatusHeaderWhenCredentialsChange() {
        ListModel<ListItem> items = mModel.get(ITEMS);
        assert items.size() >= 1;

        PropertyModel header = items.get(0).model;
        if (header.get(CHECK_STATUS) == PasswordCheckUIStatus.IDLE) {
            header.set(COMPROMISED_CREDENTIALS_COUNT,
                    Integer.valueOf(getPasswordCheck().getCompromisedCredentialsCount()));
        }
    }

    private void runCheck() {
        getPasswordCheck().startCheck();
    }

    private PasswordCheck getPasswordCheck() {
        PasswordCheck passwordCheck = PasswordCheckFactory.getOrCreate();
        assert passwordCheck != null : "Password Check UI component needs native counterpart!";
        return passwordCheck;
    }

    private ListItem createEntryForCredential(CompromisedCredential credential) {
        return new ListItem(credential.hasScript()
                        ? PasswordCheckProperties.ItemType.COMPROMISED_CREDENTIAL_WITH_SCRIPT
                        : PasswordCheckProperties.ItemType.COMPROMISED_CREDENTIAL,
                new PropertyModel
                        .Builder(PasswordCheckProperties.CompromisedCredentialProperties.ALL_KEYS)
                        .with(COMPROMISED_CREDENTIAL, credential)
                        .with(HAS_MANUAL_CHANGE_BUTTON,
                                mChangePasswordDelegate.canManuallyChangeCredential(credential))
                        .with(CREDENTIAL_HANDLER, this)
                        .build());
    }

    private void sortCredentials(List<CompromisedCredential> credentials) {
        if (mPreCheckSet == null) {
            mPreCheckSet = new HashSet<>(credentials);
        }

        Collections.sort(credentials, (CompromisedCredential lhs, CompromisedCredential rhs) -> {
            // Phished credentials should always appear first.
            if (lhs.isPhished() != rhs.isPhished()) {
                return lhs.isPhished() ? -1 : 1;
            }

            boolean lhsInitial = mPreCheckSet.contains(lhs);
            boolean rhsInitial = mPreCheckSet.contains(rhs);
            // If one is the in initial set and the other one isn't, then the credential in
            // the initial set goes first.
            if (lhsInitial != rhsInitial) {
                return lhsInitial ? -1 : 1;
            }

            // If they are both in the initial set, the most recent credential should appear first.
            if (lhsInitial && rhsInitial && lhs.getCreationTime() != rhs.getCreationTime()) {
                return -Long.compare(lhs.getCreationTime(), rhs.getCreationTime());
            }

            // If they both are not in the initial set, the older credential should appear
            // first.
            if (!lhsInitial && !rhsInitial && lhs.getCreationTime() != rhs.getCreationTime()) {
                return Long.compare(lhs.getCreationTime(), rhs.getCreationTime());
            }

            // In case of creation time equality, order alphabetically (first by origin,
            // then by username), so that the list remains stable.
            int originComparisonResult = lhs.getDisplayOrigin().compareTo(rhs.getDisplayOrigin());
            int usernameComparisonResult =
                    lhs.getDisplayUsername().compareTo(rhs.getDisplayUsername());
            return originComparisonResult == 0 ? usernameComparisonResult : originComparisonResult;
        });
    }
}
