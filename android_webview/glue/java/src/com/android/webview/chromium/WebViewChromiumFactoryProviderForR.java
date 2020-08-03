// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.webkit.PacProcessor;

class WebViewChromiumFactoryProviderForR extends WebViewChromiumFactoryProvider {
    public static WebViewChromiumFactoryProvider create(android.webkit.WebViewDelegate delegate) {
        return new WebViewChromiumFactoryProviderForR(delegate);
    }

    protected WebViewChromiumFactoryProviderForR(android.webkit.WebViewDelegate delegate) {
        super(delegate);
    }

    @Override
    public PacProcessor getPacProcessor() {
        return PacProcessorImpl.getInstance();
    }
}
