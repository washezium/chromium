// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_engines/template_url_prepopulate_data.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "build/build_config.h"
#include "components/country_codes/country_codes.h"
#include "components/google/core/common/google_util.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_data_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace TemplateURLPrepopulateData {

// Helpers --------------------------------------------------------------------

namespace {
// NOTE: You should probably not change the data in this file without changing
// |kCurrentDataVersion| in prepopulated_engines.json. See comments in
// GetDataVersion() below!

// Put the engines within each country in order with most interesting/important
// first.  The default will be the first engine.

// Default (for countries with no better engine set)
const PrepopulatedEngine* const engines_default[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
};

// Note, the below entries are sorted by country code, not the name in comment.
// Engine selection by country ------------------------------------------------
// United Arab Emirates
const PrepopulatedEngine* const engines_AE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// Albania
const PrepopulatedEngine* const engines_AL[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &duckduckgo,
    &yandex_ru,
};

// Argentina
const PrepopulatedEngine* const engines_AR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_ar,
    &ecosia,
};

// Austria
const PrepopulatedEngine* const engines_AT[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_at,
    &ecosia,
};

// Australia
const PrepopulatedEngine* const engines_AU[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_au,
    &ecosia,
};

// Bosnia and Herzegovina
const PrepopulatedEngine* const engines_BA[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &ask,
};

// Belgium
const PrepopulatedEngine* const engines_BE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Bulgaria
const PrepopulatedEngine* const engines_BG[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &yandex_ru,
};

// Bahrain
const PrepopulatedEngine* const engines_BH[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &ecosia,
};

// Burundi
const PrepopulatedEngine* const engines_BI[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &ask,
};

// Brunei
const PrepopulatedEngine* const engines_BN[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &ecosia,
};

// Bolivia
const PrepopulatedEngine* const engines_BO[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Brazil
const PrepopulatedEngine* const engines_BR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_br,
    &ecosia,
};

// Belarus
const PrepopulatedEngine* const engines_BY[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yandex_by,
    &mail_ru,
    &bing,
    &yahoo,
};

// Belize
const PrepopulatedEngine* const engines_BZ[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ask,
};

// Canada
const PrepopulatedEngine* const engines_CA[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_ca,
    &ecosia,
};

// Switzerland
const PrepopulatedEngine* const engines_CH[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_ch,
    &ecosia,
};

// Chile
const PrepopulatedEngine* const engines_CL[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_cl,
    &ecosia,
};

// China
const PrepopulatedEngine* const engines_CN[] = {
    &qwant,
    &baidu,
    &sogou,
    &duckduckgo,
    &google,
    &so_360,
    &bing,
};

// Colombia
const PrepopulatedEngine* const engines_CO[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_co,
    &ecosia,
};

// Costa Rica
const PrepopulatedEngine* const engines_CR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Czech Republic
const PrepopulatedEngine* const engines_CZ[] = {
    &qwant,
    &duckduckgo,
    &google,
    &seznam_cz,
    &bing,
    &yahoo,
    &duckduckgo,
};

// Germany
const PrepopulatedEngine* const engines_DE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_de,
    &ecosia,
};

// Denmark
const PrepopulatedEngine* const engines_DK[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_dk,
    &ecosia,
};

// Dominican Republic
const PrepopulatedEngine* const engines_DO[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Algeria
const PrepopulatedEngine* const engines_DZ[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &yandex_ru,
    &duckduckgo,
};

// Ecuador
const PrepopulatedEngine* const engines_EC[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Estonia
const PrepopulatedEngine* const engines_EE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yandex_ru,
    &yahoo,
    &mail_ru,
};

// Egypt
const PrepopulatedEngine* const engines_EG[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &yandex_ru,
    &duckduckgo,
};

// Spain
const PrepopulatedEngine* const engines_ES[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_es,
    &ecosia,
};

// Finland
const PrepopulatedEngine* const engines_FI[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_fi,
    &yandex_ru,
};

// Faroe Islands
const PrepopulatedEngine* const engines_FO[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_dk,
    &ecosia,
};

// France
const PrepopulatedEngine* const engines_FR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_fr,
    &ecosia,
};

// United Kingdom
const PrepopulatedEngine* const engines_GB[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_uk,
    &ecosia,
};

// Greece
const PrepopulatedEngine* const engines_GR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// Guatemala
const PrepopulatedEngine* const engines_GT[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Hong Kong
const PrepopulatedEngine* const engines_HK[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo_hk,
    &bing,
    &yandex_com,
    &baidu,
};

// Honduras
const PrepopulatedEngine* const engines_HN[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// Croatia
const PrepopulatedEngine* const engines_HR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Hungary
const PrepopulatedEngine* const engines_HU[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Indonesia
const PrepopulatedEngine* const engines_ID[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo_id,
    &bing,
    &yandex_com,
};

// Ireland
const PrepopulatedEngine* const engines_IE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Israel
const PrepopulatedEngine* const engines_IL[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
    &duckduckgo,
};

// India
const PrepopulatedEngine* const engines_IN[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo_in,
    &bing,
    &yandex_ru,
};

// Iraq
const PrepopulatedEngine* const engines_IQ[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &yandex_ru,
    &duckduckgo,
};

// Iran
const PrepopulatedEngine* const engines_IR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
    &ask,
};

// Iceland
const PrepopulatedEngine* const engines_IS[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Italy
const PrepopulatedEngine* const engines_IT[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Jamaica
const PrepopulatedEngine* const engines_JM[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ask,
};

// Jordan
const PrepopulatedEngine* const engines_JO[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &yandex_com,
};

// Japan
const PrepopulatedEngine* const engines_JP[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo_jp,
    &bing,
    &baidu,
    &duckduckgo,
};

// Kenya
const PrepopulatedEngine* const engines_KE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// South Korea
const PrepopulatedEngine* const engines_KR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &naver,
    &bing,
    &daum,
    &yahoo_jp,
};

// Kuwait
const PrepopulatedEngine* const engines_KW[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &yandex_com,
};

// Kazakhstan
const PrepopulatedEngine* const engines_KZ[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yandex_kz,
    &mail_ru,
    &bing,
    &yahoo,
};

// Lebanon
const PrepopulatedEngine* const engines_LB[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Liechtenstein
const PrepopulatedEngine* const engines_LI[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Lithuania
const PrepopulatedEngine* const engines_LT[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
    &duckduckgo,
};

// Luxembourg
const PrepopulatedEngine* const engines_LU[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Latvia
const PrepopulatedEngine* const engines_LV[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yandex_ru,
    &bing,
    &yahoo,
    &duckduckgo,
};

// Libya
const PrepopulatedEngine* const engines_LY[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &yandex_com,
    &duckduckgo,
};

// Morocco
const PrepopulatedEngine* const engines_MA[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &duckduckgo,
    &yandex_com,
};

// Monaco
const PrepopulatedEngine* const engines_MC[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &qwant,
};

// Moldova
const PrepopulatedEngine* const engines_MD[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yandex_ru,
    &mail_ru,
    &bing,
};

// Montenegro
const PrepopulatedEngine* const engines_ME[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
    &duckduckgo,
};

// Macedonia
const PrepopulatedEngine* const engines_MK[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// Mexico
const PrepopulatedEngine* const engines_MX[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_mx,
    &ecosia,
};

// Malaysia
const PrepopulatedEngine* const engines_MY[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_my,
    &duckduckgo,
    &baidu,
};

// Nicaragua
const PrepopulatedEngine* const engines_NI[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Netherlands
const PrepopulatedEngine* const engines_NL[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_nl,
    &yandex_ru,
};

// Norway
const PrepopulatedEngine* const engines_NO[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// New Zealand
const PrepopulatedEngine* const engines_NZ[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_nz,
    &ecosia,
};

// Oman
const PrepopulatedEngine* const engines_OM[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &ecosia,
};

// Panama
const PrepopulatedEngine* const engines_PA[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Peru
const PrepopulatedEngine* const engines_PE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_pe,
    &ecosia,
};

// Philippines
const PrepopulatedEngine* const engines_PH[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo_ph,
    &bing,
    &ecosia,
};

// Pakistan
const PrepopulatedEngine* const engines_PK[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &duckduckgo,
    &yandex_com,
};

// Poland
const PrepopulatedEngine* const engines_PL[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// Puerto Rico
const PrepopulatedEngine* const engines_PR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Portugal
const PrepopulatedEngine* const engines_PT[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Paraguay
const PrepopulatedEngine* const engines_PY[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Qatar
const PrepopulatedEngine* const engines_QA[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &yandex_com,
};

// Romania
const PrepopulatedEngine* const engines_RO[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// Serbia
const PrepopulatedEngine* const engines_RS[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &yandex_ru,
};

// Russia
const PrepopulatedEngine* const engines_RU[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yandex_ru,
    &mail_ru,
    &bing,
    &yahoo,
};

// Rwanda
const PrepopulatedEngine* const engines_RW[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &mail_ru,
};

// Saudi Arabia
const PrepopulatedEngine* const engines_SA[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_com,
    &duckduckgo,
};

// Sweden
const PrepopulatedEngine* const engines_SE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo_se,
    &ecosia,
};

// Singapore
const PrepopulatedEngine* const engines_SG[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yandex_com,
    &yahoo_sg,
    &baidu,
};

// Slovenia
const PrepopulatedEngine* const engines_SI[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
};

// Slovakia
const PrepopulatedEngine* const engines_SK[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &seznam_cz,
};

// El Salvador
const PrepopulatedEngine* const engines_SV[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Syria
const PrepopulatedEngine* const engines_SY[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
    &duckduckgo,
};

// Thailand
const PrepopulatedEngine* const engines_TH[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo_th,
    &bing,
    &duckduckgo,
    &baidu,
};

// Tunisia
const PrepopulatedEngine* const engines_TN[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo,
    &bing,
    &yandex_ru,
    &duckduckgo,
};

// Turkey
const PrepopulatedEngine* const engines_TR[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yandex_tr,
    &yahoo_tr,
    &bing,
    &duckduckgo,
};

// Trinidad and Tobago
const PrepopulatedEngine* const engines_TT[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ask,
};

// Taiwan
const PrepopulatedEngine* const engines_TW[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo_tw,
    &bing,
    &baidu,
    &ecosia,
};

// Tanzania
const PrepopulatedEngine* const engines_TZ[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &yandex_ru,
};

// Ukraine
const PrepopulatedEngine* const engines_UA[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yandex_ua,
    &bing,
    &mail_ru,
    &yahoo,
};

// United States
const PrepopulatedEngine* const engines_US[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Uruguay
const PrepopulatedEngine* const engines_UY[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ecosia,
};

// Venezuela
const PrepopulatedEngine* const engines_VE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &yahoo_ve,
    &bing,
    &ecosia,
};

// Vietnam
const PrepopulatedEngine* const engines_VN[] = {
    &qwant,
    &duckduckgo,
    &google,
    &coccoc,
    &yahoo,
    &bing,
    &ecosia,
};

// Yemen
const PrepopulatedEngine* const engines_YE[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &yandex_ru,
    &duckduckgo,
};

// South Africa
const PrepopulatedEngine* const engines_ZA[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &baidu,
};

// Zimbabwe
const PrepopulatedEngine* const engines_ZW[] = {
    &qwant,
    &duckduckgo,
    &google,
    &bing,
    &yahoo,
    &ask,
    &duckduckgo,
};
// ----------------------------------------------------------------------------

std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulationSetFromCountryID(
    int country_id) {
  const PrepopulatedEngine* const* engines;
  size_t num_engines;
  // If you add a new country make sure to update the unit test for coverage.
  switch (country_id) {
#define UNHANDLED_COUNTRY(code1, code2) \
  case country_codes::CountryCharsToCountryID((#code1)[0], (#code2)[0]):
#define END_UNHANDLED_COUNTRIES(code1, code2)       \
  engines = engines_##code1##code2;                 \
  num_engines = base::size(engines_##code1##code2); \
  break;
#define DECLARE_COUNTRY(code1, code2)\
    UNHANDLED_COUNTRY(code1, code2)\
    END_UNHANDLED_COUNTRIES(code1, code2)

    // Countries with their own, dedicated engine set.
    DECLARE_COUNTRY(A, E)  // United Arab Emirates
    DECLARE_COUNTRY(A, L)  // Albania
    DECLARE_COUNTRY(A, R)  // Argentina
    DECLARE_COUNTRY(A, T)  // Austria
    DECLARE_COUNTRY(A, U)  // Australia
    DECLARE_COUNTRY(B, A)  // Bosnia and Herzegovina
    DECLARE_COUNTRY(B, E)  // Belgium
    DECLARE_COUNTRY(B, G)  // Bulgaria
    DECLARE_COUNTRY(B, H)  // Bahrain
    DECLARE_COUNTRY(B, I)  // Burundi
    DECLARE_COUNTRY(B, N)  // Brunei
    DECLARE_COUNTRY(B, O)  // Bolivia
    DECLARE_COUNTRY(B, R)  // Brazil
    DECLARE_COUNTRY(B, Y)  // Belarus
    DECLARE_COUNTRY(B, Z)  // Belize
    DECLARE_COUNTRY(C, A)  // Canada
    DECLARE_COUNTRY(C, H)  // Switzerland
    DECLARE_COUNTRY(C, L)  // Chile
    DECLARE_COUNTRY(C, N)  // China
    DECLARE_COUNTRY(C, O)  // Colombia
    DECLARE_COUNTRY(C, R)  // Costa Rica
    DECLARE_COUNTRY(C, Z)  // Czech Republic
    DECLARE_COUNTRY(D, E)  // Germany
    DECLARE_COUNTRY(D, K)  // Denmark
    DECLARE_COUNTRY(D, O)  // Dominican Republic
    DECLARE_COUNTRY(D, Z)  // Algeria
    DECLARE_COUNTRY(E, C)  // Ecuador
    DECLARE_COUNTRY(E, E)  // Estonia
    DECLARE_COUNTRY(E, G)  // Egypt
    DECLARE_COUNTRY(E, S)  // Spain
    DECLARE_COUNTRY(F, I)  // Finland
    DECLARE_COUNTRY(F, O)  // Faroe Islands
    DECLARE_COUNTRY(F, R)  // France
    DECLARE_COUNTRY(G, B)  // United Kingdom
    DECLARE_COUNTRY(G, R)  // Greece
    DECLARE_COUNTRY(G, T)  // Guatemala
    DECLARE_COUNTRY(H, K)  // Hong Kong
    DECLARE_COUNTRY(H, N)  // Honduras
    DECLARE_COUNTRY(H, R)  // Croatia
    DECLARE_COUNTRY(H, U)  // Hungary
    DECLARE_COUNTRY(I, D)  // Indonesia
    DECLARE_COUNTRY(I, E)  // Ireland
    DECLARE_COUNTRY(I, L)  // Israel
    DECLARE_COUNTRY(I, N)  // India
    DECLARE_COUNTRY(I, Q)  // Iraq
    DECLARE_COUNTRY(I, R)  // Iran
    DECLARE_COUNTRY(I, S)  // Iceland
    DECLARE_COUNTRY(I, T)  // Italy
    DECLARE_COUNTRY(J, M)  // Jamaica
    DECLARE_COUNTRY(J, O)  // Jordan
    DECLARE_COUNTRY(J, P)  // Japan
    DECLARE_COUNTRY(K, E)  // Kenya
    DECLARE_COUNTRY(K, R)  // South Korea
    DECLARE_COUNTRY(K, W)  // Kuwait
    DECLARE_COUNTRY(K, Z)  // Kazakhstan
    DECLARE_COUNTRY(L, B)  // Lebanon
    DECLARE_COUNTRY(L, I)  // Liechtenstein
    DECLARE_COUNTRY(L, T)  // Lithuania
    DECLARE_COUNTRY(L, U)  // Luxembourg
    DECLARE_COUNTRY(L, V)  // Latvia
    DECLARE_COUNTRY(L, Y)  // Libya
    DECLARE_COUNTRY(M, A)  // Morocco
    DECLARE_COUNTRY(M, C)  // Monaco
    DECLARE_COUNTRY(M, D)  // Moldova
    DECLARE_COUNTRY(M, E)  // Montenegro
    DECLARE_COUNTRY(M, K)  // Macedonia
    DECLARE_COUNTRY(M, X)  // Mexico
    DECLARE_COUNTRY(M, Y)  // Malaysia
    DECLARE_COUNTRY(N, I)  // Nicaragua
    DECLARE_COUNTRY(N, L)  // Netherlands
    DECLARE_COUNTRY(N, O)  // Norway
    DECLARE_COUNTRY(N, Z)  // New Zealand
    DECLARE_COUNTRY(O, M)  // Oman
    DECLARE_COUNTRY(P, A)  // Panama
    DECLARE_COUNTRY(P, E)  // Peru
    DECLARE_COUNTRY(P, H)  // Philippines
    DECLARE_COUNTRY(P, K)  // Pakistan
    DECLARE_COUNTRY(P, L)  // Poland
    DECLARE_COUNTRY(P, R)  // Puerto Rico
    DECLARE_COUNTRY(P, T)  // Portugal
    DECLARE_COUNTRY(P, Y)  // Paraguay
    DECLARE_COUNTRY(Q, A)  // Qatar
    DECLARE_COUNTRY(R, O)  // Romania
    DECLARE_COUNTRY(R, S)  // Serbia
    DECLARE_COUNTRY(R, U)  // Russia
    DECLARE_COUNTRY(R, W)  // Rwanda
    DECLARE_COUNTRY(S, A)  // Saudi Arabia
    DECLARE_COUNTRY(S, E)  // Sweden
    DECLARE_COUNTRY(S, G)  // Singapore
    DECLARE_COUNTRY(S, I)  // Slovenia
    DECLARE_COUNTRY(S, K)  // Slovakia
    DECLARE_COUNTRY(S, V)  // El Salvador
    DECLARE_COUNTRY(S, Y)  // Syria
    DECLARE_COUNTRY(T, H)  // Thailand
    DECLARE_COUNTRY(T, N)  // Tunisia
    DECLARE_COUNTRY(T, R)  // Turkey
    DECLARE_COUNTRY(T, T)  // Trinidad and Tobago
    DECLARE_COUNTRY(T, W)  // Taiwan
    DECLARE_COUNTRY(T, Z)  // Tanzania
    DECLARE_COUNTRY(U, A)  // Ukraine
    DECLARE_COUNTRY(U, S)  // United States
    DECLARE_COUNTRY(U, Y)  // Uruguay
    DECLARE_COUNTRY(V, E)  // Venezuela
    DECLARE_COUNTRY(V, N)  // Vietnam
    DECLARE_COUNTRY(Y, E)  // Yemen
    DECLARE_COUNTRY(Z, A)  // South Africa
    DECLARE_COUNTRY(Z, W)  // Zimbabwe

    // Countries using the "Australia" engine set.
    UNHANDLED_COUNTRY(C, C)  // Cocos Islands
    UNHANDLED_COUNTRY(C, X)  // Christmas Island
    UNHANDLED_COUNTRY(H, M)  // Heard Island and McDonald Islands
    UNHANDLED_COUNTRY(N, F)  // Norfolk Island
    END_UNHANDLED_COUNTRIES(A, U)

    // Countries using the "China" engine set.
    UNHANDLED_COUNTRY(M, O)  // Macao
    END_UNHANDLED_COUNTRIES(C, N)

    // Countries using the "Denmark" engine set.
    UNHANDLED_COUNTRY(G, L)  // Greenland
    END_UNHANDLED_COUNTRIES(D, K)

    // Countries using the "Spain" engine set.
    UNHANDLED_COUNTRY(A, D)  // Andorra
    END_UNHANDLED_COUNTRIES(E, S)

    // Countries using the "Finland" engine set.
    UNHANDLED_COUNTRY(A, X)  // Aland Islands
    END_UNHANDLED_COUNTRIES(F, I)

    // Countries using the "France" engine set.
    UNHANDLED_COUNTRY(B, F)  // Burkina Faso
    UNHANDLED_COUNTRY(B, J)  // Benin
    UNHANDLED_COUNTRY(C, D)  // Congo - Kinshasa
    UNHANDLED_COUNTRY(C, F)  // Central African Republic
    UNHANDLED_COUNTRY(C, G)  // Congo - Brazzaville
    UNHANDLED_COUNTRY(C, I)  // Ivory Coast
    UNHANDLED_COUNTRY(C, M)  // Cameroon
    UNHANDLED_COUNTRY(D, J)  // Djibouti
    UNHANDLED_COUNTRY(G, A)  // Gabon
    UNHANDLED_COUNTRY(G, F)  // French Guiana
    UNHANDLED_COUNTRY(G, N)  // Guinea
    UNHANDLED_COUNTRY(G, P)  // Guadeloupe
    UNHANDLED_COUNTRY(H, T)  // Haiti
#if defined(OS_WIN)
    UNHANDLED_COUNTRY(I, P)  // Clipperton Island ('IP' is an WinXP-ism; ISO
                             //                    includes it with France)
#endif
    UNHANDLED_COUNTRY(M, L)  // Mali
    UNHANDLED_COUNTRY(M, Q)  // Martinique
    UNHANDLED_COUNTRY(N, C)  // New Caledonia
    UNHANDLED_COUNTRY(N, E)  // Niger
    UNHANDLED_COUNTRY(P, F)  // French Polynesia
    UNHANDLED_COUNTRY(P, M)  // Saint Pierre and Miquelon
    UNHANDLED_COUNTRY(R, E)  // Reunion
    UNHANDLED_COUNTRY(S, N)  // Senegal
    UNHANDLED_COUNTRY(T, D)  // Chad
    UNHANDLED_COUNTRY(T, F)  // French Southern Territories
    UNHANDLED_COUNTRY(T, G)  // Togo
    UNHANDLED_COUNTRY(W, F)  // Wallis and Futuna
    UNHANDLED_COUNTRY(Y, T)  // Mayotte
    END_UNHANDLED_COUNTRIES(F, R)

    // Countries using the "Greece" engine set.
    UNHANDLED_COUNTRY(C, Y)  // Cyprus
    END_UNHANDLED_COUNTRIES(G, R)

    // Countries using the "Italy" engine set.
    UNHANDLED_COUNTRY(S, M)  // San Marino
    UNHANDLED_COUNTRY(V, A)  // Vatican
    END_UNHANDLED_COUNTRIES(I, T)

    // Countries using the "Morocco" engine set.
    UNHANDLED_COUNTRY(E, H)  // Western Sahara
    END_UNHANDLED_COUNTRIES(M, A)

    // Countries using the "Netherlands" engine set.
    UNHANDLED_COUNTRY(A, N)  // Netherlands Antilles
    UNHANDLED_COUNTRY(A, W)  // Aruba
    END_UNHANDLED_COUNTRIES(N, L)

    // Countries using the "Norway" engine set.
    UNHANDLED_COUNTRY(B, V)  // Bouvet Island
    UNHANDLED_COUNTRY(S, J)  // Svalbard and Jan Mayen
    END_UNHANDLED_COUNTRIES(N, O)

    // Countries using the "New Zealand" engine set.
    UNHANDLED_COUNTRY(C, K)  // Cook Islands
    UNHANDLED_COUNTRY(N, U)  // Niue
    UNHANDLED_COUNTRY(T, K)  // Tokelau
    END_UNHANDLED_COUNTRIES(N, Z)

    // Countries using the "Portugal" engine set.
    UNHANDLED_COUNTRY(C, V)  // Cape Verde
    UNHANDLED_COUNTRY(G, W)  // Guinea-Bissau
    UNHANDLED_COUNTRY(M, Z)  // Mozambique
    UNHANDLED_COUNTRY(S, T)  // Sao Tome and Principe
    UNHANDLED_COUNTRY(T, L)  // Timor-Leste
    END_UNHANDLED_COUNTRIES(P, T)

    // Countries using the "Russia" engine set.
    UNHANDLED_COUNTRY(A, M)  // Armenia
    UNHANDLED_COUNTRY(A, Z)  // Azerbaijan
    UNHANDLED_COUNTRY(K, G)  // Kyrgyzstan
    UNHANDLED_COUNTRY(T, J)  // Tajikistan
    UNHANDLED_COUNTRY(T, M)  // Turkmenistan
    UNHANDLED_COUNTRY(U, Z)  // Uzbekistan
    END_UNHANDLED_COUNTRIES(R, U)

    // Countries using the "Saudi Arabia" engine set.
    UNHANDLED_COUNTRY(M, R)  // Mauritania
    UNHANDLED_COUNTRY(P, S)  // Palestinian Territory
    UNHANDLED_COUNTRY(S, D)  // Sudan
    END_UNHANDLED_COUNTRIES(S, A)

    // Countries using the "United Kingdom" engine set.
    UNHANDLED_COUNTRY(B, M)  // Bermuda
    UNHANDLED_COUNTRY(F, K)  // Falkland Islands
    UNHANDLED_COUNTRY(G, G)  // Guernsey
    UNHANDLED_COUNTRY(G, I)  // Gibraltar
    UNHANDLED_COUNTRY(G, S)  // South Georgia and the South Sandwich
                             //   Islands
    UNHANDLED_COUNTRY(I, M)  // Isle of Man
    UNHANDLED_COUNTRY(I, O)  // British Indian Ocean Territory
    UNHANDLED_COUNTRY(J, E)  // Jersey
    UNHANDLED_COUNTRY(K, Y)  // Cayman Islands
    UNHANDLED_COUNTRY(M, S)  // Montserrat
    UNHANDLED_COUNTRY(M, T)  // Malta
    UNHANDLED_COUNTRY(P, N)  // Pitcairn Islands
    UNHANDLED_COUNTRY(S, H)  // Saint Helena, Ascension Island, and Tristan da
                             //   Cunha
    UNHANDLED_COUNTRY(T, C)  // Turks and Caicos Islands
    UNHANDLED_COUNTRY(V, G)  // British Virgin Islands
    END_UNHANDLED_COUNTRIES(G, B)

    // Countries using the "United States" engine set.
    UNHANDLED_COUNTRY(A, S)  // American Samoa
    UNHANDLED_COUNTRY(G, U)  // Guam
    UNHANDLED_COUNTRY(M, P)  // Northern Mariana Islands
    UNHANDLED_COUNTRY(U, M)  // U.S. Minor Outlying Islands
    UNHANDLED_COUNTRY(V, I)  // U.S. Virgin Islands
    END_UNHANDLED_COUNTRIES(U, S)

    // Countries using the "default" engine set.
    UNHANDLED_COUNTRY(A, F)  // Afghanistan
    UNHANDLED_COUNTRY(A, G)  // Antigua and Barbuda
    UNHANDLED_COUNTRY(A, I)  // Anguilla
    UNHANDLED_COUNTRY(A, O)  // Angola
    UNHANDLED_COUNTRY(A, Q)  // Antarctica
    UNHANDLED_COUNTRY(B, B)  // Barbados
    UNHANDLED_COUNTRY(B, D)  // Bangladesh
    UNHANDLED_COUNTRY(B, S)  // Bahamas
    UNHANDLED_COUNTRY(B, T)  // Bhutan
    UNHANDLED_COUNTRY(B, W)  // Botswana
    UNHANDLED_COUNTRY(C, U)  // Cuba
    UNHANDLED_COUNTRY(D, M)  // Dominica
    UNHANDLED_COUNTRY(E, R)  // Eritrea
    UNHANDLED_COUNTRY(E, T)  // Ethiopia
    UNHANDLED_COUNTRY(F, J)  // Fiji
    UNHANDLED_COUNTRY(F, M)  // Micronesia
    UNHANDLED_COUNTRY(G, D)  // Grenada
    UNHANDLED_COUNTRY(G, E)  // Georgia
    UNHANDLED_COUNTRY(G, H)  // Ghana
    UNHANDLED_COUNTRY(G, M)  // Gambia
    UNHANDLED_COUNTRY(G, Q)  // Equatorial Guinea
    UNHANDLED_COUNTRY(G, Y)  // Guyana
    UNHANDLED_COUNTRY(K, H)  // Cambodia
    UNHANDLED_COUNTRY(K, I)  // Kiribati
    UNHANDLED_COUNTRY(K, M)  // Comoros
    UNHANDLED_COUNTRY(K, N)  // Saint Kitts and Nevis
    UNHANDLED_COUNTRY(K, P)  // North Korea
    UNHANDLED_COUNTRY(L, A)  // Laos
    UNHANDLED_COUNTRY(L, C)  // Saint Lucia
    UNHANDLED_COUNTRY(L, K)  // Sri Lanka
    UNHANDLED_COUNTRY(L, R)  // Liberia
    UNHANDLED_COUNTRY(L, S)  // Lesotho
    UNHANDLED_COUNTRY(M, G)  // Madagascar
    UNHANDLED_COUNTRY(M, H)  // Marshall Islands
    UNHANDLED_COUNTRY(M, M)  // Myanmar
    UNHANDLED_COUNTRY(M, N)  // Mongolia
    UNHANDLED_COUNTRY(M, U)  // Mauritius
    UNHANDLED_COUNTRY(M, V)  // Maldives
    UNHANDLED_COUNTRY(M, W)  // Malawi
    UNHANDLED_COUNTRY(N, A)  // Namibia
    UNHANDLED_COUNTRY(N, G)  // Nigeria
    UNHANDLED_COUNTRY(N, P)  // Nepal
    UNHANDLED_COUNTRY(N, R)  // Nauru
    UNHANDLED_COUNTRY(P, G)  // Papua New Guinea
    UNHANDLED_COUNTRY(P, W)  // Palau
    UNHANDLED_COUNTRY(S, B)  // Solomon Islands
    UNHANDLED_COUNTRY(S, C)  // Seychelles
    UNHANDLED_COUNTRY(S, L)  // Sierra Leone
    UNHANDLED_COUNTRY(S, O)  // Somalia
    UNHANDLED_COUNTRY(S, R)  // Suriname
    UNHANDLED_COUNTRY(S, Z)  // Swaziland
    UNHANDLED_COUNTRY(T, O)  // Tonga
    UNHANDLED_COUNTRY(T, V)  // Tuvalu
    UNHANDLED_COUNTRY(U, G)  // Uganda
    UNHANDLED_COUNTRY(V, C)  // Saint Vincent and the Grenadines
    UNHANDLED_COUNTRY(V, U)  // Vanuatu
    UNHANDLED_COUNTRY(W, S)  // Samoa
    UNHANDLED_COUNTRY(Z, M)  // Zambia
    case country_codes::kCountryIDUnknown:
    default:                // Unhandled location
    END_UNHANDLED_COUNTRIES(def, ault)
  }

  std::vector<std::unique_ptr<TemplateURLData>> t_urls;
  for (size_t i = 0; i < num_engines; ++i)
    t_urls.push_back(TemplateURLDataFromPrepopulatedEngine(*engines[i]));
  return t_urls;
}

std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulatedTemplateURLData(
    PrefService* prefs) {
  std::vector<std::unique_ptr<TemplateURLData>> t_urls;
  if (!prefs)
    return t_urls;

  const base::ListValue* list = prefs->GetList(prefs::kSearchProviderOverrides);
  if (!list)
    return t_urls;

  size_t num_engines = list->GetSize();
  for (size_t i = 0; i != num_engines; ++i) {
    const base::DictionaryValue* engine;
    if (list->GetDictionary(i, &engine)) {
      auto t_url = TemplateURLDataFromOverrideDictionary(*engine);
      if (t_url)
        t_urls.push_back(std::move(t_url));
    }
  }
  return t_urls;
}

bool SameDomain(const GURL& given_url, const GURL& prepopulated_url) {
  return prepopulated_url.is_valid() &&
      net::registry_controlled_domains::SameDomainOrHost(
          given_url, prepopulated_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

}  // namespace

// Global functions -----------------------------------------------------------

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  country_codes::RegisterProfilePrefs(registry);
  registry->RegisterListPref(prefs::kSearchProviderOverrides);
  registry->RegisterIntegerPref(prefs::kSearchProviderOverridesVersion, -1);
}

int GetDataVersion(PrefService* prefs) {
  // Allow tests to override the local version.
  return (prefs && prefs->HasPrefPath(prefs::kSearchProviderOverridesVersion)) ?
      prefs->GetInteger(prefs::kSearchProviderOverridesVersion) :
      kCurrentDataVersion;
}

std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulatedEngines(
    PrefService* prefs,
    size_t* default_search_provider_index) {
  // If there is a set of search engines in the preferences file, it overrides
  // the built-in set.
  std::vector<std::unique_ptr<TemplateURLData>> t_urls =
      GetPrepopulatedTemplateURLData(prefs);
  if (t_urls.empty()) {
    t_urls = GetPrepopulationSetFromCountryID(
        country_codes::GetCountryIDFromPrefs(prefs));
  }
  if (default_search_provider_index) {
    const auto itr = std::find_if(
        t_urls.begin(), t_urls.end(),
        [](const auto& t_url) { return t_url->prepopulate_id == google.id; });
    *default_search_provider_index =
        itr == t_urls.end() ? 0 : std::distance(t_urls.begin(), itr);
  }
  return t_urls;
}

std::unique_ptr<TemplateURLData> GetPrepopulatedEngine(PrefService* prefs,
                                                       int prepopulated_id) {
  auto engines =
      TemplateURLPrepopulateData::GetPrepopulatedEngines(prefs, nullptr);
  for (auto& engine : engines) {
    if (engine->prepopulate_id == prepopulated_id)
      return std::move(engine);
  }
  return nullptr;
}

#if defined(OS_ANDROID)

std::vector<std::unique_ptr<TemplateURLData>> GetLocalPrepopulatedEngines(
    const std::string& locale) {
  int country_id = country_codes::CountryStringToCountryID(locale);
  if (country_id == country_codes::kCountryIDUnknown) {
    LOG(ERROR) << "Unknown country code specified: " << locale;
    return std::vector<std::unique_ptr<TemplateURLData>>();
  }

  return GetPrepopulationSetFromCountryID(country_id);
}

#endif

std::vector<const PrepopulatedEngine*> GetAllPrepopulatedEngines() {
  return std::vector<const PrepopulatedEngine*>(
      &kAllEngines[0], &kAllEngines[0] + kAllEnginesLength);
}

void ClearPrepopulatedEnginesInPrefs(PrefService* prefs) {
  if (!prefs)
    return;

  prefs->ClearPref(prefs::kSearchProviderOverrides);
  prefs->ClearPref(prefs::kSearchProviderOverridesVersion);
}

std::unique_ptr<TemplateURLData> GetPrepopulatedDefaultSearch(
    PrefService* prefs) {
  size_t default_search_index;
  // This could be more efficient.  We load all URLs but keep only the default.
  std::vector<std::unique_ptr<TemplateURLData>> loaded_urls =
      GetPrepopulatedEngines(prefs, &default_search_index);

  return (default_search_index < loaded_urls.size())
             ? std::move(loaded_urls[default_search_index])
             : nullptr;
}

SearchEngineType GetEngineType(const GURL& url) {
  DCHECK(url.is_valid());

  // Check using TLD+1s, in order to more aggressively match search engine types
  // for data imported from other browsers.
  //
  // First special-case Google, because the prepopulate URL for it will not
  // convert to a GURL and thus won't have an origin.  Instead see if the
  // incoming URL's host is "[*.]google.<TLD>".
  if (google_util::IsGoogleHostname(url.host(),
                                    google_util::DISALLOW_SUBDOMAIN))
    return google.type;

  // Now check the rest of the prepopulate data.
  for (size_t i = 0; i < kAllEnginesLength; ++i) {
    // First check the main search URL.
    if (SameDomain(url, GURL(kAllEngines[i]->search_url)))
      return kAllEngines[i]->type;

    // Then check the alternate URLs.
    for (size_t j = 0; j < kAllEngines[i]->alternate_urls_size; ++j) {
      if (SameDomain(url, GURL(kAllEngines[i]->alternate_urls[j])))
        return kAllEngines[i]->type;
    }
  }

  return SEARCH_ENGINE_OTHER;
}

}  // namespace TemplateURLPrepopulateData
