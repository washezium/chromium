// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_IOS_BROWSER_ACCOUNT_CONSISTENCY_SERVICE_H_
#define COMPONENTS_SIGNIN_IOS_BROWSER_ACCOUNT_CONSISTENCY_SERVICE_H_

#include <map>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#import "components/signin/ios/browser/manage_accounts_delegate.h"
#import "components/signin/public/identity_manager/identity_manager.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_access_result.h"

namespace content_settings {
class CookieSettings;
}

namespace web {
class BrowserState;
class WebState;
class WebStatePolicyDecider;
}

class AccountReconcilor;
class PrefService;

// Handles actions necessary for keeping the list of Google accounts available
// on the web and those available on the iOS device from first-party Google apps
// consistent. This includes setting the Account Consistency cookie,
// CHROME_CONNECTED, which informs Gaia that the user is signed in to Chrome
// with Account Consistency on.
class AccountConsistencyService : public KeyedService,
                                  public signin::IdentityManager::Observer {
 public:
  // Name of the cookie that is managed by AccountConsistencyService and is used
  // to inform Google web properties that the browser is connected and that
  // Google authentication cookies are managed by |AccountReconcilor|).
  static const char kChromeConnectedCookieName[];

  // Name of the Google authentication cookie.
  static const char kGaiaCookieName[];

  // Name of the preference property that persists the domains that have a
  // CHROME_CONNECTED cookie set by this service.
  static const char kDomainsWithCookiePref[];

  AccountConsistencyService(
      web::BrowserState* browser_state,
      PrefService* prefs,
      AccountReconcilor* account_reconcilor,
      scoped_refptr<content_settings::CookieSettings> cookie_settings,
      signin::IdentityManager* identity_manager);
  ~AccountConsistencyService() override;

  // Registers the preferences used by AccountConsistencyService.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Sets the handler for |web_state| that reacts on Gaia responses with the
  // X-Chrome-Manage-Accounts header and notifies |delegate|.
  void SetWebStateHandler(web::WebState* web_state,
                          id<ManageAccountsDelegate> delegate);
  // Removes the handler associated with |web_state|.
  void RemoveWebStateHandler(web::WebState* web_state);

  // Removes CHROME_CONNECTED cookies on all the Google domains where it was
  // set. Calls callback once all cookies were removed.
  void RemoveChromeConnectedCookies(base::OnceClosure callback);

  // Checks for the presence of Gaia cookies and if they have been deleted
  // notifies the AccountReconcilor (the class responsible for rebuilding Gaia
  // cookies if needed).
  //
  // Applies a one hour time restriction in between updates to avoid too many
  // |GetAllCookies| calls on the cookie manager.
  void SetGaiaCookiesIfDeleted();

  // Enqueues a request to set the CHROME_CONNECTED cookie for |domain|.
  // The cookie is set if it is not already on |domain|.
  void SetChromeConnectedCookieWithDomain(const std::string& domain);

  // Enqueues a request to remove the CHROME_CONNECTED cookie to |domain|.
  // Does nothing if the cookie is not set on |domain|.
  void RemoveChromeConnectedCookieFromDomain(const std::string& domain,
                                             base::OnceClosure callback);

  // Notifies the AccountConsistencyService that browsing data has been removed
  // for any time period.
  void OnBrowsingDataRemoved();

 private:
  friend class AccountConsistencyServiceTest;

  // The type of a cookie request.
  enum CookieRequestType {
    ADD_CHROME_CONNECTED_COOKIE,
    REMOVE_CHROME_CONNECTED_COOKIE
  };

  // A CHROME_CONNECTED cookie request to be applied by the
  // AccountConsistencyService.
  struct CookieRequest {
    static CookieRequest CreateAddCookieRequest(const std::string& domain);
    static CookieRequest CreateRemoveCookieRequest(const std::string& domain,
                                                   base::OnceClosure callback);
    CookieRequest();
    ~CookieRequest();

    // Movable, not copyable.
    CookieRequest(CookieRequest&&);
    CookieRequest& operator=(CookieRequest&&) = default;
    CookieRequest(const CookieRequest&) = delete;
    CookieRequest& operator=(const CookieRequest&) = delete;

    CookieRequestType request_type;
    std::string domain;
    base::OnceClosure callback;
  };

  // Loads the domains with a CHROME_CONNECTED cookie from the prefs.
  void LoadFromPrefs();

  // KeyedService implementation.
  void Shutdown() override;

  // Applies the pending CHROME_CONNECTED cookie requests one by one.
  void ApplyCookieRequests();

  // Called when the request to set CHROME_CONNECTED cookie is done.
  void FinishedSetChromeConnectedCookie(
      net::CookieAccessResult cookie_access_result);

  // Called when the current CHROME_CONNECTED cookie request is done.
  void FinishedApplyingChromeConnectedCookieRequest(bool success);

  // Returns whether the CHROME_CONNECTED cookie should be added to |domain|.
  // If the cookie is not already on |domain|, it will return true. If the
  // cookie is time constrained, |cookie_refresh_interval| is present, then a
  // cookie older than |cookie_refresh_interval| returns true.
  bool ShouldSetChromeConnectedCookieToDomain(
      const std::string& domain,
      const base::TimeDelta& cookie_refresh_interval);

  // Enqueues a request to set the CHROME_CONNECTED cookie for |domain|.
  // The cookie is set if it is not already on |domain| or if it is too old
  // compared to the given |cookie_refresh_interval|.
  void SetChromeConnectedCookieWithDomain(
      const std::string& domain,
      const base::TimeDelta& cookie_refresh_interval);

  // Adds CHROME_CONNECTED cookies on all the main Google domains.
  void AddChromeConnectedCookies();

  // Triggers a Gaia cookie update on the Google domain.
  void TriggerGaiaCookieChangeIfDeleted(
      const net::CookieAccessResultList& cookie_list,
      const net::CookieAccessResultList& excluded_cookies);

  // Records whether Gaia cookies were present on navigation in UMA histogram.
  static void LogIOSGaiaCookiesPresentOnNavigation(bool is_present);

  // IdentityManager::Observer implementation.
  void OnPrimaryAccountSet(const CoreAccountInfo& account_info) override;
  void OnPrimaryAccountCleared(
      const CoreAccountInfo& previous_account_info) override;
  void OnAccountsInCookieUpdated(
      const signin::AccountsInCookieJarInfo& accounts_in_cookie_jar_info,
      const GoogleServiceAuthError& error) override;

  // Browser state associated with the service.
  web::BrowserState* browser_state_;
  // Used to update kDomainsWithCookiePref.
  PrefService* prefs_;
  // Service managing accounts reconciliation, notified of GAIA responses with
  // the X-Chrome-Manage-Accounts header
  AccountReconcilor* account_reconcilor_;
  // Cookie settings currently in use for |browser_state_|, used to check if
  // setting CHROME_CONNECTED cookies is valid.
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  // Identity manager, observed to be notified of primary account signin and
  // signout events.
  signin::IdentityManager* identity_manager_;

  // Whether a CHROME_CONNECTED cookie request is currently being applied.
  bool applying_cookie_requests_;
  // The queue of CHROME_CONNECTED cookie requests to be applied.
  base::circular_deque<CookieRequest> cookie_requests_;
  // The map between domains where a CHROME_CONNECTED cookie is present and
  // the time when the cookie was last updated.
  std::map<std::string, base::Time> last_cookie_update_map_;

  // Last time Gaia cookie was updated for the Google domain.
  base::Time last_gaia_cookie_verification_time_;

  // Handlers reacting on GAIA responses with the X-Chrome-Manage-Accounts
  // header set.
  std::map<web::WebState*, std::unique_ptr<web::WebStatePolicyDecider>>
      web_state_handlers_;

  DISALLOW_COPY_AND_ASSIGN(AccountConsistencyService);
};

#endif  // COMPONENTS_SIGNIN_IOS_BROWSER_ACCOUNT_CONSISTENCY_SERVICE_H_
