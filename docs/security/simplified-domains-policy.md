# URL simplification policy in Google Chrome

## Background
URLs displayed in a user agent's address bar are often the only security context
directly exposed to users. The URL communicates site identity, but displaying
the entire URL can often provide opportunities for spoofing (e.g.
`https://your-bank.secure.login.com/accounts`). [Starting in an M86
experiment](https://blog.chromium.org/2020/08/helping-people-spot-spoofs-url.html),
Google Chrome may simplify the URL display to show only the most critical
information for security, hiding parts of the URL that make it harder for users
to make decisions about a site's identity and trustworthiness. This page
explains the criteria Chrome uses when simplifying the URL.

For more information about best practices for URL display in browsers, see our
article on [Guidelines for URL
Display](url_display_guidelines/url_display_guidelines.md).  

## Showing the full URL when needed
By default, Chrome shows a simplified version of the URL, hiding the scheme,
path, query, and fragment. For example, `https://example.com/example1/?id=1` is
displayed simply as `example.com` in the address bar.

To allow a user to see the full URL when needed, Chrome will display the entire
URL when: 

 * The user hovers over the address bar.
 * The user focuses on the address bar.
 * In some experimental variations, when a page first loads (before a user
   begins interacting with the loaded page).

This strategy optimizes for security while making it easy and discoverable for
users to retrieve full URLs.

## How URLs are simplified

Most of the time, Chrome simplifies URLs to their **full host, including
subdomains and registered domain** (for example,
`https://foo.example.com/bar#baz` will get simplified to `foo.example.com`).

However, in some cases, Chrome will further simplify the displayed URL to just
the **registered domain name** (for example, `example.com` in
`https://foo.example.com`).

Chrome will show just the registered domain when:

 * The full host (including dots) exceeds 25 characters \[[1](#fn1)\].
 * The full host includes a well known brand keyword, but is not that brand's
   website. See below for more detail.

These criteria may change as we observe user behavior and affected sites.

Downgrading hostnames to registered domain when the above criteria is met makes
it harder for sites to spoof users by using confusing subdomains (e.g.
`your-bank.com.secure.evil.com`). When evaluating the URL's domain, Chrome will
not take private registries from the [Public Suffix
List](https://publicsuffix.org/) into account. This is to avoid creating an
incentive for malicious sites to add themselves to the Public Suffix List.

### Brand keywords
Chrome looks for well known brands in subdomains and simplifies URLs to just
their host when a brand is present. The well known brands list is computed as
follows:

 * Chrome computes a list of well known brands using a well known domains
   [list](https://source.chromium.org/chromium/chromium/src/+/master:components/url_formatter/spoof_checks/top_domains/domains.list)
   with several filtering criteria applied, for example ignoring numbers and
   [common English
   words](https://ai.googleblog.com/2006/08/all-our-n-gram-are-belong-to-you.html) \[[2](#fn2)\].
 * When visiting a URL, Chrome splits the host into fragments on `.` and `-` and
   checks whether any fragment matches a well known brand keyword. If so, the
   URL will be simplified to just the domain, eliding subdomains.

This strategy aims to elide the subdomains most likely to be deceptive and
malicious, while minimizing impact on site owners who want their sites'
subdomains to be visible.

---

## Footnotes
 1. {#fn1} This limit is chosen so that >95% of Chrome page loads won't be
    affected.
 2. {#fn2} This algorithm will be expanded to accommodate other languages if
    experimental results are positive.
