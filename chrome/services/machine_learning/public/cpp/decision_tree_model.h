// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_MACHINE_LEARNING_PUBLIC_CPP_DECISION_TREE_MODEL_H_
#define CHROME_SERVICES_MACHINE_LEARNING_PUBLIC_CPP_DECISION_TREE_MODEL_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "chrome/services/machine_learning/public/mojom/decision_tree.mojom.h"
#include "components/optimization_guide/prediction_model.h"
#include "components/optimization_guide/proto/models.pb.h"

namespace machine_learning {

// Wrapper around a DecisionTree proto for validation and evaluation.
// Actual work is done by |decision_tree::PredictionModel|.
// Made public for testing and in-process validation and evaluations.
class DecisionTreeModel {
 public:
  // Creates a |DecisionTreeModel| from a
  // |optimization_guide::proto::PredictionModel|.
  explicit DecisionTreeModel(
      std::unique_ptr<optimization_guide::proto::PredictionModel> model_proto);

  ~DecisionTreeModel();

  DecisionTreeModel(const DecisionTreeModel&) = delete;
  DecisionTreeModel& operator=(const DecisionTreeModel&) = delete;

  // Deserializes, validates, and creates a Decision Tree model from a model
  // spec. This will consume the |mojom::DecisionTreeModelSpec| held in |spec|.
  // Returns a |nullptr| if the deserialization or validation steps fails.
  static std::unique_ptr<DecisionTreeModel> FromModelSpec(
      mojom::DecisionTreeModelSpecPtr spec);

  // Serializes the Decision Tree model proto into a model spec. Returns a
  // |nullptr| if the serialization fails, or if the model is invalid.
  mojom::DecisionTreeModelSpecPtr ToModelSpec() const;

  // Returns the DecisionTreePredictionResult by evaluating |prediction_model_|
  // using the provided |model_features|. |prediction_score| will be populated
  // with the score output by the model.
  mojom::DecisionTreePredictionResult Predict(
      const base::flat_map<std::string, float>& model_features,
      double* prediction_score);

  // Whether this class is holding a valid model.
  bool IsValid() const;

 private:
  // PredictionModel held by the Service.
  std::unique_ptr<optimization_guide::PredictionModel> prediction_model_;

  // Handle to the underlying proto in |prediction_model_|.
  optimization_guide::proto::PredictionModel* model_proto_;
};

}  // namespace machine_learning

#endif  // CHROME_SERVICES_MACHINE_LEARNING_PUBLIC_CPP_DECISION_TREE_MODEL_H_
