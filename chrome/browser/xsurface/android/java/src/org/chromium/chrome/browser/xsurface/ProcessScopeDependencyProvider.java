// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.xsurface;

import android.content.Context;

import androidx.annotation.Nullable;

/**
 * Provides application-level dependencies for an external surface.
 */
public interface ProcessScopeDependencyProvider {
    /** @return the context associated with the application. */
    @Nullable
    default Context getContext() {
        return null;
    }

    /** Returns the account name of the signed-in user, or the empty string. */
    default String getAccountName() {
        return "";
    }

    /** Returns the client instance id for this chrome. */
    default String getClientInstanceId() {
        return "";
    }

    /** Returns the collection of currently active experiment ids. */
    default int[] getExperimentIds() {
        return new int[0];
    }

    /** @see {Log.e} */
    default void logError(String tag, String messageTemplate, Object... args) {}

    /** @see {Log.w} */
    default void logWarning(String tag, String messageTemplate, Object... args) {}

    /**
     * Returns an ImageFetchClient. ImageFetchClient should only be used for fetching images.
     */
    @Nullable
    default ImageFetchClient getImageFetchClient() {
        return null;
    }
}
