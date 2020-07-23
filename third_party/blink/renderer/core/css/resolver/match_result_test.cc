// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/resolver/match_result.h"

#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css/css_test_helpers.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/heap/heap.h"

namespace blink {

using css_test_helpers::ParseDeclarationBlock;

class MatchResultTest : public PageTestBase {
 protected:
  void SetUp() override;

  const CSSPropertyValueSet* PropertySet(unsigned index) const {
    return property_sets->at(index).Get();
  }

  size_t LengthOf(const MatchResult& result) const {
    return result.GetMatchedProperties().size();
  }

  CascadeOrigin OriginAt(const MatchResult& result, size_t index) const {
    DCHECK_LT(index, LengthOf(result));
    return result.GetMatchedProperties()[index].types_.origin;
  }

 private:
  Persistent<HeapVector<Member<MutableCSSPropertyValueSet>, 8>> property_sets;
};

void MatchResultTest::SetUp() {
  PageTestBase::SetUp();
  property_sets =
      MakeGarbageCollected<HeapVector<Member<MutableCSSPropertyValueSet>, 8>>();
  for (unsigned i = 0; i < 8; i++) {
    property_sets->push_back(
        MakeGarbageCollected<MutableCSSPropertyValueSet>(kHTMLQuirksMode));
  }
}

void TestMatchedPropertiesRange(const MatchedPropertiesRange& range,
                                int expected_length,
                                const CSSPropertyValueSet** expected_sets) {
  EXPECT_EQ(expected_length, range.end() - range.begin());
  for (const auto& matched_properties : range)
    EXPECT_EQ(*expected_sets++, matched_properties.properties);
}

TEST_F(MatchResultTest, UARules) {
  const CSSPropertyValueSet* ua_sets[] = {PropertySet(0), PropertySet(1)};

  MatchResult result;
  result.AddMatchedProperties(ua_sets[0]);
  result.AddMatchedProperties(ua_sets[1]);
  result.FinishAddingUARules();
  result.FinishAddingUserRules();

  result.FinishAddingAuthorRulesForTreeScope();

  TestMatchedPropertiesRange(result.AllRules(), 2, ua_sets);
  TestMatchedPropertiesRange(result.UaRules(), 2, ua_sets);
  TestMatchedPropertiesRange(result.UserRules(), 0, nullptr);
  TestMatchedPropertiesRange(result.AuthorRules(), 0, nullptr);
}

TEST_F(MatchResultTest, UserRules) {
  const CSSPropertyValueSet* user_sets[] = {PropertySet(0), PropertySet(1)};

  MatchResult result;

  result.FinishAddingUARules();
  result.AddMatchedProperties(user_sets[0]);
  result.AddMatchedProperties(user_sets[1]);
  result.FinishAddingUserRules();
  result.FinishAddingAuthorRulesForTreeScope();

  TestMatchedPropertiesRange(result.AllRules(), 2, user_sets);
  TestMatchedPropertiesRange(result.UaRules(), 0, nullptr);
  TestMatchedPropertiesRange(result.UserRules(), 2, user_sets);
  TestMatchedPropertiesRange(result.AuthorRules(), 0, nullptr);
}

TEST_F(MatchResultTest, AuthorRules) {
  const CSSPropertyValueSet* author_sets[] = {PropertySet(0), PropertySet(1)};

  MatchResult result;

  result.FinishAddingUARules();
  result.FinishAddingUserRules();
  result.AddMatchedProperties(author_sets[0]);
  result.AddMatchedProperties(author_sets[1]);
  result.FinishAddingAuthorRulesForTreeScope();

  TestMatchedPropertiesRange(result.AllRules(), 2, author_sets);
  TestMatchedPropertiesRange(result.UaRules(), 0, nullptr);
  TestMatchedPropertiesRange(result.UserRules(), 0, nullptr);
  TestMatchedPropertiesRange(result.AuthorRules(), 2, author_sets);
}

TEST_F(MatchResultTest, AllRules) {
  const CSSPropertyValueSet* all_sets[] = {PropertySet(0), PropertySet(1),
                                           PropertySet(2), PropertySet(3),
                                           PropertySet(4), PropertySet(5)};
  const CSSPropertyValueSet** ua_sets = &all_sets[0];
  const CSSPropertyValueSet** user_sets = &all_sets[2];
  const CSSPropertyValueSet** author_sets = &all_sets[4];

  MatchResult result;

  result.AddMatchedProperties(ua_sets[0]);
  result.AddMatchedProperties(ua_sets[1]);
  result.FinishAddingUARules();

  result.AddMatchedProperties(user_sets[0]);
  result.AddMatchedProperties(user_sets[1]);
  result.FinishAddingUserRules();

  result.AddMatchedProperties(author_sets[0]);
  result.AddMatchedProperties(author_sets[1]);
  result.FinishAddingAuthorRulesForTreeScope();

  TestMatchedPropertiesRange(result.AllRules(), 6, all_sets);
  TestMatchedPropertiesRange(result.UaRules(), 2, ua_sets);
  TestMatchedPropertiesRange(result.UserRules(), 2, user_sets);
  TestMatchedPropertiesRange(result.AuthorRules(), 2, author_sets);
}

TEST_F(MatchResultTest, AuthorRulesMultipleScopes) {
  const CSSPropertyValueSet* author_sets[] = {PropertySet(0), PropertySet(1),
                                              PropertySet(2), PropertySet(3)};

  MatchResult result;

  result.FinishAddingUARules();
  result.FinishAddingUserRules();

  result.AddMatchedProperties(author_sets[0]);
  result.AddMatchedProperties(author_sets[1]);
  result.FinishAddingAuthorRulesForTreeScope();

  result.AddMatchedProperties(author_sets[2]);
  result.AddMatchedProperties(author_sets[3]);
  result.FinishAddingAuthorRulesForTreeScope();

  TestMatchedPropertiesRange(result.AllRules(), 4, author_sets);
  TestMatchedPropertiesRange(result.UaRules(), 0, nullptr);
  TestMatchedPropertiesRange(result.UserRules(), 0, nullptr);
  TestMatchedPropertiesRange(result.AuthorRules(), 4, author_sets);
}

TEST_F(MatchResultTest, AllRulesMultipleScopes) {
  const CSSPropertyValueSet* all_sets[] = {
      PropertySet(0), PropertySet(1), PropertySet(2), PropertySet(3),
      PropertySet(4), PropertySet(5), PropertySet(6), PropertySet(7)};
  const CSSPropertyValueSet** ua_sets = &all_sets[0];
  const CSSPropertyValueSet** user_sets = &all_sets[2];
  const CSSPropertyValueSet** author_sets = &all_sets[4];

  MatchResult result;

  result.AddMatchedProperties(ua_sets[0]);
  result.AddMatchedProperties(ua_sets[1]);
  result.FinishAddingUARules();

  result.AddMatchedProperties(user_sets[0]);
  result.AddMatchedProperties(user_sets[1]);
  result.FinishAddingUserRules();

  result.AddMatchedProperties(author_sets[0]);
  result.AddMatchedProperties(author_sets[1]);
  result.FinishAddingAuthorRulesForTreeScope();

  result.AddMatchedProperties(author_sets[2]);
  result.AddMatchedProperties(author_sets[3]);
  result.FinishAddingAuthorRulesForTreeScope();

  TestMatchedPropertiesRange(result.AllRules(), 8, all_sets);
  TestMatchedPropertiesRange(result.UaRules(), 2, ua_sets);
  TestMatchedPropertiesRange(result.UserRules(), 2, user_sets);
  TestMatchedPropertiesRange(result.AuthorRules(), 4, author_sets);
}

TEST_F(MatchResultTest, CascadeOriginUserAgent) {
  MatchResult result;
  result.AddMatchedProperties(PropertySet(0));
  result.AddMatchedProperties(PropertySet(1));
  result.FinishAddingUARules();
  result.FinishAddingUserRules();
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 2u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUserAgent);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kUserAgent);
}

TEST_F(MatchResultTest, CascadeOriginUser) {
  MatchResult result;
  result.FinishAddingUARules();
  result.AddMatchedProperties(PropertySet(0));
  result.AddMatchedProperties(PropertySet(1));
  result.FinishAddingUserRules();
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 2u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kUser);
}

TEST_F(MatchResultTest, CascadeOriginAuthor) {
  MatchResult result;
  result.FinishAddingUARules();
  result.FinishAddingUserRules();
  result.AddMatchedProperties(PropertySet(0));
  result.AddMatchedProperties(PropertySet(1));
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 2u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kAuthor);
}

TEST_F(MatchResultTest, CascadeOriginAll) {
  MatchResult result;
  result.AddMatchedProperties(PropertySet(0));
  result.FinishAddingUARules();
  result.AddMatchedProperties(PropertySet(1));
  result.AddMatchedProperties(PropertySet(2));
  result.FinishAddingUserRules();
  result.AddMatchedProperties(PropertySet(3));
  result.AddMatchedProperties(PropertySet(4));
  result.AddMatchedProperties(PropertySet(5));
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 6u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUserAgent);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 2), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 3), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 4), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 5), CascadeOrigin::kAuthor);
}

TEST_F(MatchResultTest, CascadeOriginAllExceptUserAgent) {
  MatchResult result;
  result.FinishAddingUARules();
  result.AddMatchedProperties(PropertySet(1));
  result.AddMatchedProperties(PropertySet(2));
  result.FinishAddingUserRules();
  result.AddMatchedProperties(PropertySet(3));
  result.AddMatchedProperties(PropertySet(4));
  result.AddMatchedProperties(PropertySet(5));
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 5u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 2), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 3), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 4), CascadeOrigin::kAuthor);
}

TEST_F(MatchResultTest, CascadeOriginAllExceptUser) {
  MatchResult result;
  result.AddMatchedProperties(PropertySet(0));
  result.FinishAddingUARules();
  result.FinishAddingUserRules();
  result.AddMatchedProperties(PropertySet(3));
  result.AddMatchedProperties(PropertySet(4));
  result.AddMatchedProperties(PropertySet(5));
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 4u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUserAgent);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 2), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 3), CascadeOrigin::kAuthor);
}

TEST_F(MatchResultTest, CascadeOriginAllExceptAuthor) {
  MatchResult result;
  result.AddMatchedProperties(PropertySet(0));
  result.FinishAddingUARules();
  result.AddMatchedProperties(PropertySet(1));
  result.AddMatchedProperties(PropertySet(2));
  result.FinishAddingUserRules();
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 3u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUserAgent);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 2), CascadeOrigin::kUser);
}

TEST_F(MatchResultTest, CascadeOriginTreeScopes) {
  MatchResult result;
  result.AddMatchedProperties(PropertySet(0));
  result.FinishAddingUARules();
  result.AddMatchedProperties(PropertySet(1));
  result.FinishAddingUserRules();
  result.AddMatchedProperties(PropertySet(2));
  result.FinishAddingAuthorRulesForTreeScope();
  result.AddMatchedProperties(PropertySet(3));
  result.AddMatchedProperties(PropertySet(4));
  result.FinishAddingAuthorRulesForTreeScope();
  result.AddMatchedProperties(PropertySet(5));
  result.AddMatchedProperties(PropertySet(6));
  result.AddMatchedProperties(PropertySet(7));
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 8u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUserAgent);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 2), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 3), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 4), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 5), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 6), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 7), CascadeOrigin::kAuthor);
}

TEST_F(MatchResultTest, ExpansionsRange) {
  MatchResult result;
  result.AddMatchedProperties(ParseDeclarationBlock("left:1px;all:unset"));
  result.AddMatchedProperties(ParseDeclarationBlock("color:red"));
  result.FinishAddingUARules();
  result.AddMatchedProperties(ParseDeclarationBlock("display:block"));
  result.FinishAddingUserRules();
  result.AddMatchedProperties(ParseDeclarationBlock("left:unset"));
  result.AddMatchedProperties(ParseDeclarationBlock("top:unset"));
  result.AddMatchedProperties(
      ParseDeclarationBlock("right:unset;bottom:unset"));
  result.FinishAddingAuthorRulesForTreeScope();

  CascadeFilter filter;

  size_t i = 0;
  size_t size = result.GetMatchedProperties().size();
  for (auto actual : result.Expansions(GetDocument(), filter)) {
    ASSERT_LT(i, size);
    CascadeExpansion expected(result.GetMatchedProperties()[i], GetDocument(),
                              filter, i);
    EXPECT_EQ(expected.Id(), actual.Id());
    EXPECT_EQ(expected.Priority(), actual.Priority());
    EXPECT_EQ(expected.Value(), actual.Value());
    ++i;
  }

  EXPECT_EQ(6u, i);
}

TEST_F(MatchResultTest, EmptyExpansionsRange) {
  MatchResult result;
  result.FinishAddingUARules();
  result.FinishAddingUserRules();
  result.FinishAddingAuthorRulesForTreeScope();

  CascadeFilter filter;
  auto range = result.Expansions(GetDocument(), filter);
  EXPECT_EQ(range.end(), range.begin());
}

TEST_F(MatchResultTest, Reset) {
  MatchResult result;
  result.AddMatchedProperties(PropertySet(0));
  result.FinishAddingUARules();
  result.AddMatchedProperties(PropertySet(1));
  result.FinishAddingUserRules();
  result.AddMatchedProperties(PropertySet(2));
  result.FinishAddingAuthorRulesForTreeScope();
  result.AddMatchedProperties(PropertySet(3));
  result.FinishAddingAuthorRulesForTreeScope();
  result.AddMatchedProperties(PropertySet(4));
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 5u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUserAgent);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 2), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 3), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 4), CascadeOrigin::kAuthor);

  // Check tree_order of last entry.
  EXPECT_TRUE(result.HasMatchedProperties());
  ASSERT_EQ(5u, result.GetMatchedProperties().size());
  EXPECT_EQ(2u, result.GetMatchedProperties()[4].types_.tree_order);

  EXPECT_TRUE(result.IsCacheable());
  result.SetIsCacheable(false);
  EXPECT_FALSE(result.IsCacheable());

  result.Reset();

  EXPECT_TRUE(result.IsCacheable());
  EXPECT_FALSE(result.GetMatchedProperties().size());
  EXPECT_FALSE(result.HasMatchedProperties());

  // Add same declarations again.
  result.AddMatchedProperties(PropertySet(0));
  result.FinishAddingUARules();
  result.AddMatchedProperties(PropertySet(1));
  result.FinishAddingUserRules();
  result.AddMatchedProperties(PropertySet(2));
  result.FinishAddingAuthorRulesForTreeScope();
  result.AddMatchedProperties(PropertySet(3));
  result.FinishAddingAuthorRulesForTreeScope();
  result.AddMatchedProperties(PropertySet(4));
  result.FinishAddingAuthorRulesForTreeScope();

  ASSERT_EQ(LengthOf(result), 5u);
  EXPECT_EQ(OriginAt(result, 0), CascadeOrigin::kUserAgent);
  EXPECT_EQ(OriginAt(result, 1), CascadeOrigin::kUser);
  EXPECT_EQ(OriginAt(result, 2), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 3), CascadeOrigin::kAuthor);
  EXPECT_EQ(OriginAt(result, 4), CascadeOrigin::kAuthor);

  // Check tree_order of last entry.
  EXPECT_TRUE(result.HasMatchedProperties());
  ASSERT_EQ(5u, result.GetMatchedProperties().size());
  EXPECT_EQ(2u, result.GetMatchedProperties()[4].types_.tree_order);

  EXPECT_TRUE(result.IsCacheable());
}

}  // namespace blink
