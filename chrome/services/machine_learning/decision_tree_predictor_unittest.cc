// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/machine_learning/decision_tree_predictor.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "base/test/task_environment.h"
#include "chrome/services/machine_learning/public/cpp/decision_tree_model.h"
#include "chrome/services/machine_learning/public/cpp/test_support/machine_learning_test_utils.h"
#include "chrome/services/machine_learning/public/mojom/decision_tree.mojom.h"
#include "components/optimization_guide/proto/models.pb.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace machine_learning {

class DecisionTreePredictorTest : public ::testing::Test {
 public:
  DecisionTreePredictorTest() = default;
  ~DecisionTreePredictorTest() override = default;
};

TEST_F(DecisionTreePredictorTest, InstantiateInvalidPredictor) {
  DecisionTreePredictor predictor(nullptr);
  EXPECT_FALSE(predictor.IsValid());
}

TEST_F(DecisionTreePredictorTest, InstantiateValidPredictor) {
  auto model = std::make_unique<DecisionTreeModel>(
      testing::GetModelProtoForPredictionResult(
          mojom::DecisionTreePredictionResult::kTrue));
  DecisionTreePredictor predictor(std::move(model));

  EXPECT_TRUE(predictor.IsValid());
}

TEST_F(DecisionTreePredictorTest, ModelPrediction) {
  mojom::DecisionTreePredictionResult g_result;
  double g_score;
  auto callback = base::BindLambdaForTesting(
      [&g_result, &g_score](mojom::DecisionTreePredictionResult result,
                            double score) -> void {
        g_result = result;
        g_score = score;
      });

  auto model = std::make_unique<DecisionTreeModel>(
      testing::GetModelProtoForPredictionResult(
          mojom::DecisionTreePredictionResult::kTrue));
  std::unique_ptr<mojom::DecisionTreePredictor> predictor =
      std::make_unique<DecisionTreePredictor>(std::move(model));

  predictor->Predict({}, std::move(callback));

  EXPECT_EQ(mojom::DecisionTreePredictionResult::kTrue, g_result);
  EXPECT_GT(g_score, testing::kModelThreshold);
}

}  // namespace machine_learning
