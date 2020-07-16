// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/machine_learning/public/cpp/decision_tree_model.h"

#include <memory>
#include <string>
#include <utility>

#include "chrome/services/machine_learning/public/mojom/decision_tree.mojom-shared.h"
#include "chrome/services/machine_learning/public/mojom/decision_tree.mojom.h"
#include "components/optimization_guide/optimization_guide_enums.h"
#include "components/optimization_guide/prediction_model.h"
#include "components/optimization_guide/proto/models.pb.h"

namespace machine_learning {

DecisionTreeModel::DecisionTreeModel(
    std::unique_ptr<optimization_guide::proto::PredictionModel> model_proto)
    : prediction_model_(model_proto
                            ? optimization_guide::PredictionModel::Create(
                                  std::move(model_proto))
                            : nullptr),
      model_proto_(model_proto ? model_proto.get() : nullptr) {}

DecisionTreeModel::~DecisionTreeModel() = default;

// static
std::unique_ptr<DecisionTreeModel> DecisionTreeModel::FromModelSpec(
    mojom::DecisionTreeModelSpecPtr spec) {
  // TODO(crbug/1102428): Implement this.
  return nullptr;
}

mojom::DecisionTreeModelSpecPtr DecisionTreeModel::ToModelSpec() const {
  // TODO(crbug/1102428): Implement this.
  return nullptr;
}

mojom::DecisionTreePredictionResult DecisionTreeModel::Predict(
    const base::flat_map<std::string, float>& model_features,
    double* prediction_score) {
  if (!IsValid())
    return mojom::DecisionTreePredictionResult::kUnknown;
  double score;
  optimization_guide::OptimizationTargetDecision target_decision =
      prediction_model_->Predict(model_features, &score);

  if (prediction_score)
    *prediction_score = score;

  switch (target_decision) {
    case optimization_guide::OptimizationTargetDecision::kPageLoadMatches:
      return mojom::DecisionTreePredictionResult::kTrue;
    case optimization_guide::OptimizationTargetDecision::kPageLoadDoesNotMatch:
      return mojom::DecisionTreePredictionResult::kFalse;
    default:
      return mojom::DecisionTreePredictionResult::kUnknown;
  }
}

bool DecisionTreeModel::IsValid() const {
  return prediction_model_ != nullptr;
}

}  // namespace machine_learning
