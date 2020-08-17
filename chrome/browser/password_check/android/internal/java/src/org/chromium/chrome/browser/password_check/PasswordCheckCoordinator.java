// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.password_check;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Browser;
import android.view.MenuItem;

import androidx.annotation.VisibleForTesting;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.lifecycle.LifecycleObserver;

import org.chromium.base.IntentUtils;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.help.HelpAndFeedback;
import org.chromium.chrome.browser.password_check.helper.PasswordCheckReauthenticationHelper;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.SettingsLauncher;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

import java.util.Objects;

/**
 * Creates the PasswordCheckComponentUi. This class is responsible for managing the UI for the check
 * of the leaked password.
 */
class PasswordCheckCoordinator implements PasswordCheckComponentUi, LifecycleObserver,
                                          PasswordCheckComponentUi.ChangePasswordDelegate {
    private static final String AUTOFILL_ASSISTANT_PACKAGE =
            "org.chromium.chrome.browser.autofill_assistant.";
    private static final String AUTOFILL_ASSISTANT_ENABLED_KEY =
            AUTOFILL_ASSISTANT_PACKAGE + "ENABLED";
    private static final String PASSWORD_CHANGE_USERNAME_PARAMETER = "PASSWORD_CHANGE_USERNAME";
    private static final String INTENT_PARAMETER = "INTENT";
    private static final String INTENT = "PASSWORD_CHANGE";

    private final PasswordCheckFragmentView mFragmentView;
    private final PasswordCheckReauthenticationHelper mReauthenticationHelper;
    private final PasswordCheckMediator mMediator;
    private PropertyModel mModel;

    /**
     * Blueprint for a class that handles interactions with credentials.
     */
    interface CredentialEventHandler {
        /**
         * Edits the given Credential in the password store.
         * @param credential A {@link CompromisedCredential} to be edited.
         */
        void onEdit(CompromisedCredential credential);

        /**
         * Removes the given Credential from the password store.
         * @param credential A {@link CompromisedCredential} to be removed.
         */
        void onRemove(CompromisedCredential credential);

        /**
         * Opens a password change form or home page of |credential|'s origin or an app.
         * @param credential A {@link CompromisedCredential} to be changed.
         */
        void onChangePasswordButtonClick(CompromisedCredential credential);

        /**
         * Starts a script to change a {@link CompromisedCredential}. Can be called only if {@link
         * CompromisedCredential#hasScript()}.
         * @param credential A {@link CompromisedCredential} to be change with a script.
         */
        void onChangePasswordWithScriptButtonClick(CompromisedCredential credential);
    }

    PasswordCheckCoordinator(PasswordCheckFragmentView fragmentView) {
        mFragmentView = fragmentView;
        // TODO(crbug.com/1101256): If help is part of the view, make mediator the delegate.
        mFragmentView.setComponentDelegate(this);

        // TODO(crbug.com/1092444): Ideally, the following replaces the lifecycle event forwarding.
        //  Figure out why it isn't working and use the following lifecycle observer once it does:
        // mFragmentView.getLifecycle().addObserver(this);

        mReauthenticationHelper = new PasswordCheckReauthenticationHelper(
                mFragmentView.getActivity(), mFragmentView.getParentFragmentManager());

        mMediator = new PasswordCheckMediator(this, mReauthenticationHelper);
    }

    @Override
    public void onStartFragment() {
        // In the rare case of a restarted activity, don't recreate the model and mediator.
        if (mModel == null) {
            mModel = PasswordCheckProperties.createDefaultModel();
            PasswordCheckCoordinator.setUpModelChangeProcessors(mModel, mFragmentView);
            mMediator.initialize(
                    mModel, PasswordCheckFactory.getOrCreate(), mFragmentView.getReferrer());
        }
    }

    @Override
    public void onResumeFragment() {
        mReauthenticationHelper.onReauthenticationMaybeHappened();
    }

    @Override
    public void onDestroyFragment() {
        PasswordCheck check = PasswordCheckFactory.getPasswordCheckInstance();
        if (check != null) check.stopCheck();
        if (mFragmentView.getActivity() == null || mFragmentView.getActivity().isFinishing()) {
            mMediator.destroy();
            mModel = null;
        }
    }

    // TODO(crbug.com/1101256): Move to view code.
    @Override
    public boolean handleHelp(MenuItem item) {
        if (item.getItemId() == org.chromium.chrome.R.id.menu_id_targeted_help) {
            HelpAndFeedback.getInstance().show(mFragmentView.getActivity(),
                    mFragmentView.getActivity().getString(
                            org.chromium.chrome.R.string.help_context_check_passwords),
                    Profile.getLastUsedRegularProfile(), null);
            return true;
        }
        return false;
    }

    @Override
    public void destroy() {
        PasswordCheckFactory.destroy();
    }

    /**
     * Connects the given model with the given view using Model Change Processors.
     * @param model A {@link PropertyModel} built with {@link PasswordCheckProperties}.
     * @param view A {@link PasswordCheckFragmentView}.
     */
    @VisibleForTesting
    static void setUpModelChangeProcessors(PropertyModel model, PasswordCheckFragmentView view) {
        PropertyModelChangeProcessor.create(
                model, view, PasswordCheckViewBinder::bindPasswordCheckView);
    }

    /**
     * Launches a CCT that points to the change password form or home page of |origin|.
     * @param credential A {@link CompromisedCredential} to be changed in an App or in a CCT.
     */
    @Override
    public void launchAppOrCctWithChangePasswordUrl(CompromisedCredential credential) {
        // TODO(crbug.com/1092444): Always launch the URL if possible and let Android handle the
        //  match to open it.
        IntentUtils.safeStartActivity(mFragmentView.getActivity(),
                credential.getAssociatedApp().isEmpty()
                        ? buildIntent(credential.getPasswordChangeUrl())
                        : getPackageLaunchIntent(credential.getAssociatedApp()));
    }

    @Override
    public boolean canManuallyChangeCredential(CompromisedCredential credential) {
        return !credential.getPasswordChangeUrl().isEmpty()
                || getPackageLaunchIntent(credential.getAssociatedApp()) != null;
    }

    /**
     * Launches a CCT that starts a password change script for a {@link CompromisedCredential}.
     * @param credential A {@link CompromisedCredential} to be changed with a script.
     */
    @Override
    public void launchCctWithScript(CompromisedCredential credential) {
        Intent intent = buildIntent(credential.getOrigin().getSpec());
        populateAutofillAssistantExtras(intent, credential.getUsername());
        IntentUtils.safeStartActivity(mFragmentView.getActivity(), intent);
    }

    @Override
    public void launchEditPage(CompromisedCredential credential) {
        SettingsLauncher launcher = new SettingsLauncherImpl();
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putParcelable(
                PasswordCheckEditFragmentView.EXTRA_COMPROMISED_CREDENTIAL, credential);
        launcher.launchSettingsActivity(
                mFragmentView.getContext(), PasswordCheckEditFragmentView.class, fragmentArgs);
    }

    private Intent getPackageLaunchIntent(String packageName) {
        return Objects.requireNonNull(mFragmentView.getActivity())
                .getPackageManager()
                .getLaunchIntentForPackage(packageName);
    }

    /**
     * Builds an intent to launch a CCT.
     * @param initialUrl Initial URL to launch a CCT.
     * @return {@link Intent} for CCT.
     */
    private Intent buildIntent(String initialUrl) {
        final Activity activity = mFragmentView.getActivity();
        CustomTabsIntent customTabIntent =
                new CustomTabsIntent.Builder().setShowTitle(true).build();
        customTabIntent.intent.setData(Uri.parse(initialUrl));
        Intent intent = LaunchIntentDispatcher.createCustomTabActivityIntent(
                activity, customTabIntent.intent);
        intent.setPackage(activity.getPackageName());
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, activity.getPackageName());
        IntentHandler.addTrustedIntentExtras(intent);
        return intent;
    }

    /**
     * Populates intent extras for an Autofill Assistant script.
     * @param intent An {@link Intent} to be populated.
     * @param username A username for a password change script. One of extras to put.
     */
    private void populateAutofillAssistantExtras(Intent intent, String username) {
        intent.putExtra(AUTOFILL_ASSISTANT_ENABLED_KEY, /* value= */ true);
        intent.putExtra(AUTOFILL_ASSISTANT_PACKAGE + PASSWORD_CHANGE_USERNAME_PARAMETER, username);
        intent.putExtra(AUTOFILL_ASSISTANT_PACKAGE + INTENT_PARAMETER, INTENT);
        // TODO(crbug.com/1086114): Also add the following parameters when server side changes is
        // ready: CALLER, SOURCE. That would be useful for metrics.
    }
}
