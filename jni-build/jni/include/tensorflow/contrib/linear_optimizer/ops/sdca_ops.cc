/* Copyright 2016 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/core/framework/op.h"
namespace tensorflow {

// --------------------------------------------------------------------------

REGISTER_OP("DistributedSdcaLargeBatchSolver")
    .Attr("loss_type: {'logistic_loss', 'squared_loss', 'hinge_loss'}")
    .Attr("num_sparse_features: int >= 0")
    .Attr("num_sparse_features_with_values: int >= 0")
    .Attr("num_dense_features: int >= 0")
    .Attr("l1: float")
    .Attr("l2: float")
    .Attr("num_partitions: int >= 1")
    .Attr("num_inner_iterations: int >= 1")
    .Input("sparse_example_indices: num_sparse_features * int64")
    .Input("sparse_feature_indices: num_sparse_features * int64")
    .Input("sparse_feature_values: num_sparse_features_with_values * float")
    .Input("dense_features: num_dense_features * float")
    .Input("example_weights: float")
    .Input("example_labels: float")
    .Input("sparse_weights: num_sparse_features * float")
    .Input("dense_weights: num_dense_features * float")
    .Input("example_state_data: float")
    .Output("out_example_state_data: float")
    .Output("out_delta_sparse_weights: num_sparse_features * float")
    .Output("out_delta_dense_weights: num_dense_features * float")
    .Doc(R"doc(
Distributed version of Stochastic Dual Coordinate Ascent (SDCA) optimizer for
linear models with L1 + L2 regularization. As global optimization objective is
strongly-convex, the optimizer optimizes the dual objective at each step. The
optimizer applies each update one example at a time. Examples are sampled
uniformly, and the optimizer is learning rate free and enjoys linear convergence
rate.

Proximal Stochastic Dual Coordinate Ascent, Shalev-Shwartz, Shai; Zhang, Tong.
2012 arXiv1211.2717S: http://arxiv.org/pdf/1211.2717v1.pdf

  Loss objective = \sum f_{i}(wx_{i}) + (l2 / 2) * |w|^2 + l1 * |w|

Adding vs. Averaging in Distributed Primal-Dual Optimization.
Chenxin Ma, Virginia Smith, Martin Jaggi, Michael I. Jordan, Peter Richtarik,
Martin Takac http://arxiv.org/abs/1502.03508

loss_type: Type of the primal loss. Currently SdcaSolver supports logistic,
  squared and hinge losses.
num_sparse_features: Number of sparse feature groups to train on.
num_sparse_features_with_values: Number of sparse feature groups with values
  associated with it, otherwise implicitly treats values as 1.0.
num_dense_features: Number of dense feature groups to train on.
l1: Symmetric l1 regularization strength.
l2: Symmetric l2 regularization strength.
num_partitions: Number of partitions of the loss function.
num_inner_iterations: Number of iterations per mini-batch.
sparse_example_indices: a list of vectors which contain example indices.
sparse_feature_indices: a list of vectors which contain feature indices.
sparse_feature_values: a list of vectors which contains feature value
  associated with each feature group.
dense_features: a list of matrices which contains the dense feature values.
example_weights: a vector which contains the weight associated with each
  example.
example_labels: a vector which contains the label/target associated with each
  example.
sparse_weights: a list of vectors where each value is the weight associated with
  a sparse feature group.
dense_weights: a list of vectors where the values are the weights associated
 with a dense feature group.
example_state_data: a list of vectors containing the example state data.
out_example_state_data: a list of vectors containing the updated example state
  data.
out_delta_sparse_weights: a list of vectors where each value is the delta
  weights associated with a sparse feature group.
out_delta_dense_weights: a list of vectors where the values are the delta
  weights associated with a dense feature group.
)doc");

REGISTER_OP("SdcaShrinkL1")
    .Attr("num_sparse_features: int >= 0")
    .Attr("num_dense_features: int >= 0")
    .Attr("l1: float")
    .Attr("l2: float")
    .Input("sparse_weights: Ref(num_sparse_features * float)")
    .Input("dense_weights: Ref(num_dense_features * float)")
    .Doc(R"doc(
Applies L1 regularization shrink step on the parameters.

num_sparse_features: Number of sparse feature groups to train on.
num_dense_features: Number of dense feature groups to train on.
l1: Symmetric l1 regularization strength.
l2: Symmetric l2 regularization strength. Should be a positive float.
sparse_weights: a list of vectors where each value is the weight associated with
  a sparse feature group.
dense_weights: a list of vectors where the value is the weight associated with
  a dense feature group.
)doc");

REGISTER_OP("SdcaFprint")
    .Input("input: string")
    .Output("output: string")
    .Doc(R"doc(
Computes fingerprints of the input strings.

input: vector of strings to compute fingerprints on.
output: vector containing the computed fingerprints.
)doc");

}  // namespace tensorflow
