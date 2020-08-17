// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.password_check;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.PreferenceFragmentCompat;

/**
 * This class is responsible for rendering the edit fragment where users can provide a new password
 * for compromised credentials.
 * TODO(crbug.com/1092444): Make this a component that is reusable in password settings as well.
 */
public class PasswordCheckEditFragmentView extends PreferenceFragmentCompat {
    public static final String EXTRA_COMPROMISED_CREDENTIAL = "extra_compromised_credential";
    private static final String EXTRA_NEW_PASSWORD = "extra_new_password";

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {}

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        setHasOptionsMenu(true);
        getActivity().setTitle(R.string.password_entry_viewer_edit_stored_password_action_title);
        return inflater.inflate(R.layout.password_check_edit_fragment, container, false);
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        menu.clear(); // Remove help and feedback for this screen.
        inflater.inflate(R.menu.password_check_editor_action_bar_menu, menu);
        // TODO(crbug.com/1092444): Make the back arrow an 'X'.
    }
}
