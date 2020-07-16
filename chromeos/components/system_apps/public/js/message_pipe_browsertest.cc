// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/system_apps/public/js/message_pipe_browsertest.h"

#include "base/files/file_path.h"

namespace {

constexpr base::FilePath::CharType kRootDir[] =
    FILE_PATH_LITERAL("chromeos/components/system_apps/public/js/");

}  // namespace

MessagePipeBrowserTestBase::MessagePipeBrowserTestBase()
    : JsLibraryTest(base::FilePath(kRootDir)) {}

MessagePipeBrowserTestBase::~MessagePipeBrowserTestBase() = default;
