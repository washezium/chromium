// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/metrics_util.h"

#include "base/bind.h"
#include "base/check.h"
#include "base/no_destructor.h"

namespace ash {
namespace metrics_util {

namespace {

bool g_data_collection_enabled = false;

std::vector<cc::FrameSequenceMetrics::ThroughputData>& GetDataCollector() {
  static base::NoDestructor<
      std::vector<cc::FrameSequenceMetrics::ThroughputData>>
      data;
  return *data;
}

void CollectDataAndForwardReport(
    ReportCallback callback,
    const cc::FrameSequenceMetrics::ThroughputData throughput) {
  // An arbitrary cap on the maximum number of animations being collected.
  DCHECK_LT(GetDataCollector().size(), 1000u);

  GetDataCollector().push_back(throughput);
  std::move(callback).Run(throughput);
}

// Calculates smoothness from |throughput| and sends to |callback|.
void ForwardSmoothness(SmoothnessCallback callback,
                       cc::FrameSequenceMetrics::ThroughputData throughput) {
  const int smoothness = std::floor(100.0f * throughput.frames_produced /
                                    throughput.frames_expected);
  callback.Run(smoothness);
}

}  // namespace

ReportCallback ForSmoothness(SmoothnessCallback callback,
                             bool exclude_from_data_collection) {
  auto forward_smoothness =
      base::BindRepeating(&ForwardSmoothness, std::move(callback));
  if (!g_data_collection_enabled || exclude_from_data_collection)
    return forward_smoothness;

  return base::BindRepeating(&CollectDataAndForwardReport,
                             std::move(forward_smoothness));
}

void StartDataCollection() {
  DCHECK(!g_data_collection_enabled);
  g_data_collection_enabled = true;
}

std::vector<cc::FrameSequenceMetrics::ThroughputData> StopDataCollection() {
  DCHECK(g_data_collection_enabled);
  g_data_collection_enabled = false;

  std::vector<cc::FrameSequenceMetrics::ThroughputData> data;
  data.swap(GetDataCollector());
  return data;
}

}  // namespace metrics_util
}  // namespace ash
