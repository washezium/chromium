// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.password_check;

import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.is;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.notNull;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.CompromisedCredentialProperties.COMPROMISED_CREDENTIAL;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.CompromisedCredentialProperties.CREDENTIAL_HANDLER;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.CompromisedCredentialProperties.HAS_MANUAL_CHANGE_BUTTON;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.DELETION_CONFIRMATION_HANDLER;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.CHECK_PROGRESS;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.CHECK_STATUS;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.CHECK_TIMESTAMP;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.COMPROMISED_CREDENTIALS_COUNT;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.LAUNCH_ACCOUNT_CHECKUP_ACTION;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.RESTART_BUTTON_ACTION;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.HeaderProperties.UNKNOWN_PROGRESS;
import static org.chromium.chrome.browser.password_check.PasswordCheckProperties.ITEMS;
import static org.chromium.chrome.browser.password_check.PasswordCheckUIStatus.CANCELED;
import static org.chromium.chrome.browser.password_check.PasswordCheckUIStatus.ERROR_OFFLINE;
import static org.chromium.chrome.browser.password_check.PasswordCheckUIStatus.ERROR_UNKNOWN;
import static org.chromium.chrome.browser.password_check.PasswordCheckUIStatus.IDLE;
import static org.chromium.chrome.browser.password_check.PasswordCheckUIStatus.RUNNING;

import android.content.DialogInterface;
import android.util.Pair;

import androidx.appcompat.app.AlertDialog;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import org.chromium.base.Callback;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.password_check.PasswordCheckProperties.ItemType;
import org.chromium.chrome.browser.password_check.helper.PasswordCheckChangePasswordHelper;
import org.chromium.chrome.browser.password_check.helper.PasswordCheckReauthenticationHelper;
import org.chromium.chrome.browser.password_check.helper.PasswordCheckReauthenticationHelper.ReauthReason;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.ui.modelutil.ListModel;
import org.chromium.ui.modelutil.MVCListAdapter;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

/**
 * Controller tests verify that the PasswordCheck controller modifies the model if the API is used
 * properly.
 */
@RunWith(BaseRobolectricTestRunner.class)
@EnableFeatures(ChromeFeatureList.PASSWORD_CHECK)
public class PasswordCheckControllerTest {
    private static final CompromisedCredential ANA =
            new CompromisedCredential("https://m.a.xyz/signin", mock(GURL.class), "Ana", "m.a.xyz",
                    "Ana", "password", "", "xyz.a.some.package", 2, true, false, false);
    private static final CompromisedCredential BOB = new CompromisedCredential(
            "http://www.b.ch/signin", mock(GURL.class), "", "http://www.b.ch", "(No username)",
            "DoneSth", "http://www.b.ch/.well-known/change-password", "", 1, true, false, true);

    @Rule
    public TestRule mFeaturesProcessorRule = new Features.JUnitProcessor();

    private static final Pair<Integer, Integer> PROGRESS_UPDATE = new Pair<>(2, 19);

    @Mock
    private PasswordCheckComponentUi.Delegate mDelegate;
    @Mock
    private PasswordCheckChangePasswordHelper mChangePasswordDelegate;
    @Mock
    private PasswordCheck mPasswordCheck;
    @Mock
    private PasswordCheckReauthenticationHelper mReauthenticationHelper;

    // DO NOT INITIALIZE HERE! The objects would be shared here which leaks state between tests.
    private PasswordCheckMediator mMediator;
    private PropertyModel mModel;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mModel = PasswordCheckProperties.createDefaultModel();
        mMediator = new PasswordCheckMediator(mChangePasswordDelegate, mReauthenticationHelper);
        PasswordCheckFactory.setPasswordCheckForTesting(mPasswordCheck);
        mMediator.initialize(mModel, mDelegate, PasswordCheckReferrer.PASSWORD_SETTINGS, () -> {});
    }

    @Test
    public void testCreatesValidDefaultModel() {
        verify(mPasswordCheck).addObserver(mMediator, true);
        assertNotNull(mModel.get(ITEMS));
    }

    @Test
    public void testAddsAndRemovesFromObserverList() {
        mMediator.destroy();
        verify(mPasswordCheck).removeObserver(mMediator);
    }

    @Test
    public void testInitializeRunningHeaderAndTriggerRunAsSoonAsPossible() {
        assertRunningHeader(mModel.get(ITEMS).get(0), UNKNOWN_PROGRESS);
        verify(mPasswordCheck).startCheck();
    }

    @Test
    public void testInitializeHeaderWithLastStatusWhenComingFromSafetyCheck() {
        clearInvocations(mPasswordCheck); // Clear invocations from setup code.
        when(mPasswordCheck.getCheckStatus()).thenReturn(PasswordCheckUIStatus.IDLE);
        mMediator.initialize(mModel, mDelegate, PasswordCheckReferrer.SAFETY_CHECK, () -> {});
        assertIdleHeader(mModel.get(ITEMS).get(0));
        verify(mPasswordCheck, never()).startCheck();
    }

    @Test
    public void testCreatesHeaderForStatus() {
        mMediator.onPasswordCheckStatusChanged(IDLE);
        ListModel<MVCListAdapter.ListItem> itemList = mModel.get(ITEMS);
        assertThat(itemList.get(0).type, is(ItemType.HEADER));
        assertThat(itemList.get(0).model.get(CHECK_STATUS), is(IDLE));
        assertNotNull(itemList.get(0).model.get(RESTART_BUTTON_ACTION));
    }

    @Test
    public void testUpdateStatusHeaderOnError() {
        assertRunningHeader(mModel.get(ITEMS).get(0), UNKNOWN_PROGRESS);
        mMediator.onPasswordCheckStatusChanged(ERROR_OFFLINE);
        ListModel<MVCListAdapter.ListItem> itemList = mModel.get(ITEMS);
        assertThat(itemList.size(), is(1));
        MVCListAdapter.ListItem header = itemList.get(0);
        assertHeaderTypeWithStatus(header, ERROR_OFFLINE);
        assertNull(header.model.get(CHECK_PROGRESS));
        assertNull(header.model.get(CHECK_TIMESTAMP));
        assertNull(header.model.get(COMPROMISED_CREDENTIALS_COUNT));
    }

    @Test
    public void testUpdateStatusHeaderOnIdle() {
        assertRunningHeader(mModel.get(ITEMS).get(0), UNKNOWN_PROGRESS);
        mMediator.onPasswordCheckStatusChanged(IDLE);
        ListModel<MVCListAdapter.ListItem> itemList = mModel.get(ITEMS);
        assertThat(itemList.size(), is(1));
        assertIdleHeader(itemList.get(0));
    }

    @Test
    public void testUpdateProgressHeader() {
        assertRunningHeader(mModel.get(ITEMS).get(0), UNKNOWN_PROGRESS);
        int already_processed = PROGRESS_UPDATE.first;
        int remaining_in_queue = PROGRESS_UPDATE.second - already_processed;
        mMediator.onPasswordCheckProgressChanged(already_processed, remaining_in_queue);
        assertRunningHeader(mModel.get(ITEMS).get(0), PROGRESS_UPDATE);
    }

    @Test
    public void testEditTriggersCanReauthenticate() {
        mMediator.onEdit(ANA);
        verify(mReauthenticationHelper).canReauthenticate();
    }

    @Test
    public void testCannotReauthenticateDoesNothing() {
        when(mReauthenticationHelper.canReauthenticate()).thenReturn(false);
        mMediator.onEdit(ANA);
        verify(mReauthenticationHelper, never()).reauthenticate(anyInt(), any(Callback.class));
    }

    @Test
    public void testCanReauthenticateTriggersReauthenticate() {
        when(mReauthenticationHelper.canReauthenticate()).thenReturn(true);
        mMediator.onEdit(ANA);
        verify(mReauthenticationHelper)
                .reauthenticate(eq(ReauthReason.EDIT_PASSWORD), any(Callback.class));
    }

    @Test
    public void testCreatesEntryForExistingCredentials() {
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {ANA});
        when(mPasswordCheck.areScriptsRefreshed()).thenReturn(true);
        when(mChangePasswordDelegate.canManuallyChangeCredential(eq(ANA))).thenReturn(true);

        mMediator.onPasswordCheckStatusChanged(IDLE);
        mMediator.onCompromisedCredentialsFetchCompleted();

        assertThat(mModel.get(ITEMS).get(1).type, is(ItemType.COMPROMISED_CREDENTIAL));
        assertThat(mModel.get(ITEMS).get(1).model.get(COMPROMISED_CREDENTIAL), equalTo(ANA));
        assertThat(mModel.get(ITEMS).get(1).model.get(CREDENTIAL_HANDLER), is(mMediator));
        assertThat(mModel.get(ITEMS).get(1).model.get(HAS_MANUAL_CHANGE_BUTTON), is(true));
    }

    @Test
    public void testHidesChangeButtonIfManualChangeIsNotPossible() {
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {BOB});
        when(mPasswordCheck.areScriptsRefreshed()).thenReturn(true);
        when(mChangePasswordDelegate.canManuallyChangeCredential(eq(BOB))).thenReturn(false);

        mMediator.onPasswordCheckStatusChanged(IDLE);
        mMediator.onCompromisedCredentialsFetchCompleted();

        assertThat(mModel.get(ITEMS).get(1).type, is(ItemType.COMPROMISED_CREDENTIAL_WITH_SCRIPT));
        assertThat(mModel.get(ITEMS).get(1).model.get(COMPROMISED_CREDENTIAL), equalTo(BOB));
        assertThat(mModel.get(ITEMS).get(1).model.get(CREDENTIAL_HANDLER), is(mMediator));
        assertThat(mModel.get(ITEMS).get(1).model.get(HAS_MANUAL_CHANGE_BUTTON), is(false));
    }

    @Test
    public void testAppendsEntryForNewlyFoundCredentials() {
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {ANA});
        when(mPasswordCheck.areScriptsRefreshed()).thenReturn(true);
        when(mChangePasswordDelegate.canManuallyChangeCredential(eq(BOB))).thenReturn(true);
        mMediator.onPasswordCheckStatusChanged(IDLE);
        mMediator.onCompromisedCredentialsFetchCompleted();
        assertThat(mModel.get(ITEMS).size(), is(2)); // Header + existing credentials.

        mMediator.onCompromisedCredentialFound(BOB);

        assertThat(mModel.get(ITEMS).get(2).type, is(ItemType.COMPROMISED_CREDENTIAL_WITH_SCRIPT));
        assertThat(mModel.get(ITEMS).get(2).model.get(COMPROMISED_CREDENTIAL), equalTo(BOB));
        assertThat(mModel.get(ITEMS).get(2).model.get(CREDENTIAL_HANDLER), is(mMediator));
        assertThat(mModel.get(ITEMS).get(2).model.get(HAS_MANUAL_CHANGE_BUTTON), is(true));
    }

    @Test
    public void testReplacesEntriesForUpdateOfEntireList() {
        mMediator.onPasswordCheckStatusChanged(IDLE);

        // First call adds only ANA.
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {ANA});
        when(mPasswordCheck.areScriptsRefreshed()).thenReturn(true);
        mMediator.onCompromisedCredentialsFetchCompleted();
        assertThat(mModel.get(ITEMS).size(), is(2)); // Header + existing credentials.

        // Second call adds BOB and removes ANA.
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {BOB});
        mMediator.onCompromisedCredentialsFetchCompleted();
        assertThat(mModel.get(ITEMS).size(), is(2));
        assertThat(mModel.get(ITEMS).get(1).type, is(ItemType.COMPROMISED_CREDENTIAL_WITH_SCRIPT));
        assertThat(mModel.get(ITEMS).get(1).model.get(COMPROMISED_CREDENTIAL), equalTo(BOB));
        assertThat(mModel.get(ITEMS).get(1).model.get(CREDENTIAL_HANDLER), is(mMediator));
    }

    @Test
    public void testIdleStatusUpdatedOnCredentialsFetchCompleted() {
        // Set initial status to IDLE with no compromised credentials.
        when(mPasswordCheck.getCompromisedCredentialsCount()).thenReturn(0);
        mMediator.onPasswordCheckStatusChanged(IDLE);
        assertThat(mModel.get(ITEMS).get(0).model.get(COMPROMISED_CREDENTIALS_COUNT), is(0));

        // Add 2 compromised credentials.
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {ANA, BOB});
        when(mPasswordCheck.areScriptsRefreshed()).thenReturn(true);
        when(mPasswordCheck.getCompromisedCredentialsCount()).thenReturn(2);
        mMediator.onCompromisedCredentialsFetchCompleted();
        assertThat(mModel.get(ITEMS).size(), is(3)); // Header + existing credentials.

        // Check the compromised credentials count updated.
        assertThat(mModel.get(ITEMS).get(0).model.get(COMPROMISED_CREDENTIALS_COUNT), is(2));
    }

    @Test
    public void testCanceledStatusDoesntUpdateModel() {
        assertRunningHeader(mModel.get(ITEMS).get(0), UNKNOWN_PROGRESS);
        mMediator.onPasswordCheckStatusChanged(CANCELED);
        // Check that the header model didn't change.
        assertRunningHeader(mModel.get(ITEMS).get(0), UNKNOWN_PROGRESS);
    }

    @Test
    public void testNotIdleStatusNotUpdatedOnCredentialsFetchCompleted() {
        mMediator.onPasswordCheckStatusChanged(RUNNING);
        assertNull(mModel.get(ITEMS).get(0).model.get(COMPROMISED_CREDENTIALS_COUNT));

        // Add ANA while the check is running.
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {ANA});
        when(mPasswordCheck.areScriptsRefreshed()).thenReturn(true);
        when(mPasswordCheck.getCompromisedCredentialsCount()).thenReturn(1);
        mMediator.onCompromisedCredentialsFetchCompleted();
        assertThat(mModel.get(ITEMS).size(), is(2)); // Header + existing credentials.

        // Check the compromised credential count did not update.
        assertNull(mModel.get(ITEMS).get(0).model.get(COMPROMISED_CREDENTIALS_COUNT));
    }

    @Test
    public void testIdleStatusUpdatedOnCredentialFound() {
        // Set initial status to IDLE with no compromised credentials.
        when(mPasswordCheck.getCompromisedCredentialsCount()).thenReturn(0);
        mMediator.onPasswordCheckStatusChanged(IDLE);
        assertThat(mModel.get(ITEMS).get(0).model.get(COMPROMISED_CREDENTIALS_COUNT), is(0));

        // Add ANA to the compromised credentials.
        when(mPasswordCheck.getCompromisedCredentialsCount()).thenReturn(1);
        mMediator.onCompromisedCredentialFound(ANA);
        assertThat(mModel.get(ITEMS).size(), is(2)); // Header + existing credentials.

        // Check the compromised credentials count updated.
        assertThat(mModel.get(ITEMS).get(0).model.get(COMPROMISED_CREDENTIALS_COUNT), is(1));
    }

    @Test
    public void testNotIdleStatusNotUpdatedOnCredentialFound() {
        mMediator.onPasswordCheckStatusChanged(ERROR_UNKNOWN);
        assertNull(mModel.get(ITEMS).get(0).model.get(COMPROMISED_CREDENTIALS_COUNT));

        // Add ANA after the check has failed.
        when(mPasswordCheck.getCompromisedCredentialsCount()).thenReturn(1);
        mMediator.onCompromisedCredentialFound(ANA);
        assertThat(mModel.get(ITEMS).size(), is(2)); // Header + existing credentials.

        // Check the compromised credentials count did not update.
        assertNull(mModel.get(ITEMS).get(0).model.get(COMPROMISED_CREDENTIALS_COUNT));
    }

    @Test
    public void testSortsInitialSetOfCredentals() {
        mMediator.onPasswordCheckStatusChanged(IDLE);

        CompromisedCredential phishedEarly = makeCredential("example.com", "alice", 1, false, true);
        CompromisedCredential phishedLeakedLate = makeCredential("test.com", "bob", 3, true, true);
        CompromisedCredential leakedEarly = makeCredential("example.org", "alice", 2, true, false);
        CompromisedCredential leakedLate = makeCredential("site.com", "john", 4, true, false);

        when(mPasswordCheck.areScriptsRefreshed()).thenReturn(true);
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {
                        phishedEarly, leakedEarly, leakedLate, phishedLeakedLate});
        mMediator.onCompromisedCredentialsFetchCompleted();

        assertThat(mModel.get(ITEMS).size(), is(5));
        assertThat(
                mModel.get(ITEMS).get(1).model.get(COMPROMISED_CREDENTIAL), is(phishedLeakedLate));
        assertThat(mModel.get(ITEMS).get(2).model.get(COMPROMISED_CREDENTIAL), is(phishedEarly));
        assertThat(mModel.get(ITEMS).get(3).model.get(COMPROMISED_CREDENTIAL), is(leakedLate));
        assertThat(mModel.get(ITEMS).get(4).model.get(COMPROMISED_CREDENTIAL), is(leakedEarly));
    }

    @Test
    public void testSortsAppendedCredentials() {
        mMediator.onPasswordCheckStatusChanged(IDLE);

        CompromisedCredential phishedEarly = makeCredential("example.com", "alice", 1, false, true);
        CompromisedCredential phishedLeakedLate = makeCredential("test.com", "bob", 3, true, true);
        CompromisedCredential leakedEarly = makeCredential("example.org", "alice", 2, true, false);
        CompromisedCredential leakedLate = makeCredential("site.com", "john", 4, true, false);

        when(mPasswordCheck.areScriptsRefreshed()).thenReturn(true);

        // Send the initial set of credentials (to simulate loading them from disk).
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {
                        phishedEarly, leakedEarly, leakedLate, phishedLeakedLate});
        mMediator.onCompromisedCredentialsFetchCompleted();

        // Send an updated list simulating credentials found in the current check.
        CompromisedCredential leakedNewEarly1 =
                makeCredential("example.com", "john", 5, true, false);
        CompromisedCredential leakedNewEarly2 = makeCredential("test.com", "john", 5, true, false);
        CompromisedCredential leakedNewLate = makeCredential("site.org", "alice", 6, true, false);
        when(mPasswordCheck.getCompromisedCredentials())
                .thenReturn(new CompromisedCredential[] {phishedEarly, leakedEarly, leakedLate,
                        leakedNewEarly2, leakedNewLate, leakedNewEarly1, phishedLeakedLate});
        mMediator.onCompromisedCredentialsFetchCompleted();

        // Expect that the order of the original set has been maintained and that the newly found
        // leaked credentials appear at the end in ascending order of creation time (or in ascending
        // alphabetical order for equal times).
        assertThat(mModel.get(ITEMS).size(), is(8));
        assertThat(
                mModel.get(ITEMS).get(1).model.get(COMPROMISED_CREDENTIAL), is(phishedLeakedLate));
        assertThat(mModel.get(ITEMS).get(2).model.get(COMPROMISED_CREDENTIAL), is(phishedEarly));
        assertThat(mModel.get(ITEMS).get(3).model.get(COMPROMISED_CREDENTIAL), is(leakedLate));
        assertThat(mModel.get(ITEMS).get(4).model.get(COMPROMISED_CREDENTIAL), is(leakedEarly));
        assertThat(mModel.get(ITEMS).get(5).model.get(COMPROMISED_CREDENTIAL), is(leakedNewEarly1));
        assertThat(mModel.get(ITEMS).get(6).model.get(COMPROMISED_CREDENTIAL), is(leakedNewEarly2));
        assertThat(mModel.get(ITEMS).get(7).model.get(COMPROMISED_CREDENTIAL), is(leakedNewLate));
    }

    @Test
    public void testRemovingElementTriggersDelegate() {
        // Removing sets a valid handler:
        mMediator.onRemove(ANA);
        assertNotNull(mModel.get(DELETION_CONFIRMATION_HANDLER));

        // When the handler is triggered (because the dialog was confirmed), remove the credential:
        mModel.get(DELETION_CONFIRMATION_HANDLER)
                .onClick(mock(DialogInterface.class), AlertDialog.BUTTON_POSITIVE);
        verify(mDelegate).removeCredential(eq(ANA));
        assertNull(mModel.get(DELETION_CONFIRMATION_HANDLER));
    }

    @Test
    public void testOnChangePasswordButtonClick() {
        mMediator.onChangePasswordButtonClick(ANA);
        verify(mChangePasswordDelegate).launchAppOrCctWithChangePasswordUrl(eq(ANA));
    }

    @Test
    public void testOnChangePasswordWithScriptButtonClick() {
        mMediator.onChangePasswordWithScriptButtonClick(BOB);
        verify(mChangePasswordDelegate).launchCctWithScript(eq(BOB));
    }

    @Test
    public void testOnEditPasswordButtonClick() {
        when(mReauthenticationHelper.canReauthenticate()).thenReturn(true);
        doAnswer(invocation -> {
            Callback<Boolean> cb = invocation.getArgument(1);
            cb.onResult(true);
            return true;
        })
                .when(mReauthenticationHelper)
                .reauthenticate(anyInt(), notNull());
        mMediator.onEdit(ANA);
        verify(mChangePasswordDelegate).launchEditPage(eq(ANA));
    }

    private void assertIdleHeader(MVCListAdapter.ListItem header) {
        assertHeaderTypeWithStatus(header, IDLE);
        assertNull(header.model.get(CHECK_PROGRESS));
        assertNotNull(header.model.get(CHECK_TIMESTAMP));
        assertNotNull(header.model.get(COMPROMISED_CREDENTIALS_COUNT));
    }

    private void assertRunningHeader(
            MVCListAdapter.ListItem header, Pair<Integer, Integer> progress) {
        assertHeaderTypeWithStatus(header, RUNNING);
        assertThat(header.model.get(CHECK_PROGRESS), is(progress));
        assertNull(header.model.get(CHECK_TIMESTAMP));
        assertNull(header.model.get(COMPROMISED_CREDENTIALS_COUNT));
    }

    private void assertHeaderTypeWithStatus(
            MVCListAdapter.ListItem header, @PasswordCheckUIStatus int status) {
        assertThat(header.type, is(ItemType.HEADER));
        assertThat(header.model.get(CHECK_STATUS), is(status));
        assertNotNull(header.model.get(RESTART_BUTTON_ACTION));
        assertNotNull(header.model.get(LAUNCH_ACCOUNT_CHECKUP_ACTION));
    }

    private CompromisedCredential makeCredential(
            String origin, String username, long creationTime, boolean leaked, boolean phished) {
        return new CompromisedCredential(origin, mock(GURL.class), username, origin, username,
                "password", origin, new String(), creationTime, leaked, phished, false);
    }
}
