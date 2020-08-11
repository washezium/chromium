// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.password_check;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;
import android.view.MenuItem;

import androidx.annotation.VisibleForTesting;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;

import org.chromium.base.IntentUtils;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.help.HelpAndFeedback;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

/**
 * Creates the PasswordCheckComponentUi. This class is responsible for managing the UI for the check
 * of the leaked password.
 */
class PasswordCheckCoordinator implements PasswordCheckComponentUi, LifecycleObserver {
    private static final String WELL_KNOWN_URL_PATH = "/.well-known/change-password";
    private static final String AUTOFILL_ASSISTANT_PACKAGE =
            "org.chromium.chrome.browser.autofill_assistant.";
    private static final String AUTOFILL_ASSISTANT_ENABLED_KEY =
            AUTOFILL_ASSISTANT_PACKAGE + "ENABLED";
    private static final String PASSWORD_CHANGE_USERNAME_PARAMETER = "PASSWORD_CHANGE_USERNAME";
    private static final String INTENT_PARAMETER = "INTENT";
    private static final String INTENT = "PASSWORD_CHANGE";

    private final PasswordCheckFragmentView mFragmentView;
    private final PasswordCheckMediator mMediator = new PasswordCheckMediator(
            this::launchCctWithChangePasswordUrl, this::launchCctWithScript);
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
        mFragmentView.getLifecycle().addObserver(this);
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_START)
    public void connectToModelWhenViewIsReady() {
        // In the rare case of a restarted activity, don't recreate the model and mediator.
        if (mModel == null) {
            mModel = PasswordCheckProperties.createDefaultModel();
            PasswordCheckCoordinator.setUpModelChangeProcessors(mModel, mFragmentView);
            mMediator.initialize(mModel, PasswordCheckFactory.getOrCreate());
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
        mMediator.destroy();
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
     * @param origin Origin of the site to be opened in a CCT.
     */
    private void launchCctWithChangePasswordUrl(String origin) {
        // TODO(crbug.com/1092444): Handle the case when an app should be opened. Consider to set
        // |browser_fallback_url|, it is used in case of error while opening a CCT.
        Intent intent = buildIntent(origin + WELL_KNOWN_URL_PATH);
        IntentUtils.safeStartActivity(mFragmentView.getActivity(), intent);
    }

    /**
     * Launches a CCT that starts a password change script for a {@link CompromisedCredential}.
     * @param credential A {@link CompromisedCredential} to be changed with a script.
     */
    private void launchCctWithScript(CompromisedCredential credential) {
        Intent intent = buildIntent(credential.getOriginUrl());
        populateAutofillAssistantExtras(intent, credential.getUsername());
        IntentUtils.safeStartActivity(mFragmentView.getActivity(), intent);
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
