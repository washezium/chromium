// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin.account_picker;

import android.accounts.AccountManager;
import android.app.Activity;
import android.content.Intent;

import androidx.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.task.PostTask;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.signin.IdentityServicesProvider;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.signin.SigninUtils;
import org.chromium.chrome.browser.signin.WebSigninBridge;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.signin.AccountManagerFacadeProvider;
import org.chromium.components.signin.base.CoreAccountInfo;
import org.chromium.components.signin.base.GoogleServiceAuthError;
import org.chromium.components.signin.metrics.SigninAccessPoint;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.ui.base.WindowAndroid;

/**
 * This class is used in web sign-in flow for the account picker bottom sheet.
 *
 * It is responsible for the sign-in and adding account functions needed for the
 * web sign-in flow.
 */
public class AccountPickerDelegate implements WebSigninBridge.Listener {
    private final WindowAndroid mWindowAndroid;
    private final ChromeActivity mChromeActivity;
    private final Tab mTab;
    private final String mContinueUrl;
    private final SigninManager mSigninManager;
    private WebSigninBridge mWebSigninBridge;
    private Callback<GoogleServiceAuthError> mOnSignInErrorCallback;

    public AccountPickerDelegate(WindowAndroid windowAndroid, String continueUrl) {
        mWindowAndroid = windowAndroid;
        mChromeActivity = (ChromeActivity) mWindowAndroid.getActivity().get();
        mTab = mChromeActivity.getActivityTab();
        mContinueUrl = continueUrl;
        mSigninManager = IdentityServicesProvider.get().getSigninManager(
                Profile.getLastUsedRegularProfile());
    }

    /**
     * Releases resources used by this class.
     */
    public void onDismiss() {
        if (mWebSigninBridge != null) {
            mWebSigninBridge.destroy();
            mWebSigninBridge = null;
        }
    }

    /**
     * Signs the user into the given account.
     */
    public void signIn(CoreAccountInfo coreAccountInfo,
            Callback<GoogleServiceAuthError> onSignInErrorCallback) {
        mOnSignInErrorCallback = onSignInErrorCallback;
        mSigninManager.signIn(
                SigninAccessPoint.WEB_SIGNIN, coreAccountInfo, new SigninManager.SignInCallback() {
                    @Override
                    public void onSignInComplete() {
                        // TODO(https://crbug.com/1092399): Implement sign-in properly in delegate
                        // We should wait for the cookie signin and cookie regeneration here,
                        // PostTask.postDelayedTask is just a temporary measure for testing the
                        // flow, it will be removed soon.
                        PostTask.postDelayedTask(UiThreadTaskTraits.DEFAULT,
                                () -> { mTab.loadUrl(new LoadUrlParams(mContinueUrl)); }, 2000);
                    }

                    @Override
                    public void onSignInAborted() {
                        // TODO(https//crbug.com/1092399): Add UI to show in this case
                    }
                });
    }

    /**
     * Notifies when the user clicked the "add account" button.
     */
    public void addAccount(Callback<String> callback) {
        AccountManagerFacadeProvider.getInstance().createAddAccountIntent(
                (@Nullable Intent intent) -> {
                    if (intent != null) {
                        WindowAndroid.IntentCallback intentCallback =
                                new WindowAndroid.IntentCallback() {
                                    @Override
                                    public void onIntentCompleted(
                                            WindowAndroid window, int resultCode, Intent data) {
                                        if (resultCode == Activity.RESULT_OK) {
                                            callback.onResult(data.getStringExtra(
                                                    AccountManager.KEY_ACCOUNT_NAME));
                                        }
                                    }
                                };
                        mWindowAndroid.showIntent(intent, intentCallback, null);
                    } else {
                        // AccountManagerFacade couldn't create intent, use SigninUtils to open
                        // settings instead.
                        SigninUtils.openSettingsForAllAccounts(mChromeActivity);
                    }
                });
    }

    /**
     * Notifies when the user clicked the "Go incognito mode" button.
     */
    public void goIncognitoMode() {}

    /**
     * TODO(https://crbug.com/1092399): Redirect URL when web sign-in succeeded.
     * Sign-in completed successfully and the primary account is available in the cookie jar.
     */
    @Override
    public void onSigninSucceded() {}

    /**
     * Sign-in process failed.
     *
     * @param error Details about the error that occurred in the sign-in process.
     */
    @Override
    public void onSigninFailed(GoogleServiceAuthError error) {
        mOnSignInErrorCallback.onResult(error);
    }
}
