// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code if governed by a BSD-style license that can be
// found in LICENSE file.

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

using testing::AnyOf;
using testing::ElementsAre;

namespace blink {

class DisableBackgroundThrottlingIsRespectedTest
    : public SimTest,
      private ScopedTimerThrottlingForBackgroundTabsForTest {
 public:
  DisableBackgroundThrottlingIsRespectedTest()
      : ScopedTimerThrottlingForBackgroundTabsForTest(false) {}
  void SetUp() override { SimTest::SetUp(); }
};

TEST_F(DisableBackgroundThrottlingIsRespectedTest,
       DisableBackgroundThrottlingIsRespected) {
  SimRequest main_resource("https://example.com/", "text/html");

  LoadURL("https://example.com/");

  main_resource.Complete(
      "(<script>"
      "  function f(repetitions) {"
      "     if (repetitions == 0) return;"
      "     console.log('called f');"
      "     setTimeout(f, 10, repetitions - 1);"
      "  }"
      "  f(5);"
      "</script>)");

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  // Run delayed tasks for 1 second. All tasks should be completed
  // with throttling disabled.
  test::RunDelayedTasks(base::TimeDelta::FromSeconds(1));

  EXPECT_THAT(ConsoleMessages(), ElementsAre("called f", "called f", "called f",
                                             "called f", "called f"));
}

class BackgroundPageThrottlingTest : public SimTest {};

TEST_F(BackgroundPageThrottlingTest, BackgroundPagesAreThrottled) {
  SimRequest main_resource("https://example.com/", "text/html");

  LoadURL("https://example.com/");

  main_resource.Complete(
      "(<script>"
      "  function f(repetitions) {"
      "     if (repetitions == 0) return;"
      "     console.log('called f');"
      "     setTimeout(f, 10, repetitions - 1);"
      "  }"
      "  setTimeout(f, 10, 50);"
      "</script>)");

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  // Make sure that we run no more than one task a second.
  test::RunDelayedTasks(base::TimeDelta::FromMilliseconds(3000));
  EXPECT_THAT(
      ConsoleMessages(),
      AnyOf(ElementsAre("called f", "called f", "called f"),
            ElementsAre("called f", "called f", "called f", "called f")));
}

class IntensiveWakeUpThrottlingTest : public SimTest {
 public:
  IntensiveWakeUpThrottlingTest() {
    scoped_feature_list_.InitWithFeatures(
        {features::kIntensiveWakeUpThrottling},
        // Disable freezing because it hides the effect of intensive throttling.
        {features::kStopInBackground});

    platform_->SetAutoAdvanceNowToPendingTasks(false);

    // Align the time on a 1-minute interval, to simplify expectations.
    platform_->AdvanceClock(
        platform_->NowTicks().SnappedToNextTick(
            base::TimeTicks(), base::TimeDelta::FromMinutes(1)) -
        platform_->NowTicks());
  }

  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

namespace {

// A script that waits 5 minutes, then creates a timer that reschedules itself
// 50 times with 10 ms delay.
constexpr char kRepeatingTimerScript[] =
    "<script>"
    "  function onTimer(repetitions) {"
    "     if (repetitions == 0) return;"
    "     console.log('called onTimer');"
    "     setTimeout(onTimer, 10, repetitions - 1);"
    "  }"
    "  function afterFiveMinutes() {"
    "    setTimeout(onTimer, 10, 50);"
    "  }"
    "  setTimeout(afterFiveMinutes, 5 * 60 * 1000);"
    "</script>";

// A script that schedules a timer with a long delay that is not aligned on the
// intensive throttling wake up interval.
constexpr char kLongUnalignedTimerScript[] =
    "<script>"
    "  function onTimer() {"
    "     console.log('called onTimer');"
    "  }"
    "  setTimeout(onTimer, 342 * 1000);"
    "</script>";

// A time delta that matches the delay in the above script.
constexpr base::TimeDelta kLongUnalignedTimerDelay =
    base::TimeDelta::FromSeconds(342);

}  // namespace

// Verify that a main frame timer that reposts itself with a 10 ms timeout runs
// once every minute.
TEST_F(IntensiveWakeUpThrottlingTest, MainFrameTimer_ShortTimeout) {
  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(kRepeatingTimerScript);

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  // No timer is scheduled in the 5 first minutes.
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(5));
  EXPECT_THAT(ConsoleMessages(), ElementsAre());

  // After that, intensive throttling starts and there should be 1 wake up per
  // minute.
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(1));
  EXPECT_THAT(ConsoleMessages(), ElementsAre("called onTimer"));
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(1));
  EXPECT_THAT(ConsoleMessages(),
              ElementsAre("called onTimer", "called onTimer"));
}

// Verify that a same-origin subframe timer that reposts itself with a 10 ms
// timeout runs once every minute.
TEST_F(IntensiveWakeUpThrottlingTest, SameOriginSubFrameTimer_ShortTimeout) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest subframe_resource("https://example.com/iframe.html", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(R"(<iframe src="https://example.com/iframe.html" />)");
  // Run tasks to let the main frame request the iframe resource. It is not
  // possible to complete the iframe resource request before that.
  platform_->RunUntilIdle();
  subframe_resource.Complete(kRepeatingTimerScript);

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  // No timer is scheduled in the 5 first minutes.
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(5));
  EXPECT_THAT(ConsoleMessages(), ElementsAre());

  // After that, intensive throttling starts and there should be 1 wake up per
  // minute.
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(1));
  EXPECT_THAT(ConsoleMessages(), ElementsAre("called onTimer"));
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(1));
  EXPECT_THAT(ConsoleMessages(),
              ElementsAre("called onTimer", "called onTimer"));
}

// Verify that a cross-origin subframe timer that reposts itself with a 10 ms
// timeout runs once every minute.
TEST_F(IntensiveWakeUpThrottlingTest, CrossOriginSubFrameTimer_ShortTimeout) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest subframe_resource("https://cross-origin.example.com/iframe.html",
                               "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(
      R"(<iframe src="https://cross-origin.example.com/iframe.html" />)");
  // Run tasks to let the main frame request the iframe resource. It is not
  // possible to complete the iframe resource request before that.
  platform_->RunUntilIdle();
  subframe_resource.Complete(kRepeatingTimerScript);

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  // No timer is scheduled in the 5 first minutes.
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(5));
  EXPECT_THAT(ConsoleMessages(), ElementsAre());

  // After that, intensive throttling starts and there should be 1 wake up per
  // minute.
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(1));
  EXPECT_THAT(ConsoleMessages(), ElementsAre("called onTimer"));
  platform_->RunForPeriod(base::TimeDelta::FromMinutes(1));
  EXPECT_THAT(ConsoleMessages(),
              ElementsAre("called onTimer", "called onTimer"));
}

// Verify that a main frame timer with a long timeout runs at the desired run
// time when there is no other recent timer wake up.
TEST_F(IntensiveWakeUpThrottlingTest, MainFrameTimer_LongUnalignedTimeout) {
  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(kLongUnalignedTimerScript);

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  platform_->RunForPeriod(kLongUnalignedTimerDelay -
                          base::TimeDelta::FromSeconds(1));
  EXPECT_THAT(ConsoleMessages(), ElementsAre());

  platform_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_THAT(ConsoleMessages(), ElementsAre("called onTimer"));
}

// Verify that a same-origin subframe timer with a long timeout runs at the
// desired run time when there is no other recent timer wake up.
TEST_F(IntensiveWakeUpThrottlingTest,
       SameOriginSubFrameTimer_LongUnalignedTimeout) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest subframe_resource("https://example.com/iframe.html", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(R"(<iframe src="https://example.com/iframe.html" />)");
  // Run tasks to let the main frame request the iframe resource. It is not
  // possible to complete the iframe resource request before that.
  platform_->RunUntilIdle();
  subframe_resource.Complete(kLongUnalignedTimerScript);

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  platform_->RunForPeriod(kLongUnalignedTimerDelay -
                          base::TimeDelta::FromSeconds(1));
  EXPECT_THAT(ConsoleMessages(), ElementsAre());

  platform_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_THAT(ConsoleMessages(), ElementsAre("called onTimer"));
}

// Verify that a cross-origin subframe timer with a long timeout runs at an
// aligned time, even when there is no other recent timer wake up (in a
// same-origin frame, it would have run at the desired time).
TEST_F(IntensiveWakeUpThrottlingTest,
       CrossOriginSubFrameTimer_LongUnalignedTimeout) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest subframe_resource("https://cross-origin.example.com/iframe.html",
                               "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(
      R"(<iframe src="https://cross-origin.example.com/iframe.html" />)");
  // Run tasks to let the main frame request the iframe resource. It is not
  // possible to complete the iframe resource request before that.
  platform_->RunUntilIdle();
  subframe_resource.Complete(kLongUnalignedTimerScript);

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  platform_->RunForPeriod(base::TimeDelta::FromSeconds(342));
  EXPECT_THAT(ConsoleMessages(), ElementsAre());

  // Fast-forward to the next aligned time.
  platform_->RunForPeriod(base::TimeDelta::FromSeconds(18));
  EXPECT_THAT(ConsoleMessages(), ElementsAre("called onTimer"));
}

// Verify that if both the main frame and a cross-origin frame schedule a timer
// with a long unaligned delay, the main frame timer runs at the desired time
// (because there was no recent same-origin wake up) while the cross-origin
// timer runs at an aligned time.
TEST_F(IntensiveWakeUpThrottlingTest,
       MainFrameAndCrossOriginSubFrameTimer_LongUnalignedTimeout) {
  SimRequest main_resource("https://example.com/", "text/html");
  SimRequest subframe_resource("https://cross-origin.example.com/iframe.html",
                               "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(
      WTF::String(kLongUnalignedTimerScript) +
      "<iframe src=\"https://cross-origin.example.com/iframe.html\" />");
  // Run tasks to let the main frame request the iframe resource. It is not
  // possible to complete the iframe resource request before that.
  platform_->RunUntilIdle();
  subframe_resource.Complete(kLongUnalignedTimerScript);

  GetDocument().GetPage()->GetPageScheduler()->SetPageVisible(false);

  platform_->RunForPeriod(base::TimeDelta::FromSeconds(342));
  EXPECT_THAT(ConsoleMessages(), ElementsAre("called onTimer"));

  // Fast-forward to the next aligned time.
  platform_->RunForPeriod(base::TimeDelta::FromSeconds(18));
  EXPECT_THAT(ConsoleMessages(),
              ElementsAre("called onTimer", "called onTimer"));
}

}  // namespace blink
