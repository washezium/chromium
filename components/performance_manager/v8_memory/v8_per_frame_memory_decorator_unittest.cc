// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/performance_manager/public/v8_memory/v8_per_frame_memory_decorator.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/bind_test_util.h"
#include "base/test/gtest_util.h"
#include "base/time/time.h"
#include "components/performance_manager/graph/frame_node_impl.h"
#include "components/performance_manager/graph/page_node_impl.h"
#include "components/performance_manager/graph/process_node_impl.h"
#include "components/performance_manager/public/performance_manager.h"
#include "components/performance_manager/public/render_process_host_id.h"
#include "components/performance_manager/public/render_process_host_proxy.h"
#include "components/performance_manager/test_support/graph_test_harness.h"
#include "components/performance_manager/test_support/performance_manager_test_harness.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace performance_manager {

using testing::_;

constexpr RenderProcessHostId kTestProcessID = RenderProcessHostId(0xFAB);
constexpr uint64_t kUnassociatedBytes = 0xABBA;

namespace {

using FrameData = V8PerFrameMemoryDecorator::FrameData;
using ProcessData = V8PerFrameMemoryDecorator::ProcessData;

class LenientMockV8PerFrameMemoryReporter
    : public mojom::V8PerFrameMemoryReporter {
 public:
  LenientMockV8PerFrameMemoryReporter() : receiver_(this) {}

  MOCK_METHOD(void,
              GetPerFrameV8MemoryUsageData,
              (GetPerFrameV8MemoryUsageDataCallback callback),
              (override));

  void Bind(
      mojo::PendingReceiver<mojom::V8PerFrameMemoryReporter> pending_receiver) {
    return receiver_.Bind(std::move(pending_receiver));
  }

 private:
  mojo::Receiver<mojom::V8PerFrameMemoryReporter> receiver_;
};

using MockV8PerFrameMemoryReporter =
    testing::StrictMock<LenientMockV8PerFrameMemoryReporter>;

class LenientMockMeasurementAvailableObserver
    : public V8PerFrameMemoryDecorator::Observer {
 public:
  MOCK_METHOD(void,
              OnV8MemoryMeasurementAvailable,
              (const ProcessNode* const process_node),
              (override));

  void ExpectObservationOnProcess(
      const ProcessNode* const process_node,
      uint64_t expected_unassociated_v8_bytes_used) {
    EXPECT_CALL(*this, OnV8MemoryMeasurementAvailable(process_node))
        .WillOnce([process_node, expected_unassociated_v8_bytes_used]() {
          // When the observer is notified unassociated_v8_bytes_used() should
          // immediately be available on the process node.
          EXPECT_EQ(expected_unassociated_v8_bytes_used,
                    ProcessData::ForProcessNode(process_node)
                        ->unassociated_v8_bytes_used());
        });
  }
};

using MockMeasurementAvailableObserver =
    testing::StrictMock<LenientMockMeasurementAvailableObserver>;

class LenientMockV8PerFrameMemoryObserverAnySeq
    : public V8PerFrameMemoryObserverAnySeq {
 public:
  MOCK_METHOD(void,
              OnV8MemoryMeasurementAvailable,
              (RenderProcessHostId render_process_host_id,
               const V8PerFrameMemoryDecorator::ProcessData& process_data,
               const V8PerFrameMemoryObserverAnySeq::FrameDataMap& frame_data),
              (override));
};
using MockV8PerFrameMemoryObserverAnySeq =
    testing::StrictMock<LenientMockV8PerFrameMemoryObserverAnySeq>;

class V8PerFrameMemoryDecoratorTestBase {
 public:
  static constexpr base::TimeDelta kMinTimeBetweenRequests =
      base::TimeDelta::FromSeconds(30);

  V8PerFrameMemoryDecoratorTestBase() {
    internal::SetBindV8PerFrameMemoryReporterCallbackForTesting(
        &bind_callback_);
  }

  virtual ~V8PerFrameMemoryDecoratorTestBase() {
    internal::SetBindV8PerFrameMemoryReporterCallbackForTesting(nullptr);
  }

  // Adaptor that calls GetMainThreadTaskRunner for the test harness's task
  // environment.
  virtual scoped_refptr<base::SingleThreadTaskRunner>
  GetMainThreadTaskRunner() = 0;

  void ReplyWithData(
      mojom::PerProcessV8MemoryUsageDataPtr data,
      MockV8PerFrameMemoryReporter::GetPerFrameV8MemoryUsageDataCallback
          callback) {
    std::move(callback).Run(std::move(data));
  }

  void DelayedReplyWithData(
      const base::TimeDelta& delay,
      mojom::PerProcessV8MemoryUsageDataPtr data,
      MockV8PerFrameMemoryReporter::GetPerFrameV8MemoryUsageDataCallback
          callback) {
    GetMainThreadTaskRunner()->PostDelayedTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(data)), delay);
  }

  void ExpectQuery(
      MockV8PerFrameMemoryReporter* mock_reporter,
      base::RepeatingCallback<void(
          MockV8PerFrameMemoryReporter::GetPerFrameV8MemoryUsageDataCallback
              callback)> responder) {
    EXPECT_CALL(*mock_reporter, GetPerFrameV8MemoryUsageData(_))
        .WillOnce([this, responder](
                      MockV8PerFrameMemoryReporter::
                          GetPerFrameV8MemoryUsageDataCallback callback) {
          this->last_query_time_ = base::TimeTicks::Now();
          responder.Run(std::move(callback));
        });
  }

  void ExpectQueryAndReply(MockV8PerFrameMemoryReporter* mock_reporter,
                           mojom::PerProcessV8MemoryUsageDataPtr data) {
    ExpectQuery(
        mock_reporter,
        base::BindRepeating(&V8PerFrameMemoryDecoratorTestBase::ReplyWithData,
                            base::Unretained(this), base::Passed(&data)));
  }

  void ExpectQueryAndDelayReply(MockV8PerFrameMemoryReporter* mock_reporter,
                                const base::TimeDelta& delay,
                                mojom::PerProcessV8MemoryUsageDataPtr data) {
    ExpectQuery(mock_reporter,
                base::BindRepeating(
                    &V8PerFrameMemoryDecoratorTestBase::DelayedReplyWithData,
                    base::Unretained(this), delay, base::Passed(&data)));
  }

  void ExpectBindAndRespondToQuery(
      MockV8PerFrameMemoryReporter* mock_reporter,
      mojom::PerProcessV8MemoryUsageDataPtr data,
      RenderProcessHostId expected_process_id = kTestProcessID) {
    // Wrap the move-only |data| in a callback for the expectation below.
    ExpectQueryAndReply(mock_reporter, std::move(data));

    EXPECT_CALL(*this, BindReceiverWithProxyHost(_, _))
        .WillOnce([mock_reporter, expected_process_id](
                      mojo::PendingReceiver<
                          performance_manager::mojom::V8PerFrameMemoryReporter>
                          pending_receiver,
                      RenderProcessHostProxy proxy) {
          DCHECK_EQ(expected_process_id, proxy.render_process_host_id());
          mock_reporter->Bind(std::move(pending_receiver));
        });
  }

  MOCK_METHOD(void,
              BindReceiverWithProxyHost,
              (mojo::PendingReceiver<
                   performance_manager::mojom::V8PerFrameMemoryReporter>
                   pending_receiver,
               RenderProcessHostProxy proxy),
              (const));

  // Always bind the receiver callback on the main sequence.
  internal::BindV8PerFrameMemoryReporterCallback bind_callback_ =
      base::BindLambdaForTesting(
          [this](mojo::PendingReceiver<
                     performance_manager::mojom::V8PerFrameMemoryReporter>
                     pending_receiver,
                 RenderProcessHostProxy proxy) {
            this->GetMainThreadTaskRunner()->PostTask(
                FROM_HERE, base::BindOnce(&V8PerFrameMemoryDecoratorTestBase::
                                              BindReceiverWithProxyHost,
                                          base::Unretained(this),
                                          std::move(pending_receiver), proxy));
          });

  base::TimeTicks last_query_time_;
};

void AddPerFrameIsolateMemoryUsage(FrameToken frame_token,
                                   int64_t world_id,
                                   uint64_t bytes_used,
                                   mojom::PerProcessV8MemoryUsageData* data) {
  mojom::PerFrameV8MemoryUsageData* per_frame_data = nullptr;
  for (auto& datum : data->associated_memory) {
    if (datum->frame_token == frame_token.value()) {
      per_frame_data = datum.get();
      break;
    }
  }

  if (!per_frame_data) {
    mojom::PerFrameV8MemoryUsageDataPtr datum =
        mojom::PerFrameV8MemoryUsageData::New();
    datum->frame_token = frame_token.value();
    per_frame_data = datum.get();
    data->associated_memory.push_back(std::move(datum));
  }
  ASSERT_FALSE(base::Contains(per_frame_data->associated_bytes, world_id));

  auto isolated_world_usage = mojom::V8IsolatedWorldMemoryUsage::New();
  isolated_world_usage->bytes_used = bytes_used;
  per_frame_data->associated_bytes[world_id] = std::move(isolated_world_usage);
}

}  // namespace

class V8PerFrameMemoryDecoratorTest : public GraphTestHarness,
                                      public V8PerFrameMemoryDecoratorTestBase {
 public:
  scoped_refptr<base::SingleThreadTaskRunner> GetMainThreadTaskRunner()
      override {
    return task_env().GetMainThreadTaskRunner();
  }
};

using V8PerFrameMemoryDecoratorDeathTest = V8PerFrameMemoryDecoratorTest;

class V8PerFrameMemoryRequestAnySeqTest
    : public PerformanceManagerTestHarness,
      public V8PerFrameMemoryDecoratorTestBase {
 public:
  scoped_refptr<base::SingleThreadTaskRunner> GetMainThreadTaskRunner()
      override {
    return task_environment()->GetMainThreadTaskRunner();
  }
};

constexpr base::TimeDelta
    V8PerFrameMemoryDecoratorTestBase::kMinTimeBetweenRequests;

TEST_F(V8PerFrameMemoryDecoratorTest, InstantiateOnEmptyGraph) {
  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  MockV8PerFrameMemoryReporter mock_reporter;
  auto data = mojom::PerProcessV8MemoryUsageData::New();
  data->unassociated_bytes_used = kUnassociatedBytes;
  ExpectBindAndRespondToQuery(&mock_reporter, std::move(data));

  // Create a process node and validate that it gets a request.
  auto process = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));

  // Data should not be available until the measurement is taken.
  EXPECT_FALSE(ProcessData::ForProcessNode(process.get()));

  // Run until idle to make sure the measurement isn't a hard loop.
  task_env().RunUntilIdle();

  EXPECT_TRUE(ProcessData::ForProcessNode(process.get()));
  EXPECT_EQ(
      kUnassociatedBytes,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());
}

TEST_F(V8PerFrameMemoryDecoratorTest, InstantiateOnNonEmptyGraph) {
  // Instantiate the decorator with an existing process node and validate that
  // it gets a request.
  auto process = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));

  MockV8PerFrameMemoryReporter mock_reporter;
  auto data = mojom::PerProcessV8MemoryUsageData::New();
  data->unassociated_bytes_used = kUnassociatedBytes;
  ExpectBindAndRespondToQuery(&mock_reporter, std::move(data));

  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  // Data should not be available until the measurement is taken.
  EXPECT_FALSE(ProcessData::ForProcessNode(process.get()));

  // Run until idle to make sure the measurement isn't a hard loop.
  task_env().RunUntilIdle();

  EXPECT_TRUE(ProcessData::ForProcessNode(process.get()));
  EXPECT_EQ(
      kUnassociatedBytes,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());
}

TEST_F(V8PerFrameMemoryDecoratorTest, OnlyMeasureRenderers) {
  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  for (int type = content::PROCESS_TYPE_BROWSER;
       type < content::PROCESS_TYPE_CONTENT_END; ++type) {
    if (type == content::PROCESS_TYPE_RENDERER)
      continue;

    // Instantiate a non-renderer process node and validate that it causes no
    // bind requests.
    EXPECT_CALL(*this, BindReceiverWithProxyHost(_, _)).Times(0);
    auto process = CreateNode<ProcessNodeImpl>(
        static_cast<content::ProcessType>(type),
        RenderProcessHostProxy::CreateForTesting(kTestProcessID));

    task_env().RunUntilIdle();
    testing::Mock::VerifyAndClearExpectations(this);
  }
}

TEST_F(V8PerFrameMemoryDecoratorTest, QueryRateIsLimited) {
  auto process = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));

  MockV8PerFrameMemoryReporter mock_reporter;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    // Response to request 1.
    data->unassociated_bytes_used = 1;
    ExpectBindAndRespondToQuery(&mock_reporter, std::move(data));
  }

  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  // Run until idle to make sure the measurement isn't a hard loop.
  task_env().RunUntilIdle();

  EXPECT_TRUE(ProcessData::ForProcessNode(process.get()));
  EXPECT_EQ(
      1u,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());

  // There shouldn't be an additional request this soon.
  task_env().FastForwardBy(kMinTimeBetweenRequests / 2);
  testing::Mock::VerifyAndClearExpectations(&mock_reporter);

  // Set up another request and capture the callback for later invocation.
  MockV8PerFrameMemoryReporter::GetPerFrameV8MemoryUsageDataCallback callback;
  ExpectQuery(
      &mock_reporter,
      base::BindLambdaForTesting(
          [&callback](
              MockV8PerFrameMemoryReporter::GetPerFrameV8MemoryUsageDataCallback
                  result_callback) { callback = std::move(result_callback); }));

  // Skip forward to when another request should be issued.
  task_env().FastForwardBy(kMinTimeBetweenRequests);
  ASSERT_FALSE(callback.is_null());

  // Skip forward a long while, and validate that no additional requests are
  // issued until the pending request has completed.
  task_env().FastForwardBy(10 * kMinTimeBetweenRequests);
  testing::Mock::VerifyAndClearExpectations(&mock_reporter);

  EXPECT_TRUE(ProcessData::ForProcessNode(process.get()));
  EXPECT_EQ(
      1u,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());

  // Expect another query once completing the query above.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    // Response to request 3.
    data->unassociated_bytes_used = 3;
    ExpectQueryAndReply(&mock_reporter, std::move(data));
  }

  // Reply to the request above.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    // Response to request 2.
    data->unassociated_bytes_used = 2;
    std::move(callback).Run(std::move(data));
  }

  task_env().RunUntilIdle();

  // This should have updated all the way to the third response.
  EXPECT_TRUE(ProcessData::ForProcessNode(process.get()));
  EXPECT_EQ(
      3u,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());

  // Despite the long delay to respond to request 2, there shouldn't be another
  // request until kMinTimeBetweenRequests has expired.
  task_env().FastForwardBy(kMinTimeBetweenRequests / 2);
  testing::Mock::VerifyAndClearExpectations(&mock_reporter);
}

TEST_F(V8PerFrameMemoryDecoratorTest, MultipleProcessesHaveDistinctSchedules) {
  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  // Create a process node and validate that it gets a request.
  MockV8PerFrameMemoryReporter reporter1;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 1;
    ExpectBindAndRespondToQuery(&reporter1, std::move(data));
  }

  auto process1 = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));

  task_env().FastForwardBy(kMinTimeBetweenRequests / 4);
  testing::Mock::VerifyAndClearExpectations(&reporter1);

  // Create a second process node and validate that it gets a request.
  MockV8PerFrameMemoryReporter reporter2;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 2;
    ExpectBindAndRespondToQuery(&reporter2, std::move(data));
  }

  auto process2 = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));

  task_env().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&reporter2);

  EXPECT_TRUE(ProcessData::ForProcessNode(process1.get()));
  EXPECT_EQ(1u, ProcessData::ForProcessNode(process1.get())
                    ->unassociated_v8_bytes_used());
  EXPECT_TRUE(ProcessData::ForProcessNode(process2.get()));
  EXPECT_EQ(2u, ProcessData::ForProcessNode(process2.get())
                    ->unassociated_v8_bytes_used());

  // Capture the request time from each process.
  auto capture_time_lambda =
      [](base::TimeTicks* request_time,
         MockV8PerFrameMemoryReporter::GetPerFrameV8MemoryUsageDataCallback
             callback) {
        *request_time = base::TimeTicks::Now();
        std::move(callback).Run(mojom::PerProcessV8MemoryUsageData::New());
      };

  base::TimeTicks process1_request_time;
  ExpectQuery(&reporter1,
              base::BindRepeating(capture_time_lambda,
                                  base::Unretained(&process1_request_time)));
  base::TimeTicks process2_request_time;
  ExpectQuery(&reporter2,
              base::BindRepeating(capture_time_lambda,
                                  base::Unretained(&process2_request_time)));

  task_env().FastForwardBy(kMinTimeBetweenRequests * 1.25);

  // Check that both processes got polled, and that process2 was polled after
  // process1.
  EXPECT_FALSE(process1_request_time.is_null());
  EXPECT_FALSE(process2_request_time.is_null());
  EXPECT_GT(process2_request_time, process1_request_time);
}

TEST_F(V8PerFrameMemoryDecoratorTest, PerFrameDataIsDistributed) {
  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  MockV8PerFrameMemoryReporter reporter;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    // Add data for an unknown frame.
    AddPerFrameIsolateMemoryUsage(FrameToken(base::UnguessableToken::Create()),
                                  0, 1024u, data.get());

    ExpectBindAndRespondToQuery(&reporter, std::move(data));
  }

  auto process = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));

  task_env().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&reporter);

  // Since the frame was unknown, the usage should have accrued to unassociated.
  EXPECT_TRUE(ProcessData::ForProcessNode(process.get()));
  EXPECT_EQ(
      1024u,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());

  // Create a couple of frames with specified IDs.
  auto page = CreateNode<PageNodeImpl>();

  FrameToken frame1_id = FrameToken(base::UnguessableToken::Create());
  auto frame1 = CreateNode<FrameNodeImpl>(process.get(), page.get(), nullptr, 1,
                                          2, frame1_id);

  FrameToken frame2_id = FrameToken(base::UnguessableToken::Create());
  auto frame2 = CreateNode<FrameNodeImpl>(process.get(), page.get(), nullptr, 3,
                                          4, frame2_id);
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    AddPerFrameIsolateMemoryUsage(frame1_id, 0, 1001u, data.get());
    AddPerFrameIsolateMemoryUsage(frame2_id, 0, 1002u, data.get());
    ExpectQueryAndReply(&reporter, std::move(data));
  }

  task_env().FastForwardBy(kMinTimeBetweenRequests * 1.5);
  testing::Mock::VerifyAndClearExpectations(&reporter);

  ASSERT_TRUE(FrameData::ForFrameNode(frame1.get()));
  EXPECT_EQ(1001u, FrameData::ForFrameNode(frame1.get())->v8_bytes_used());
  ASSERT_TRUE(FrameData::ForFrameNode(frame2.get()));
  EXPECT_EQ(1002u, FrameData::ForFrameNode(frame2.get())->v8_bytes_used());

  // Now verify that data is cleared for any frame that doesn't get an update,
  // plus verify that unknown frame data toes to unassociated bytes.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    AddPerFrameIsolateMemoryUsage(frame1_id, 0, 1003u, data.get());
    AddPerFrameIsolateMemoryUsage(FrameToken(base::UnguessableToken::Create()),
                                  0, 2233u, data.get());
    ExpectQueryAndReply(&reporter, std::move(data));
  }
  task_env().FastForwardBy(kMinTimeBetweenRequests);
  testing::Mock::VerifyAndClearExpectations(&reporter);

  ASSERT_TRUE(FrameData::ForFrameNode(frame1.get()));
  EXPECT_EQ(1003u, FrameData::ForFrameNode(frame1.get())->v8_bytes_used());
  EXPECT_FALSE(FrameData::ForFrameNode(frame2.get()));
  ASSERT_TRUE(ProcessData::ForProcessNode(process.get()));
  EXPECT_EQ(
      2233u,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());
}

TEST_F(V8PerFrameMemoryDecoratorTest, MeasurementRequestsSorted) {
  // Create some queries with different sample frequencies.
  constexpr base::TimeDelta kShortInterval(kMinTimeBetweenRequests);
  constexpr base::TimeDelta kMediumInterval(2 * kMinTimeBetweenRequests);
  constexpr base::TimeDelta kLongInterval(3 * kMinTimeBetweenRequests);

  // Create longer requests first to be sure they sort correctly.
  auto medium_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kMediumInterval, graph());

  auto short_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kShortInterval, graph());

  auto long_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kLongInterval, graph());

  auto* decorator = V8PerFrameMemoryDecorator::GetFromGraph(graph());
  ASSERT_TRUE(decorator);

  // A single measurement should be taken immediately regardless of the overall
  // frequency.
  MockV8PerFrameMemoryReporter mock_reporter;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 1U;
    ExpectBindAndRespondToQuery(&mock_reporter, std::move(data));
  }

  auto process = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));
  EXPECT_FALSE(ProcessData::ForProcessNode(process.get()));

  task_env().FastForwardBy(base::TimeDelta::FromSeconds(1));
  // All the following FastForwardBy calls will place the clock 1 sec after a
  // measurement is expected.

  ASSERT_TRUE(ProcessData::ForProcessNode(process.get()));
  EXPECT_EQ(
      1U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());

  // Another measurement should be taken after the shortest interval.
  EXPECT_EQ(kShortInterval, decorator->GetMinTimeBetweenRequestsPerProcess());
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 2U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kShortInterval);
    EXPECT_EQ(2U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
  }

  // Remove the shortest request. Now a measurement should be taken after the
  // medium interval, which is twice the short interval.
  short_memory_request.reset();
  EXPECT_EQ(kMediumInterval, decorator->GetMinTimeBetweenRequestsPerProcess());
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 3U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kShortInterval);
    EXPECT_EQ(2U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
    task_env().FastForwardBy(kShortInterval);
    EXPECT_EQ(3U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
  }

  // Remove the longest request. A measurement should still be taken after the
  // medium interval.
  long_memory_request.reset();
  EXPECT_EQ(kMediumInterval, decorator->GetMinTimeBetweenRequestsPerProcess());
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 4U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kMediumInterval);
    EXPECT_EQ(4U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
  }

  // Remove the medium request, making the queue empty.
  medium_memory_request.reset();
  EXPECT_TRUE(decorator->GetMinTimeBetweenRequestsPerProcess().is_zero());
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 5U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kLongInterval);
    EXPECT_EQ(4U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
  }

  // Create another request. Since this is the first request in an empty queue
  // the measurement should be taken immediately.
  long_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kLongInterval, graph());
  EXPECT_EQ(kLongInterval, decorator->GetMinTimeBetweenRequestsPerProcess());

  task_env().FastForwardBy(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(
      5U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used());

  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 6U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kLongInterval);
    EXPECT_EQ(6U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
  }

  // Now there should be kLongInterval - 1 sec until the next measurement.
  // Make sure a shorter request replaces this (the new interval should cause a
  // measurement and the old interval should not).
  medium_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kMediumInterval, graph());
  EXPECT_EQ(kMediumInterval, decorator->GetMinTimeBetweenRequestsPerProcess());

  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 7U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kMediumInterval);
    EXPECT_EQ(7U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
  }

  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 8U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    constexpr base::TimeDelta kRestOfLongInterval =
        kLongInterval - kMediumInterval;
    task_env().FastForwardBy(kRestOfLongInterval);
    EXPECT_EQ(7U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());

    task_env().FastForwardBy(kMediumInterval - kRestOfLongInterval);
    EXPECT_EQ(8U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
  }

  // Remove the medium request and add it back. The measurement interval should
  // not change.
  medium_memory_request.reset();
  EXPECT_EQ(kLongInterval, decorator->GetMinTimeBetweenRequestsPerProcess());
  medium_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kMediumInterval, graph());
  EXPECT_EQ(kMediumInterval, decorator->GetMinTimeBetweenRequestsPerProcess());

  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 9U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kMediumInterval);
    EXPECT_EQ(9U, ProcessData::ForProcessNode(process.get())
                      ->unassociated_v8_bytes_used());
  }

  // Add another long request. There should still be requests after the medium
  // interval.
  auto long_memory_request2 =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kLongInterval, graph());
  EXPECT_EQ(kMediumInterval, decorator->GetMinTimeBetweenRequestsPerProcess());

  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 10U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kMediumInterval);
    EXPECT_EQ(10U, ProcessData::ForProcessNode(process.get())
                       ->unassociated_v8_bytes_used());
  }

  // Remove the medium request. Now there are 2 requests which should cause
  // measurements at the same interval. Make sure only 1 measurement is taken.
  medium_memory_request.reset();
  EXPECT_EQ(kLongInterval, decorator->GetMinTimeBetweenRequestsPerProcess());

  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 11U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kLongInterval);
    EXPECT_EQ(11U, ProcessData::ForProcessNode(process.get())
                       ->unassociated_v8_bytes_used());
  }

  // Remove 1 of the 2 long requests. Measurements should not change.
  long_memory_request2.reset();
  EXPECT_EQ(kLongInterval, decorator->GetMinTimeBetweenRequestsPerProcess());

  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 12U;
    ExpectQueryAndReply(&mock_reporter, std::move(data));

    task_env().FastForwardBy(kLongInterval);
    EXPECT_EQ(12U, ProcessData::ForProcessNode(process.get())
                       ->unassociated_v8_bytes_used());
  }
}

TEST_F(V8PerFrameMemoryDecoratorTest, MeasurementRequestsWithDelay) {
  // Create some queries with different sample frequencies.
  constexpr base::TimeDelta kShortInterval(kMinTimeBetweenRequests);
  constexpr base::TimeDelta kMediumInterval(2 * kMinTimeBetweenRequests);
  constexpr base::TimeDelta kLongInterval(3 * kMinTimeBetweenRequests);

  // Make measurements take long enough that a second request could be sent.
  constexpr base::TimeDelta kMeasurementLength(1.5 * kShortInterval);
  constexpr base::TimeDelta kOneSecond = base::TimeDelta::FromSeconds(1);

  auto long_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kLongInterval, graph());

  auto* decorator = V8PerFrameMemoryDecorator::GetFromGraph(graph());
  ASSERT_TRUE(decorator);

  // Move past the first request since it's complicated to untangle the Bind
  // and QueryAndDelayReply expectations.
  MockV8PerFrameMemoryReporter mock_reporter;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 0U;
    ExpectBindAndRespondToQuery(&mock_reporter, std::move(data));
  }
  auto process = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));
  task_env().FastForwardBy(kOneSecond);
  // All the following FastForwardBy calls will place the clock 1 sec after a
  // measurement is expected.

  // Advance to the middle of a measurement and create a new request. Should
  // update GetMinTimeBetweenRequestsPerProcess but not start a new
  // measurement until the existing measurement finishes.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 1U;
    ExpectQueryAndDelayReply(&mock_reporter, kMeasurementLength,
                             std::move(data));
  }
  task_env().FastForwardBy(kLongInterval);
  EXPECT_EQ(last_query_time_, task_env().NowTicks() - kOneSecond)
      << "Measurement didn't start when expected";
  EXPECT_EQ(
      0U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement ended early";
  base::TimeTicks measurement_start_time = last_query_time_;

  auto medium_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kMediumInterval, graph());
  EXPECT_EQ(kMediumInterval, decorator->GetMinTimeBetweenRequestsPerProcess());
  task_env().FastForwardBy(kMeasurementLength);
  ASSERT_EQ(
      1U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement didn't end when expected";
  EXPECT_EQ(last_query_time_, measurement_start_time);

  // Next measurement should start kMediumInterval secs after the START of the
  // last measurement.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 2U;
    ExpectQueryAndDelayReply(&mock_reporter, kMeasurementLength,
                             std::move(data));
  }
  task_env().FastForwardBy(kMediumInterval - kMeasurementLength);
  EXPECT_EQ(last_query_time_, task_env().NowTicks() - kOneSecond)
      << "Measurement didn't start when expected";
  EXPECT_EQ(
      1U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement ended early";
  measurement_start_time = last_query_time_;

  task_env().FastForwardBy(kMeasurementLength);
  EXPECT_EQ(
      2U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement didn't end when expected";
  EXPECT_EQ(last_query_time_, measurement_start_time);

  // Create a request that would be sent in the middle of a measurement. It
  // should start immediately after the measurement finishes.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 3U;
    ExpectQueryAndDelayReply(&mock_reporter, kMeasurementLength,
                             std::move(data));
  }
  task_env().FastForwardBy(kMediumInterval - kMeasurementLength);
  EXPECT_EQ(last_query_time_, task_env().NowTicks() - kOneSecond)
      << "Measurement didn't start when expected";
  EXPECT_EQ(
      2U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement ended early";
  measurement_start_time = last_query_time_;

  auto short_memory_request =
      std::make_unique<V8PerFrameMemoryDecorator::MeasurementRequest>(
          kShortInterval, graph());
  EXPECT_EQ(kShortInterval, decorator->GetMinTimeBetweenRequestsPerProcess());
  EXPECT_EQ(last_query_time_, measurement_start_time);

  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 4U;
    ExpectQueryAndDelayReply(&mock_reporter, kMeasurementLength,
                             std::move(data));
  }
  task_env().FastForwardBy(kMeasurementLength);
  EXPECT_EQ(last_query_time_, task_env().NowTicks() - kOneSecond)
      << "Measurement didn't start when expected";
  EXPECT_EQ(
      3U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement ended early";
  measurement_start_time = last_query_time_;

  // Delete the short request. Should update
  // GetMinTimeBetweenRequestsPerProcess but not start a new measurement
  // until the existing measurement finishes.
  short_memory_request.reset();
  EXPECT_EQ(kMediumInterval, decorator->GetMinTimeBetweenRequestsPerProcess());
  task_env().FastForwardBy(kMeasurementLength);
  EXPECT_EQ(
      4U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement didn't end when expected";
  EXPECT_EQ(last_query_time_, measurement_start_time);

  // Delete the last request while a measurement is in process. The
  // measurement should finish successfully but no more should be sent.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 5U;
    ExpectQueryAndDelayReply(&mock_reporter, kMeasurementLength,
                             std::move(data));
  }
  task_env().FastForwardBy(kMediumInterval - kMeasurementLength);
  EXPECT_EQ(last_query_time_, task_env().NowTicks() - kOneSecond)
      << "Measurement didn't start when expected";
  EXPECT_EQ(
      4U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement ended early";
  measurement_start_time = last_query_time_;

  medium_memory_request.reset();
  long_memory_request.reset();
  EXPECT_TRUE(decorator->GetMinTimeBetweenRequestsPerProcess().is_zero());
  task_env().FastForwardBy(kMeasurementLength);
  EXPECT_EQ(
      5U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "Measurement didn't end when expected";
  EXPECT_EQ(last_query_time_, measurement_start_time);

  // No more requests should be sent.
  testing::Mock::VerifyAndClearExpectations(this);
  task_env().FastForwardBy(kLongInterval);
}

TEST_F(V8PerFrameMemoryDecoratorTest, MeasurementRequestOutlivesDecorator) {
  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  auto* decorator = V8PerFrameMemoryDecorator::GetFromGraph(graph());
  ASSERT_TRUE(decorator);

  MockV8PerFrameMemoryReporter mock_reporter;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 1U;
    ExpectBindAndRespondToQuery(&mock_reporter, std::move(data));
  }
  auto process = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));
  task_env().FastForwardBy(base::TimeDelta::FromSeconds(1));
  ASSERT_EQ(
      1U,
      ProcessData::ForProcessNode(process.get())->unassociated_v8_bytes_used())
      << "First measurement didn't happen when expected";

  graph()->TakeFromGraph(decorator);

  // No request should be sent, and the decorator destructor should not DCHECK.
  testing::Mock::VerifyAndClearExpectations(this);
  task_env().FastForwardBy(kMinTimeBetweenRequests);
}

TEST_F(V8PerFrameMemoryDecoratorTest, NotifyObservers) {
  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  auto* decorator = V8PerFrameMemoryDecorator::GetFromGraph(graph());
  ASSERT_TRUE(decorator);

  MockMeasurementAvailableObserver observer1;
  MockMeasurementAvailableObserver observer2;
  decorator->AddObserver(&observer1);
  decorator->AddObserver(&observer2);

  // Create a process node and validate that all observers are notified when a
  // measurement is available for it.
  MockV8PerFrameMemoryReporter reporter1;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 1U;
    ExpectBindAndRespondToQuery(&reporter1, std::move(data));
  }

  auto process1 = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));

  observer1.ExpectObservationOnProcess(process1.get(), 1U);
  observer2.ExpectObservationOnProcess(process1.get(), 1U);

  task_env().FastForwardBy(kMinTimeBetweenRequests / 2);
  testing::Mock::VerifyAndClearExpectations(&reporter1);
  testing::Mock::VerifyAndClearExpectations(&observer1);
  testing::Mock::VerifyAndClearExpectations(&observer2);

  // Create a process node and validate that all observers are notified when
  // any measurement is available. After fast-forwarding the first measurement
  // for process2 and the second measurement for process1 will arrive.
  MockV8PerFrameMemoryReporter reporter2;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 2U;
    ExpectBindAndRespondToQuery(&reporter2, std::move(data));
  }
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 3U;
    ExpectQueryAndReply(&reporter1, std::move(data));
  }

  auto process2 = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));

  observer1.ExpectObservationOnProcess(process2.get(), 2U);
  observer2.ExpectObservationOnProcess(process2.get(), 2U);
  observer1.ExpectObservationOnProcess(process1.get(), 3U);
  observer2.ExpectObservationOnProcess(process1.get(), 3U);

  task_env().FastForwardBy(kMinTimeBetweenRequests / 2);
  testing::Mock::VerifyAndClearExpectations(&reporter1);
  testing::Mock::VerifyAndClearExpectations(&reporter2);
  testing::Mock::VerifyAndClearExpectations(&observer1);
  testing::Mock::VerifyAndClearExpectations(&observer2);

  // Remove an observer and make sure the other is still notified after the
  // next measurement.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 4U;
    ExpectQueryAndReply(&reporter1, std::move(data));
  }
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 5U;
    ExpectQueryAndReply(&reporter2, std::move(data));
  }

  decorator->RemoveObserver(&observer1);

  observer2.ExpectObservationOnProcess(process1.get(), 4U);
  observer2.ExpectObservationOnProcess(process2.get(), 5U);

  task_env().FastForwardBy(kMinTimeBetweenRequests);
  testing::Mock::VerifyAndClearExpectations(&reporter1);
  testing::Mock::VerifyAndClearExpectations(&reporter2);
  testing::Mock::VerifyAndClearExpectations(&observer1);
  testing::Mock::VerifyAndClearExpectations(&observer2);
}

TEST_F(V8PerFrameMemoryDecoratorTest, ObserverOutlivesDecorator) {
  V8PerFrameMemoryDecorator::MeasurementRequest memory_request(
      kMinTimeBetweenRequests, graph());

  auto* decorator = V8PerFrameMemoryDecorator::GetFromGraph(graph());
  ASSERT_TRUE(decorator);

  MockMeasurementAvailableObserver observer;
  decorator->AddObserver(&observer);

  // Create a process node and move past the initial request to it.
  MockV8PerFrameMemoryReporter reporter;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 1U;
    ExpectBindAndRespondToQuery(&reporter, std::move(data));
  }

  auto process = CreateNode<ProcessNodeImpl>(
      content::PROCESS_TYPE_RENDERER,
      RenderProcessHostProxy::CreateForTesting(kTestProcessID));
  observer.ExpectObservationOnProcess(process.get(), 1U);

  task_env().FastForwardBy(base::TimeDelta::FromSeconds(1));

  testing::Mock::VerifyAndClearExpectations(&reporter);
  testing::Mock::VerifyAndClearExpectations(&observer);

  // Start the next measurement.
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = 2U;
    ExpectQueryAndDelayReply(&reporter, kMinTimeBetweenRequests,
                             std::move(data));
  }
  task_env().FastForwardBy(kMinTimeBetweenRequests);

  // Destroy the decorator before the measurement completes. The observer
  // should not be notified.
  graph()->TakeFromGraph(decorator);
  task_env().FastForwardBy(kMinTimeBetweenRequests);
}

TEST_F(V8PerFrameMemoryDecoratorDeathTest,
       MeasurementRequestMultipleStartMeasurement) {
  EXPECT_DCHECK_DEATH({
    V8PerFrameMemoryDecorator::MeasurementRequest request(
        kMinTimeBetweenRequests);
    request.StartMeasurement(graph());
    request.StartMeasurement(graph());
  });

  EXPECT_DCHECK_DEATH({
    V8PerFrameMemoryDecorator::MeasurementRequest request(
        kMinTimeBetweenRequests, graph());
    request.StartMeasurement(graph());
  });
}

TEST_F(V8PerFrameMemoryRequestAnySeqTest, RequestIsSequenceSafe) {
  // Precondition: CallOnGraph must run on a different sequence.  Note that all
  // tasks passed to CallOnGraph will only run when run_loop.Run() is called
  // below.
  ASSERT_TRUE(GetMainThreadTaskRunner()->RunsTasksInCurrentSequence());
  PerformanceManager::CallOnGraph(
      FROM_HERE, base::BindLambdaForTesting([this] {
        EXPECT_FALSE(
            this->GetMainThreadTaskRunner()->RunsTasksInCurrentSequence());
      }));

  // Set the active contents and simulate a navigation, which adds nodes to the
  // graph.
  SetContents(CreateTestWebContents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://www.foo.com/"));

  // Create some test data to return for a measurement request.
  constexpr uint64_t kAssociatedBytes = 0x123;
  content::RenderFrameHost* main_frame = web_contents()->GetMainFrame();
  ASSERT_NE(nullptr, main_frame);
  const RenderProcessHostId process_id(main_frame->GetProcess()->GetID());
  const FrameToken frame_token(main_frame->GetFrameToken());
  const content::GlobalFrameRoutingId frame_id(process_id.value(),
                                               main_frame->GetRoutingID());

  V8PerFrameMemoryDecorator::ProcessData expected_process_data;
  expected_process_data.set_unassociated_v8_bytes_used(kUnassociatedBytes);
  V8PerFrameMemoryObserverAnySeq::FrameDataMap expected_frame_data;
  expected_frame_data[frame_id].set_v8_bytes_used(kAssociatedBytes);

  MockV8PerFrameMemoryReporter reporter;
  {
    auto data = mojom::PerProcessV8MemoryUsageData::New();
    data->unassociated_bytes_used = kUnassociatedBytes;
    AddPerFrameIsolateMemoryUsage(frame_token, 0, kAssociatedBytes, data.get());
    ExpectBindAndRespondToQuery(&reporter, std::move(data), process_id);
  }

  // Decorator should not exist before creating a request.
  PerformanceManager::CallOnGraph(
      FROM_HERE, base::BindOnce([](Graph* graph) {
        EXPECT_FALSE(V8PerFrameMemoryDecorator::GetFromGraph(graph));
      }));

  // This object is created on the main sequence but should cause a
  // MeasurementRequest to be created on the graph sequence after the above
  // task.
  auto request = std::make_unique<V8PerFrameMemoryRequestAnySeq>(
      V8PerFrameMemoryDecoratorTest::kMinTimeBetweenRequests);
  MockV8PerFrameMemoryObserverAnySeq observer;
  request->AddObserver(&observer);

  // Decorator now exists and has the request frequency set, proving that the
  // MeasurementRequest was created.
  PerformanceManager::CallOnGraph(
      FROM_HERE, base::BindOnce([](Graph* graph) {
        auto* decorator = V8PerFrameMemoryDecorator::GetFromGraph(graph);
        ASSERT_TRUE(decorator);
        EXPECT_EQ(V8PerFrameMemoryDecoratorTest::kMinTimeBetweenRequests,
                  decorator->GetMinTimeBetweenRequestsPerProcess());
      }));

  // The observer should be invoked on the main sequence when a measurement is
  // available. Exit the RunLoop when this happens.
  base::RunLoop run_loop;
  EXPECT_CALL(observer,
              OnV8MemoryMeasurementAvailable(process_id, expected_process_data,
                                             expected_frame_data))
      .WillOnce([this, &run_loop, &process_id, &expected_frame_data]() {
        run_loop.Quit();
        ASSERT_TRUE(
            this->GetMainThreadTaskRunner()->RunsTasksInCurrentSequence())
            << "Observer invoked on wrong sequence";
        // Verify that the notification parameters can be used to retrieve a
        // RenderFrameHost and RenderProcessHost. This is safe on the main
        // thread.
        EXPECT_NE(nullptr,
                  content::RenderProcessHost::FromID(process_id.value()));
        const content::GlobalFrameRoutingId frame_id =
            expected_frame_data.cbegin()->first;
        EXPECT_NE(nullptr, content::RenderFrameHost::FromID(frame_id));
      });

  // Now execute all the above tasks.
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(this);
  testing::Mock::VerifyAndClearExpectations(&reporter);
  testing::Mock::VerifyAndClearExpectations(&observer);

  // Destroying the object on the main sequence should cause the wrapped
  // MeasurementRequest to be destroyed on the graph sequence after any
  // scheduled tasks, which resets the request frequency to zero.
  PerformanceManager::CallOnGraph(
      FROM_HERE, base::BindOnce([](Graph* graph) {
        EXPECT_EQ(V8PerFrameMemoryDecoratorTest::kMinTimeBetweenRequests,
                  V8PerFrameMemoryDecorator::GetFromGraph(graph)
                      ->GetMinTimeBetweenRequestsPerProcess());
      }));

  // Must remove the observer before destroying the request to avoid a DCHECK
  // from ObserverList.
  request->RemoveObserver(&observer);
  request.reset();

  PerformanceManager::CallOnGraph(
      FROM_HERE, base::BindOnce([](Graph* graph) {
        EXPECT_TRUE(V8PerFrameMemoryDecorator::GetFromGraph(graph)
                        ->GetMinTimeBetweenRequestsPerProcess()
                        .is_zero());
      }));

  // Execute the above tasks and exit.
  base::RunLoop run_loop2;
  PerformanceManager::CallOnGraph(FROM_HERE, run_loop2.QuitClosure());
  run_loop2.Run();
}

}  // namespace performance_manager
