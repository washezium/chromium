// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/profile_picker_ui.h"
#include "base/feature_list.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_util.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/webui/signin/profile_picker_handler.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/profile_picker_resources.h"
#include "chrome/grit/profile_picker_resources_map.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/webui/web_ui_util.h"

namespace {
bool IsProfileCreationAllowed() {
  PrefService* service = g_browser_process->local_state();
  DCHECK(service);
  return service->GetBoolean(prefs::kBrowserAddPersonEnabled);
}

bool IsGuestModeEnabled() {
  PrefService* service = g_browser_process->local_state();
  DCHECK(service);
  return service->GetBoolean(prefs::kBrowserGuestModeEnabled);
}

void AddStrings(content::WebUIDataSource* html_source) {
  static constexpr webui::LocalizedString kLocalizedStrings[] = {
      {"mainViewTitle", IDS_PROFILE_PICKER_MAIN_VIEW_TITLE},
      {"mainViewSubtitle", IDS_PROFILE_PICKER_MAIN_VIEW_SUBTITLE},
      {"addSpaceButton", IDS_PROFILE_PICKER_ADD_SPACE_BUTTON},
      {"askOnStartupCheckboxText", IDS_PROFILE_PICKER_ASK_ON_STARTUP},
      {"browseAsGuestButton", IDS_PROFILE_PICKER_BROWSE_AS_GUEST_BUTTON},
      {"menu", IDS_MENU},
      {"profileMenuName", IDS_PROFILE_PICKER_PROFILE_MENU_BUTTON_NAME},
      {"profileMenuRemoveText", IDS_PROFILE_PICKER_PROFILE_MENU_REMOVE_TEXT},
      {"profileMenuCustomizeText",
       IDS_PROFILE_PICKER_PROFILE_MENU_CUSTOMIZE_TEXT},
      {"removeWarningLocalProfile",
       IDS_PROFILE_PICKER_REMOVE_WARNING_LOCAL_PROFILE},
      {"removeWarningSignedInProfile",
       IDS_PROFILE_PICKER_REMOVE_WARNING_SIGNED_IN_PROFILE},
      {"removeWarningHistory", IDS_PROFILE_PICKER_REMOVE_WARNING_HISTORY},
      {"removeWarningPasswords", IDS_PROFILE_PICKER_REMOVE_WARNING_PASSWORDS},
      {"removeWarningBookmarks", IDS_PROFILE_PICKER_REMOVE_WARNING_BOOKMARKS},
      {"removeWarningAutofill", IDS_PROFILE_PICKER_REMOVE_WARNING_AUTOFILL},
      {"removeWarningCalculating",
       IDS_PROFILE_PICKER_REMOVE_WARNING_CALCULATING},
      {"backButtonLabel", IDS_PROFILE_PICKER_BACK_BUTTON_LABEL},
      {"profileTypeChoiceTitle",
       IDS_PROFILE_PICKER_PROFILE_CREATION_FLOW_PROFILE_TYPE_CHOICE_TITLE},
      {"profileTypeChoiceSubtitle",
       IDS_PROFILE_PICKER_PROFILE_CREATION_FLOW_PROFILE_TYPE_CHOICE_SUBTITLE},
      {"signInButtonLabel",
       IDS_PROFILE_PICKER_PROFILE_CREATION_FLOW_SIGNIN_BUTTON_LABEL},
      {"notNowButtonLabel",
       IDS_PROFILE_PICKER_PROFILE_CREATION_FLOW_NOT_NOW_BUTTON_LABEL},
  };
  AddLocalizedStringsBulk(html_source, kLocalizedStrings);
  html_source->AddBoolean("askOnStartup",
                          g_browser_process->local_state()->GetBoolean(
                              prefs::kBrowserShowProfilePickerOnStartup));
  html_source->AddBoolean(
      "signInProfileCreationFlowSupported",
      base::FeatureList::IsEnabled(features::kSignInProfileCreationFlow));

  // Add policies.
  html_source->AddBoolean("isForceSigninEnabled",
                          signin_util::IsForceSigninEnabled());
  html_source->AddBoolean("isGuestModeEnabled", IsGuestModeEnabled());
  html_source->AddBoolean("isProfileCreationAllowed",
                          IsProfileCreationAllowed());
  // TODO(crbug.com/1063856): Check if |BrowserSignin| device policy exists.
}

}  // namespace

ProfilePickerUI::ProfilePickerUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kChromeUIProfilePickerHost);

  web_ui->AddMessageHandler(std::make_unique<ProfilePickerHandler>());

  std::string generated_path =
      "@out_folder@/gen/chrome/browser/resources/signin/profile_picker/";
  webui::SetupWebUIDataSource(
      html_source,
      base::make_span(kProfilePickerResources, kProfilePickerResourcesSize),
      generated_path, IDR_PROFILE_PICKER_PROFILE_PICKER_HTML);

  AddStrings(html_source);
  content::WebUIDataSource::Add(profile, html_source);
}

ProfilePickerUI::~ProfilePickerUI() = default;
