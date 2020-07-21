// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_DATA_MODEL_AUTOFILL_STRUCTURED_ADDRESS_UTILS_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_DATA_MODEL_AUTOFILL_STRUCTURED_ADDRESS_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/field_types.h"
#include "third_party/re2/src/re2/re2.h"

namespace autofill {
namespace structured_address {

// Enum to express the few quantifiers needed to parse values.
enum MatchQuantifier {
  // The capture group is required.
  MATCH_REQUIRED,
  // The capture group is optional.
  MATCH_OPTIONAL,
  // The capture group is lazy optional meaning that it is avoided if an overall
  // match is possible.
  MATCH_LAZY_OPTIONAL,
};

// Options for capturing a named group using the
// |CaptureTypeWithPattern(...)| functions.
struct CaptureOptions {
  // A separator that must be matched after a capture group.
  // By default, a group must be either followed by a space-like character (\s)
  // or it must be the last group in the line. The separator is allowed to be
  // empty.
  std::string separator = "\\s|$";
  // Indicates if the group is required, optional or even lazy optional.
  MatchQuantifier quantifier = MATCH_REQUIRED;
};

// A cache for compiled RE2 regular expressions.
class Re2RegExCache {
 public:
  Re2RegExCache& operator=(const Re2RegExCache&) = delete;
  Re2RegExCache(const Re2RegExCache&) = delete;
  ~Re2RegExCache() = delete;

  // Returns a singleton instance.
  static Re2RegExCache* Instance();

  // Returns a pointer to a constant compiled expression that matches |pattern|
  // case-insensitively.
  const RE2* GetRegEx(const std::string& pattern);

#ifdef UNIT_TEST
  // Returns true if the compiled regular expression corresponding to |pattern|
  // is cached.
  bool IsRegExCachedForTesting(const std::string& pattern) {
    return regex_map_.count(pattern) > 0;
  }
#endif

 private:
  Re2RegExCache();

  // Since the constructor is private, |base::NoDestructor| must be friend to be
  // allowed to construct the cache.
  friend class base::NoDestructor<Re2RegExCache>;

  // Stores a compiled regular expression keyed by its corresponding |pattern|.
  std::map<std::string, std::unique_ptr<const RE2>> regex_map_;

  // A lock to prevent concurrent access to the map.
  base::Lock lock_;
};

// Parses |value| with an regular expression defined by |pattern|.
// Returns true on success meaning that the expressions is fully matched.
// The matching results are written into the supplied |result_map|, keyed by the
// name of the capture group with the captured substrings as the value.
bool ParseValueByRegularExpression(
    const std::string& value,
    const std::string& pattern,
    std::map<std::string, std::string>* result_map);

// Same as above, but accepts a compiled regular expression instead of the
// pattern.
bool ParseValueByRegularExpression(
    const std::string& value,
    const RE2* regex,
    std::map<std::string, std::string>* result_map);

// Returns a compiled case insensitive regular expression for |pattern|.
std::unique_ptr<const RE2> BuildRegExFromPattern(std::string pattern);

// Returns true if |value| can be matched with |pattern|.
bool IsPartialMatch(const std::string& value, const std::string& pattern);

// Returns a vector that contains all partial matches of |pattern| in |value|;
std::vector<std::string> GetAllPartialMatches(const std::string& value,
                                              const std::string& pattern);

// Extracts all placeholders of the format ${PLACEHOLDER} in |value|.
std::vector<std::string> ExtractAllPlaceholders(const std::string& value);

// Returns |value| as a placeholder token: ${value}.
std::string GetPlaceholderToken(const std::string& value);

// Returns a named capture group created by the concatenation of the
// StringPieces in |pattern_span_initializer_list|. The group is named by the
// string representation of |type| and respects |options|.
std::string CaptureTypeWithPattern(
    const ServerFieldType& type,
    std::initializer_list<base::StringPiece> pattern_span_initializer_list,
    const CaptureOptions& options);

// Same as |CaptureTypeWithPattern(type, pattern_span_initializer_list,
// options)| but uses default options.
std::string CaptureTypeWithPattern(
    const ServerFieldType& type,
    std::initializer_list<base::StringPiece> pattern_span_initializer_list);

// Returns a capture group named by the string representation of |type| that
// matches |pattern|.
std::string CaptureTypeWithPattern(const ServerFieldType& type,
                                   const std::string& pattern,
                                   const CaptureOptions& options);

// Same as |CaptureTypeWithPattern(type, pattern, options)| but uses default
// options.
std::string CaptureTypeWithPattern(const ServerFieldType& type,
                                   const std::string& pattern);

}  // namespace structured_address

}  // namespace autofill
#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_DATA_MODEL_AUTOFILL_STRUCTURED_ADDRESS_UTILS_H_
