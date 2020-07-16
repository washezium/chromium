// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/web_applications/test/js_library_test.h"

#include <string>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "url/gurl.h"

namespace {

constexpr char kSystemAppTestHost[] = "system-app-test";

bool IsSystemAppTestURL(const GURL& url) {
  return url.SchemeIs(content::kChromeUIScheme) &&
         url.host() == kSystemAppTestHost;
}

void HandleRequest(const base::FilePath& root_dir,
                   const std::string& url_path,
                   content::WebUIDataSource::GotDataCallback callback) {
  base::FilePath path;
  CHECK(base::PathService::Get(base::BasePathKey::DIR_SOURCE_ROOT, &path));
  path = path.Append(root_dir);
  path = path.AppendASCII(url_path.substr(0, url_path.find("?")));

  std::string contents;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    CHECK(base::ReadFileToString(path, &contents)) << path.value();
  }

  scoped_refptr<base::RefCountedString> ref_contents(
      new base::RefCountedString);
  ref_contents->data() = contents;
  std::move(callback).Run(ref_contents);
}

class JsLibraryTestWebUIController : public ui::MojoWebUIController {
 public:
  explicit JsLibraryTestWebUIController(const base::FilePath& root_dir,
                                        content::WebUI* web_ui)
      : ui::MojoWebUIController(web_ui) {
    auto* data_source = content::WebUIDataSource::Create(kSystemAppTestHost);
    data_source->SetRequestFilter(
        base::BindRepeating([](const std::string& path) { return true; }),
        base::BindRepeating(&HandleRequest, root_dir));

    content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                  data_source);
  }
};

class JsLibraryTestWebUIControllerFactory
    : public content::WebUIControllerFactory {
 public:
  explicit JsLibraryTestWebUIControllerFactory(const base::FilePath& root_dir)
      : root_dir_(root_dir) {}

  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override {
    return std::make_unique<JsLibraryTestWebUIController>(root_dir_, web_ui);
  }

  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override {
    if (IsSystemAppTestURL(url)) {
      return reinterpret_cast<content::WebUI::TypeID>(this);
    }
    return content::WebUI::kNoWebUI;
  }

  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) override {
    return IsSystemAppTestURL(url);
  }

  bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                              const GURL& url) override {
    return IsSystemAppTestURL(url);
  }

 private:
  const base::FilePath root_dir_;
};

}  // namespace

JsLibraryTest::JsLibraryTest(const base::FilePath& root_dir)
    : factory_(
          std::make_unique<JsLibraryTestWebUIControllerFactory>(root_dir)) {
  content::WebUIControllerFactory::RegisterFactory(factory_.get());
}

JsLibraryTest::~JsLibraryTest() {
  content::WebUIControllerFactory::UnregisterFactoryForTesting(factory_.get());
}
