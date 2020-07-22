// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/layout/ng/grid/ng_grid_track_collection.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_test.h"

namespace blink {
namespace {

#define EXPECT_RANGE(expected_start, expected_count, iterator)              \
  EXPECT_EQ(expected_count, iterator.RepeatCount());                        \
  EXPECT_EQ(expected_start, iterator.RangeTrackStart());                    \
  EXPECT_EQ(expected_start + expected_count - 1, iterator.RangeTrackEnd()); \
  EXPECT_FALSE(iterator.IsRangeCollapsed());
#define EXPECT_COLLAPSED_RANGE(expected_start, expected_count, iterator)    \
  EXPECT_EQ(expected_start, iterator.RangeTrackStart());                    \
  EXPECT_EQ(expected_count, iterator.RepeatCount());                        \
  EXPECT_EQ(expected_start + expected_count - 1, iterator.RangeTrackEnd()); \
  EXPECT_TRUE(iterator.IsRangeCollapsed());
#define EXPECT_SET(expected_size, expected_count, iterator)    \
  EXPECT_EQ(expected_size, iterator.CurrentSet().TrackSize()); \
  EXPECT_EQ(expected_count, iterator.CurrentSet().TrackCount());

class NGGridTrackCollectionBaseTest : public NGGridTrackCollectionBase {
 public:
  struct TestTrackRange {
    wtf_size_t track_number;
    wtf_size_t track_count;
  };
  explicit NGGridTrackCollectionBaseTest(
      const std::vector<wtf_size_t>& range_sizes) {
    wtf_size_t track_number = 0;
    for (wtf_size_t size : range_sizes) {
      TestTrackRange range;
      range.track_number = track_number;
      range.track_count = size;
      ranges_.push_back(range);
      track_number += size;
    }
  }

 protected:
  wtf_size_t RangeTrackNumber(wtf_size_t range_index) const override {
    return ranges_[range_index].track_number;
  }
  wtf_size_t RangeTrackCount(wtf_size_t range_index) const override {
    return ranges_[range_index].track_count;
  }
  bool IsRangeCollapsed(wtf_size_t range_index) const override { return false; }

  wtf_size_t RangeCount() const override { return ranges_.size(); }

 private:
  Vector<TestTrackRange> ranges_;
};

class NGGridTrackCollectionTest : public NGLayoutTest {
 protected:
  Vector<GridTrackSize> CreateTrackSizes(wtf_size_t track_count) {
    return {track_count, GridTrackSize(Length::Auto())};
  }
};

TEST_F(NGGridTrackCollectionTest, TestRangeIndexFromTrackNumber) {
  // Small case.
  NGGridTrackCollectionBaseTest track_collection({3, 10u, 5u});
  EXPECT_EQ(0u, track_collection.RangeIndexFromTrackNumber(0u));
  EXPECT_EQ(1u, track_collection.RangeIndexFromTrackNumber(4u));
  EXPECT_EQ(2u, track_collection.RangeIndexFromTrackNumber(15u));

  // Small case with large repeat count.
  track_collection = NGGridTrackCollectionBaseTest({3000000u, 7u, 10u});
  EXPECT_EQ(0u, track_collection.RangeIndexFromTrackNumber(600u));
  EXPECT_EQ(1u, track_collection.RangeIndexFromTrackNumber(3000000u));
  EXPECT_EQ(1u, track_collection.RangeIndexFromTrackNumber(3000004u));

  // Larger case.
  track_collection = NGGridTrackCollectionBaseTest({
      10u,   // 0 - 9
      10u,   // 10 - 19
      10u,   // 20 - 29
      10u,   // 30 - 39
      20u,   // 40 - 59
      20u,   // 60 - 79
      20u,   // 80 - 99
      100u,  // 100 - 199
  });
  EXPECT_EQ(0u, track_collection.RangeIndexFromTrackNumber(0u));
  EXPECT_EQ(3u, track_collection.RangeIndexFromTrackNumber(35u));
  EXPECT_EQ(4u, track_collection.RangeIndexFromTrackNumber(40u));
  EXPECT_EQ(5u, track_collection.RangeIndexFromTrackNumber(79));
  EXPECT_EQ(7u, track_collection.RangeIndexFromTrackNumber(105u));
}

TEST_F(NGGridTrackCollectionTest, TestRangeRepeatIteratorMoveNext) {
  // [0-2] [3-12] [13-17]
  NGGridTrackCollectionBaseTest track_collection({3u, 10u, 5u});
  EXPECT_EQ(0u, track_collection.RangeIndexFromTrackNumber(0u));

  NGGridTrackCollectionBaseTest::RangeRepeatIterator iterator(&track_collection,
                                                              0u);
  EXPECT_RANGE(0u, 3u, iterator);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_RANGE(3u, 10u, iterator);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_RANGE(13u, 5u, iterator);

  EXPECT_FALSE(iterator.MoveToNextRange());

  NGGridTrackCollectionBaseTest empty_collection({});

  NGGridTrackCollectionBaseTest::RangeRepeatIterator empty_iterator(
      &empty_collection, 0u);
  EXPECT_EQ(NGGridTrackCollectionBase::kInvalidRangeIndex,
            empty_iterator.RangeTrackStart());
  EXPECT_EQ(NGGridTrackCollectionBase::kInvalidRangeIndex,
            empty_iterator.RangeTrackEnd());
  EXPECT_EQ(0u, empty_iterator.RepeatCount());
  EXPECT_FALSE(empty_iterator.MoveToNextRange());
}

TEST_F(NGGridTrackCollectionTest, TestNGGridTrackList) {
  NGGridTrackList track_list;
  ASSERT_EQ(0u, track_list.RepeaterCount());
  EXPECT_FALSE(track_list.HasAutoRepeater());

  EXPECT_TRUE(track_list.AddRepeater(CreateTrackSizes(2), 4));
  ASSERT_EQ(1u, track_list.RepeaterCount());
  EXPECT_EQ(8u, track_list.TotalTrackCount());
  EXPECT_EQ(4u, track_list.RepeatCount(0, 4));
  EXPECT_EQ(2u, track_list.RepeatSize(0));
  EXPECT_FALSE(track_list.HasAutoRepeater());

  // Can't add an empty repeater to a list.
  EXPECT_FALSE(track_list.AddAutoRepeater(
      CreateTrackSizes(0), NGGridTrackRepeater::RepeatType::kAutoFit));
  EXPECT_FALSE(track_list.AddRepeater(CreateTrackSizes(3), 0));
  ASSERT_EQ(1u, track_list.RepeaterCount());

  EXPECT_TRUE(track_list.AddRepeater(CreateTrackSizes(1), kNotFound - 20));
  ASSERT_EQ(2u, track_list.RepeaterCount());
  EXPECT_EQ(kNotFound - 12u, track_list.TotalTrackCount());
  EXPECT_EQ(kNotFound - 20u, track_list.RepeatCount(1, 4));
  EXPECT_EQ(1u, track_list.RepeatSize(1));
  EXPECT_FALSE(track_list.HasAutoRepeater());

  // Try to add a repeater that would overflow the total track count.
  EXPECT_FALSE(track_list.AddRepeater(CreateTrackSizes(5), 7));
  EXPECT_FALSE(track_list.AddRepeater(CreateTrackSizes(7), 5));
  EXPECT_FALSE(track_list.AddAutoRepeater(
      CreateTrackSizes(31), NGGridTrackRepeater::RepeatType::kAutoFill));
  ASSERT_EQ(2u, track_list.RepeaterCount());

  EXPECT_TRUE(track_list.AddAutoRepeater(
      CreateTrackSizes(3), NGGridTrackRepeater::RepeatType::kAutoFit));
  ASSERT_EQ(3u, track_list.RepeaterCount());
  EXPECT_EQ(kNotFound - 9u, track_list.TotalTrackCount());
  EXPECT_EQ(4u, track_list.RepeatCount(2, 4));
  EXPECT_EQ(3u, track_list.RepeatSize(2));
  EXPECT_TRUE(track_list.HasAutoRepeater());

  // Can't add more than one auto repeater to a list.
  EXPECT_FALSE(track_list.AddAutoRepeater(
      CreateTrackSizes(3), NGGridTrackRepeater::RepeatType::kAutoFill));
  ASSERT_EQ(3u, track_list.RepeaterCount());
}

TEST_F(NGGridTrackCollectionTest, TestNGGridBlockTrackCollection) {
  NGGridTrackList explicit_tracks, implicit_tracks;
  ASSERT_TRUE(explicit_tracks.AddRepeater(CreateTrackSizes(2), 4));
  ASSERT_TRUE(explicit_tracks.AddAutoRepeater(
      CreateTrackSizes(3), NGGridTrackRepeater::RepeatType::kAutoFill));
  ASSERT_EQ(2u, explicit_tracks.RepeaterCount());
  NGGridBlockTrackCollection block_collection;
  block_collection.SetSpecifiedTracks(&explicit_tracks, &implicit_tracks, 3);
  block_collection.FinalizeRanges();

  NGGridTrackCollectionBase::RangeRepeatIterator iterator(&block_collection,
                                                          0u);
  EXPECT_RANGE(1u, 8u, iterator);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_RANGE(9u, 9u, iterator);

  EXPECT_FALSE(iterator.MoveToNextRange());
}

TEST_F(NGGridTrackCollectionTest, TestNGGridBlockTrackCollectionCollapsed) {
  NGGridTrackList explicit_tracks, implicit_tracks;
  ASSERT_TRUE(explicit_tracks.AddRepeater(CreateTrackSizes(2), 4));
  ASSERT_TRUE(explicit_tracks.AddAutoRepeater(
      CreateTrackSizes(3), NGGridTrackRepeater::RepeatType::kAutoFit));
  ASSERT_TRUE(explicit_tracks.AddRepeater(CreateTrackSizes(3), 7));
  ASSERT_EQ(3u, explicit_tracks.RepeaterCount());
  NGGridBlockTrackCollection block_collection;
  block_collection.SetSpecifiedTracks(&explicit_tracks, &implicit_tracks, 3);
  block_collection.FinalizeRanges();

  NGGridTrackCollectionBase::RangeRepeatIterator iterator(&block_collection,
                                                          0u);
  EXPECT_RANGE(1u, 8u, iterator);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_COLLAPSED_RANGE(9u, 9u, iterator);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_RANGE(18u, 21u, iterator);

  EXPECT_FALSE(iterator.MoveToNextRange());
}

TEST_F(NGGridTrackCollectionTest, TestNGGridBlockTrackCollectionImplicit) {
  NGGridTrackList explicit_tracks;
  ASSERT_TRUE(explicit_tracks.AddRepeater(CreateTrackSizes(2), 4));
  ASSERT_TRUE(explicit_tracks.AddRepeater(CreateTrackSizes(3), 3));
  ASSERT_TRUE(explicit_tracks.AddRepeater(CreateTrackSizes(3), 7));
  ASSERT_EQ(3u, explicit_tracks.RepeaterCount());

  NGGridTrackList implicit_tracks;
  ASSERT_TRUE(implicit_tracks.AddRepeater(CreateTrackSizes(8), 2));

  NGGridBlockTrackCollection block_collection;
  block_collection.SetSpecifiedTracks(&explicit_tracks, &implicit_tracks, 3);
  block_collection.EnsureTrackCoverage(3, 40);
  block_collection.EnsureTrackCoverage(3, 40);
  block_collection.FinalizeRanges();
  NGGridTrackCollectionBase::RangeRepeatIterator iterator(&block_collection,
                                                          0u);
  EXPECT_RANGE(1u, 2u, iterator);
  EXPECT_FALSE(block_collection.RangeAtTrackNumber(1u).is_implicit_range);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_RANGE(3u, 6u, iterator);
  EXPECT_FALSE(block_collection.RangeAtTrackNumber(4).is_implicit_range);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_RANGE(9u, 9u, iterator);
  EXPECT_FALSE(block_collection.RangeAtTrackNumber(7).is_implicit_range);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_RANGE(18u, 21u, iterator);
  EXPECT_FALSE(block_collection.RangeAtTrackNumber(20).is_implicit_range);

  EXPECT_TRUE(iterator.MoveToNextRange());
  EXPECT_TRUE(block_collection.RangeAtTrackNumber(40).is_implicit_range);
  EXPECT_RANGE(39u, 4u, iterator);

  EXPECT_FALSE(iterator.MoveToNextRange());
}

TEST_F(NGGridTrackCollectionTest,
       TestNGGridLayoutAlgorithmTrackCollectionSetIterator) {
  Vector<wtf_size_t> set_counts = {2, 5, 3, 11, 13, 7};

  NGGridTrackList explicit_tracks, implicit_tracks;
  for (wtf_size_t set_count : set_counts)
    ASSERT_TRUE(explicit_tracks.AddRepeater(CreateTrackSizes(set_count), 1));
  ASSERT_EQ(set_counts.size(), explicit_tracks.RepeaterCount());

  NGGridBlockTrackCollection block_collection;
  block_collection.SetSpecifiedTracks(&explicit_tracks, &implicit_tracks,
                                      /* auto_repeat_count */ 0);
  block_collection.FinalizeRanges();
  NGGridLayoutAlgorithmTrackCollection algorithm_collection(block_collection);

  wtf_size_t set_count = 0;
  wtf_size_t range_count = 0;
  NGGridTrackCollectionBase::RangeRepeatIterator range_iterator =
      algorithm_collection.RangeIterator();

  for (; !range_iterator.IsAtEnd(); ++range_count) {
    EXPECT_RANGE(1u + set_count, set_counts[range_count], range_iterator);

    wtf_size_t current_set_count = 0;
    NGGridLayoutAlgorithmTrackCollection::SetIterator set_iterator =
        algorithm_collection.IteratorForRange(range_iterator.RangeIndex());

    for (; !set_iterator.IsAtEnd(); ++current_set_count) {
      EXPECT_SET(GridTrackSize(Length::Auto()), 1u, set_iterator);
      set_iterator.MoveToNextSet();
    }

    EXPECT_EQ(set_counts[range_count], current_set_count);
    set_count += current_set_count;
    range_iterator.MoveToNextRange();
  }

  EXPECT_EQ(set_counts.size(), range_count);
  EXPECT_EQ(41u, set_count);
}

TEST_F(NGGridTrackCollectionTest,
       TestNGGridLayoutAlgorithmTrackCollectionExplicitTracks) {
  // We'll use fixed size tracks to differentiate between them by int value.
  NGGridTrackList explicit_tracks, implicit_tracks;

  // repeat(3, 1px 2px 3px)
  Vector<GridTrackSize> track_sizes = {GridTrackSize(Length::Fixed(1)),
                                       GridTrackSize(Length::Fixed(2)),
                                       GridTrackSize(Length::Fixed(3))};
  ASSERT_TRUE(explicit_tracks.AddRepeater(track_sizes, 3));

  // repeat(auto-fit, 4px 5px)
  track_sizes = {GridTrackSize(Length::Fixed(4)),
                 GridTrackSize(Length::Fixed(5))};
  ASSERT_TRUE(explicit_tracks.AddAutoRepeater(
      track_sizes, NGGridTrackRepeater::RepeatType::kAutoFit));
  ASSERT_EQ(2u, explicit_tracks.RepeaterCount());

  NGGridBlockTrackCollection block_collection;
  block_collection.SetSpecifiedTracks(&explicit_tracks, &implicit_tracks,
                                      /* auto_repeat_count */ 5);
  block_collection.EnsureTrackCoverage(3, 4);
  block_collection.EnsureTrackCoverage(13, 4);
  block_collection.EnsureTrackCoverage(18, 3);
  block_collection.EnsureTrackCoverage(23, 5);
  block_collection.FinalizeRanges();

  NGGridLayoutAlgorithmTrackCollection algorithm_collection(block_collection);
  NGGridTrackCollectionBase::RangeRepeatIterator range_iterator =
      algorithm_collection.RangeIterator();

  EXPECT_RANGE(1u, 2u, range_iterator);
  NGGridLayoutAlgorithmTrackCollection::SetIterator set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(1)), 1u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(2)), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(3u, 4u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(3)), 2u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(1)), 1u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(2)), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(7u, 3u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(1)), 1u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(2)), 1u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(3)), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_COLLAPSED_RANGE(10u, 3u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(0)), 3u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(13u, 4u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(5)), 2u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(4)), 2u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_COLLAPSED_RANGE(17u, 1u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(0)), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(18u, 2u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(4)), 1u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(5)), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(20u, 1u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Auto()), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(21u, 2u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Auto()), 2u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(23u, 5u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Auto()), 5u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_FALSE(range_iterator.MoveToNextRange());
}

TEST_F(NGGridTrackCollectionTest,
       TestNGGridLayoutAlgorithmTrackCollectionImplicitTracks) {
  // We'll use fixed size tracks to differentiate between them by int value.
  NGGridTrackList explicit_tracks, implicit_tracks;

  // Explicit grid: 1px 2px 3px 4px
  Vector<GridTrackSize> track_sizes = {
      GridTrackSize(Length::Fixed(1)), GridTrackSize(Length::Fixed(2)),
      GridTrackSize(Length::Fixed(3)), GridTrackSize(Length::Fixed(4))};
  ASSERT_TRUE(explicit_tracks.AddRepeater(track_sizes, 1));
  ASSERT_EQ(1u, explicit_tracks.RepeaterCount());

  // Implicit grid: 5px 6px 7px
  track_sizes = {GridTrackSize(Length::Fixed(5)),
                 GridTrackSize(Length::Fixed(6)),
                 GridTrackSize(Length::Fixed(7))};
  ASSERT_TRUE(implicit_tracks.AddRepeater(track_sizes, 1));
  ASSERT_EQ(1u, implicit_tracks.RepeaterCount());

  NGGridBlockTrackCollection block_collection;
  block_collection.SetSpecifiedTracks(&explicit_tracks, &implicit_tracks,
                                      /* auto_repeat_count */ 0);
  block_collection.EnsureTrackCoverage(3, 13);
  block_collection.EnsureTrackCoverage(24, 2);
  block_collection.FinalizeRanges();

  NGGridLayoutAlgorithmTrackCollection algorithm_collection(block_collection);
  NGGridTrackCollectionBase::RangeRepeatIterator range_iterator =
      algorithm_collection.RangeIterator();

  EXPECT_RANGE(1u, 2u, range_iterator);
  NGGridLayoutAlgorithmTrackCollection::SetIterator set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(1)), 1u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(2)), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(3u, 2u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(3)), 1u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(4)), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(5u, 11u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(5)), 4u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(6)), 4u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(7)), 3u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(16u, 8u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(7)), 3u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(5)), 3u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(6)), 2u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_TRUE(range_iterator.MoveToNextRange());

  EXPECT_RANGE(24u, 2u, range_iterator);
  set_iterator =
      algorithm_collection.IteratorForRange(range_iterator.RangeIndex());
  EXPECT_SET(GridTrackSize(Length::Fixed(6)), 1u, set_iterator);
  EXPECT_TRUE(set_iterator.MoveToNextSet());
  EXPECT_SET(GridTrackSize(Length::Fixed(7)), 1u, set_iterator);
  EXPECT_FALSE(set_iterator.MoveToNextSet());
  EXPECT_FALSE(range_iterator.MoveToNextRange());
}

}  // namespace

}  // namespace blink
