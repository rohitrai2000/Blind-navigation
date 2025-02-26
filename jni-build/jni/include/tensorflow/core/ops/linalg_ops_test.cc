/* Copyright 2016 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");

You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/core/framework/node_def_builder.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference_testutil.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {

TEST(LinalgOpsTest, MatrixDeterminant_ShapeFn) {
  ShapeInferenceTestOp op("MatrixDeterminant");
  INFER_OK(op, "?", "[]");
  INFER_OK(op, "[?,?]", "[]");
  INFER_OK(op, "[1,?]", "[]");
  INFER_OK(op, "[?,1]", "[]");
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[1,2]");
}

TEST(LinalgOpsTest, BatchMatrixDeterminant_ShapeFn) {
  ShapeInferenceTestOp op("BatchMatrixDeterminant");
  INFER_OK(op, "?", "?");
  INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Dimensions must be equal, but are 2 and 1", op, "[1,?,3,4,1,2]");

  INFER_OK(op, "[?,?]", "[]");
  INFER_OK(op, "[1,?]", "[]");
  INFER_OK(op, "[?,1]", "[]");

  // Repeat previous block of tests with input rank > 2.
  INFER_OK(op, "[1,?,3,4,?,?]", "[d0_0,d0_1,d0_2,d0_3]");
  INFER_OK(op, "[1,?,3,4,1,?]", "[d0_0,d0_1,d0_2,d0_3]");
  INFER_OK(op, "[1,?,3,4,?,1]", "[d0_0,d0_1,d0_2,d0_3]");
}

TEST(LinalgOpsTest, UnchangedSquare_ShapeFn) {
  for (const char* op_name : {"Cholesky", "CholeskyGrad", "MatrixInverse"}) {
    ShapeInferenceTestOp op(op_name);

    const string extra_shape = (op.name == "CholeskyGrad" ? ";?" : "");

    INFER_OK(op, "?" + extra_shape, "[?,?]");
    INFER_OK(op, "[?,?]" + extra_shape, "[d0_0|d0_1,d0_0|d0_1]");
    INFER_OK(op, "[1,?]" + extra_shape, "[d0_0,d0_0]");
    INFER_OK(op, "[?,1]" + extra_shape, "[d0_1,d0_1]");
    INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1]" + extra_shape);
    INFER_ERROR("Dimensions must be equal, but are 1 and 2", op,
                "[1,2]" + extra_shape);
  }
}

TEST(LinalgOpsTest, BatchUnchangedSquare_ShapeFn) {
  for (const char* op_name :
       {"BatchCholesky", "BatchCholeskyGrad", "BatchMatrixInverse"}) {
    ShapeInferenceTestOp op(op_name);

    const string extra_shape = (op.name == "BatchCholeskyGrad" ? ";?" : "");

    INFER_OK(op, "?" + extra_shape, "?");
    INFER_ERROR("Shape must be at least rank 2 but is rank 1", op,
                "[1]" + extra_shape);
    INFER_ERROR("Dimensions must be equal, but are 1 and 2", op,
                "[1,2]" + extra_shape);

    INFER_OK(op, "[?,?]" + extra_shape, "[d0_0|d0_1,d0_0|d0_1]");
    INFER_OK(op, "[1,?]" + extra_shape, "[d0_0,d0_0]");
    INFER_OK(op, "[?,1]" + extra_shape, "[d0_1,d0_1]");

    // Repeat previous block of tests with input rank > 2.
    INFER_OK(op, "[5,?,7,?,?]" + extra_shape,
             "[d0_0,d0_1,d0_2,d0_3|d0_4,d0_3|d0_4]");
    INFER_OK(op, "[5,?,7,1,?]" + extra_shape, "[d0_0,d0_1,d0_2,d0_3,d0_3]");
    INFER_OK(op, "[5,?,7,?,1]" + extra_shape, "[d0_0,d0_1,d0_2,d0_4,d0_4]");
  }
}

TEST(LinalgOpsTest, SelfAdjointEig_ShapeFn) {
  ShapeInferenceTestOp op("SelfAdjointEig");
  INFER_OK(op, "?", "[?,?]");
  INFER_OK(op, "[?,?]", "[?,d0_0|d0_1]");
  INFER_OK(op, "[1,?]", "[2,d0_0]");
  INFER_OK(op, "[?,1]", "[2,d0_1]");
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[1,2]");
}

TEST(LinalgOpsTest, BatchSelfAdjointEig_ShapeFn) {
  ShapeInferenceTestOp op("BatchSelfAdjointEig");
  INFER_OK(op, "?", "?");
  INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[1,2]");

  INFER_OK(op, "[?,?]", "[?,d0_0|d0_1]");
  INFER_OK(op, "[1,?]", "[2,d0_0]");
  INFER_OK(op, "[?,1]", "[2,d0_1]");

  // Repeat previous block of tests with input rank > 2.
  INFER_OK(op, "[5,?,7,?,?]", "[d0_0,d0_1,d0_2,?,d0_3|d0_4]");
  INFER_OK(op, "[5,?,7,1,?]", "[d0_0,d0_1,d0_2,2,d0_3]");
  INFER_OK(op, "[5,?,7,?,1]", "[d0_0,d0_1,d0_2,2,d0_4]");
}

TEST(LinalgOpsTest, SelfAdjointEigV2_ShapeFn) {
  ShapeInferenceTestOp op("SelfAdjointEigV2");
  auto set_compute_v = [&op](bool compute_v) {
    TF_CHECK_OK(NodeDefBuilder("test", "Pack")
                    .Input({{"input", 0, DT_FLOAT}})
                    .Attr("compute_v", compute_v)
                    .Finalize(&op.node_def));
  };
  set_compute_v(false);
  INFER_OK(op, "?", "[?];[0]");
  INFER_OK(op, "[?,?]", "[d0_0|d0_1];[0]");
  INFER_OK(op, "[1,?]", "[d0_0|d0_1];[0]");
  INFER_OK(op, "[?,1]", "[d0_0|d0_1];[0]");
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[1,2]");

  set_compute_v(true);
  INFER_OK(op, "?", "[?];[?,?]");
  INFER_OK(op, "[?,?]", "[d0_0|d0_1];[d0_0|d0_1,d0_0|d0_1]");
  INFER_OK(op, "[1,?]", "[d0_0|d0_1];[d0_0|d0_1,d0_0|d0_1]");
  INFER_OK(op, "[?,1]", "[d0_0|d0_1];[d0_0|d0_1,d0_0|d0_1]");
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[1,2]");
}

TEST(LinalgOpsTest, BatchSelfAdjointEigV2_ShapeFn) {
  ShapeInferenceTestOp op("BatchSelfAdjointEigV2");
  auto set_compute_v = [&op](bool compute_v) {
    TF_CHECK_OK(NodeDefBuilder("test", "Pack")
                    .Input({{"input", 0, DT_FLOAT}})
                    .Attr("compute_v", compute_v)
                    .Finalize(&op.node_def));
  };

  set_compute_v(false);
  INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[1,2]");
  INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[3,1,2]");

  INFER_OK(op, "?", "?;[0]");
  INFER_OK(op, "[?,?]", "[d0_0|d0_1];[0]");
  INFER_OK(op, "[1,?]", "[d0_0|d0_1];[0]");
  INFER_OK(op, "[?,1]", "[d0_0|d0_1];[0]");

  // Repeat previous block of tests with input rank > 2.
  INFER_OK(op, "[5,?,7,?,?]", "[d0_0,d0_1,d0_2,d0_3|d0_4];[0]");
  INFER_OK(op, "[5,?,7,1,?]", "[d0_0,d0_1,d0_2,d0_3|d0_4];[0]");
  INFER_OK(op, "[5,?,7,?,1]", "[d0_0,d0_1,d0_2,d0_3|d0_4];[0]");

  set_compute_v(true);
  INFER_OK(op, "?", "?;?");
  INFER_OK(op, "[?,?]", "[d0_0|d0_1];[d0_0|d0_1,d0_0|d0_1]");
  INFER_OK(op, "[1,?]", "[d0_0|d0_1];[d0_0|d0_1,d0_0|d0_1]");
  INFER_OK(op, "[?,1]", "[d0_0|d0_1];[d0_0|d0_1,d0_0|d0_1]");

  // Repeat previous block of tests with input rank > 2.
  INFER_OK(op, "[5,?,7,?,?]",
           "[d0_0,d0_1,d0_2,d0_3|d0_4];[d0_0,d0_1,d0_2,d0_3|d0_4,d0_3|d0_4]");
  INFER_OK(op, "[5,?,7,1,?]",
           "[d0_0,d0_1,d0_2,d0_3|d0_4];[d0_0,d0_1,d0_2,d0_3|d0_4,d0_3|d0_4]");
  INFER_OK(op, "[5,?,7,?,1]",
           "[d0_0,d0_1,d0_2,d0_3|d0_4];[d0_0,d0_1,d0_2,d0_3|d0_4,d0_3|d0_4]");
}

TEST(LinalgOpsTest, SquareMatrixSolve_ShapeFn) {
  for (const char* op_name : {"MatrixSolve", "MatrixTriangularSolve"}) {
    ShapeInferenceTestOp op(op_name);
    INFER_OK(op, "?;?", "[?,?]");
    INFER_OK(op, "[?,?];?", "[d0_0,?]");

    // Inputs are [M,M] and [M,K].  Output is [M,K].
    INFER_OK(op, "[?,?];[1,?]", "[d1_0,d1_1]");
    INFER_OK(op, "[1,?];[1,?]", "[d0_0|d1_0,d1_1]");
    INFER_OK(op, "[?,1];[1,?]", "[d0_1|d1_0,d1_1]");
    INFER_OK(op, "[1,1];[?,?]", "[d0_0|d0_1,d1_1]");
    INFER_OK(op, "[1,1];[1,?]", "[d0_0|d0_1|d1_0,d1_1]");
    INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1];?");
    INFER_ERROR("Shape must be rank 2 but is rank 1", op, "?;[1]");
    INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[1,2];?");
  }
}

TEST(LinalgOpsTest, BatchSquareMatrixSolve_ShapeFn) {
  for (const char* op_name :
       {"BatchMatrixSolve", "BatchMatrixTriangularSolve"}) {
    ShapeInferenceTestOp op(op_name);
    INFER_OK(op, "?;?", "?");
    INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "[1];?");
    INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "?;[1]");
    INFER_ERROR("Dimensions must be equal, but are 1 and 2", op, "[1,2];?");

    INFER_OK(op, "[?,?];?", "[d0_0|d0_1,?]");

    // Inputs are [...,M,M] and [...,M,K].  Output is [...,M,K].
    // First test where ... is empty.
    INFER_OK(op, "[?,?];[1,?]", "[d1_0,d1_1]");
    INFER_OK(op, "[1,?];[1,?]", "[d0_0|d1_0,d1_1]");
    INFER_OK(op, "[?,1];[1,?]", "[d0_1|d1_0,d1_1]");
    INFER_OK(op, "[1,1];[?,?]", "[d0_0,d1_1]");
    INFER_OK(op, "[1,1];[1,?]", "[d0_0|d0_1|d1_0,d1_1]");

    // Test with ... being 2-d.
    INFER_OK(op, "[10,?,?,?];[?,20,1,?]", "[d0_0,d1_1,d1_2,d1_3]");
    INFER_OK(op, "[10,?,1,?];[?,20,1,?]", "[d0_0,d1_1,d0_2|d1_2,d1_3]");
    INFER_OK(op, "[10,?,?,1];[?,20,1,?]", "[d0_0,d1_1,d0_3|d1_2,d1_3]");
    INFER_OK(op, "[10,?,1,1];[?,20,?,?]", "[d0_0,d1_1,d0_2,d1_3]");
    INFER_OK(op, "[10,?,1,1];[?,20,1,?]", "[d0_0,d1_1,d0_2|d0_3|d1_2,d1_3]");
  }
}

TEST(LinalgOpsTest, MatrixSolveLs_ShapeFn) {
  ShapeInferenceTestOp op("MatrixSolveLs");
  INFER_OK(op, "?;?;?", "[?,?]");

  // TODO(cwhipkey): l2_regularizer (input[2]) can have rank asserted?
  // (same question for BatchMatrixSolveLs).

  // Inputs are [M,N], [M,K], and batch_regularizer.  Output is [N,K]
  INFER_OK(op, "[1,?];[1,?];?", "[d0_1,d1_1]");
  INFER_OK(op, "[1,2];[1,3];?", "[d0_1,d1_1]");

  // First dims must be compatible.
  INFER_ERROR("Dimensions must be equal, but are 5 and 6", op, "[5,?];[6,?];?");

  // Rank checks.
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1];?;?");
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "?;[1];?");
}

TEST(LinalgOpsTest, BatchMatrixSolveLs_ShapeFn) {
  ShapeInferenceTestOp op("BatchMatrixSolveLs");
  INFER_OK(op, "?;?;?", "?");

  // Inputs are [...,M,N], [...,M,K], and batch_regularizer.
  // Output is [...,N,K]

  // Test with no batch dims.
  INFER_OK(op, "[1,?];[1,?];?", "[d0_1,d1_1]");
  INFER_OK(op, "[1,2];[1,3];?", "[d0_1,d1_1]");
  INFER_ERROR("Dimensions must be equal, but are 5 and 6", op, "[5,?];[6,?];?");

  // Test with batch dims.
  INFER_OK(op, "[10,?,1,?];[?,20,1,?];?", "[d0_0,d1_1,d0_3,d1_3]");
  INFER_OK(op, "[10,20,1,2];[10,20,1,3];?", "[d0_0|d1_0,d0_1|d1_1,d0_3,d1_3]");
  INFER_ERROR("Dimensions must be equal, but are 5 and 6", op,
              "[10,?,5,?];[?,20,6,?];?");

  // Rank checks.
  INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "[1];?;?");
  INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "?;[1];?");
}

TEST(LinalgOpsTest, Svd_ShapeFn) {
  ShapeInferenceTestOp op("Svd");
  auto set_attrs = [&op](bool compute_uv, bool full_matrices) {
    TF_CHECK_OK(NodeDefBuilder("test", "Svd")
                    .Input({"input", 0, DT_FLOAT})
                    .Attr("compute_uv", compute_uv)
                    .Attr("full_matrices", full_matrices)
                    .Finalize(&op.node_def));
  };

  set_attrs(false, false);
  INFER_OK(op, "?", "[?];[0];[0]");
  INFER_OK(op, "[?,?]", "[?];[0];[0]");
  INFER_OK(op, "[2,?]", "[?];[0];[0]");
  INFER_OK(op, "[?,2]", "[?];[0];[0]");
  INFER_OK(op, "[2,2]", "[d0_0];[0];[0]");
  INFER_OK(op, "[3,2]", "[d0_1];[0];[0]");
  INFER_OK(op, "[2,3]", "[d0_0];[0];[0]");
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Shape must be rank 2 but is rank 3", op, "[1,2,3]");

  set_attrs(true, false);
  INFER_OK(op, "?", "[?];[?,?];[?,?]");
  INFER_OK(op, "[?,?]", "[?];[d0_0,?];[d0_1,?]");
  INFER_OK(op, "[2,?]", "[?];[d0_0,?];[d0_1,?]");
  INFER_OK(op, "[?,2]", "[?];[d0_0,?];[d0_1,?]");
  INFER_OK(op, "[2,2]", "[d0_0];[d0_0,d0_0];[d0_1,d0_0]");
  INFER_OK(op, "[3,2]", "[d0_1];[d0_0,d0_1];[d0_1,d0_1]");
  INFER_OK(op, "[2,3]", "[d0_0];[d0_0,d0_0];[d0_1,d0_0]");
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Shape must be rank 2 but is rank 3", op, "[1,2,3]");

  set_attrs(true, true);
  INFER_OK(op, "?", "[?];[?,?];[?,?]");
  INFER_OK(op, "[?,?]", "[?];[d0_0,d0_0];[d0_1,d0_1]");
  INFER_OK(op, "[2,?]", "[?];[d0_0,d0_0];[d0_1,d0_1]");
  INFER_OK(op, "[?,2]", "[?];[d0_0,d0_0];[d0_1,d0_1]");
  INFER_OK(op, "[2,2]", "[d0_0];[d0_0,d0_0];[d0_1,d0_1]");
  INFER_OK(op, "[3,2]", "[d0_1];[d0_0,d0_0];[d0_1,d0_1]");
  INFER_OK(op, "[2,3]", "[d0_0];[d0_0,d0_0];[d0_1,d0_1]");
  INFER_ERROR("Shape must be rank 2 but is rank 1", op, "[1]");
  INFER_ERROR("Shape must be rank 2 but is rank 3", op, "[1,2,3]");
}

TEST(LinalgOpsTest, BatchSvd_ShapeFn) {
  ShapeInferenceTestOp op("BatchSvd");
  auto set_attrs = [&op](bool compute_uv, bool full_matrices) {
    TF_CHECK_OK(NodeDefBuilder("test", "BatchSvd")
                    .Input({"input", 0, DT_FLOAT})
                    .Attr("compute_uv", compute_uv)
                    .Attr("full_matrices", full_matrices)
                    .Finalize(&op.node_def));
  };
  set_attrs(false, false);
  INFER_OK(op, "?", "?;[0];[0]");
  INFER_OK(op, "[?,?,?]", "[d0_0,?];[0];[0]");
  INFER_OK(op, "[4,?,?]", "[d0_0,?];[0];[0]");
  INFER_OK(op, "[4,2,?]", "[d0_0,?];[0];[0]");
  INFER_OK(op, "[4,?,2]", "[d0_0,?];[0];[0]");
  INFER_OK(op, "[?,2,2]", "[d0_0,d0_1];[0];[0]");
  INFER_OK(op, "[4,2,2]", "[d0_0,d0_1];[0];[0]");
  INFER_OK(op, "[?,3,2]", "[d0_0,d0_2];[0];[0]");
  INFER_OK(op, "[4,3,2]", "[d0_0,d0_2];[0];[0]");
  INFER_OK(op, "[?,2,3]", "[d0_0,d0_1];[0];[0]");
  INFER_OK(op, "[4,2,3]", "[d0_0,d0_1];[0];[0]");
  INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "[1]");

  set_attrs(true, false);
  INFER_OK(op, "?", "?;?;?");
  INFER_OK(op, "[?,?,?]", "[d0_0,?];[d0_0,d0_1,?];[d0_0,d0_2,?]");
  INFER_OK(op, "[4,?,?]", "[d0_0,?];[d0_0,d0_1,?];[d0_0,d0_2,?]");
  INFER_OK(op, "[4,2,?]", "[d0_0,?];[d0_0,d0_1,?];[d0_0,d0_2,?]");
  INFER_OK(op, "[4,?,2]", "[d0_0,?];[d0_0,d0_1,?];[d0_0,d0_2,?]");
  INFER_OK(op, "[?,2,2]", "[d0_0,d0_1];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_1]");
  INFER_OK(op, "[4,2,2]", "[d0_0,d0_1];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_1]");
  INFER_OK(op, "[?,3,2]", "[d0_0,d0_2];[d0_0,d0_1,d0_2];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[4,3,2]", "[d0_0,d0_2];[d0_0,d0_1,d0_2];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[?,2,3]", "[d0_0,d0_1];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_1]");
  INFER_OK(op, "[4,2,3]", "[d0_0,d0_1];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_1]");
  INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "[1]");

  set_attrs(true, true);
  INFER_OK(op, "?", "?;?;?");
  INFER_OK(op, "[?,?,?]", "[d0_0,?];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[4,?,?]", "[d0_0,?];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[4,2,?]", "[d0_0,?];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[4,?,2]", "[d0_0,?];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[?,2,2]", "[d0_0,d0_1];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[4,2,2]", "[d0_0,d0_1];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[?,3,2]", "[d0_0,d0_2];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[4,3,2]", "[d0_0,d0_2];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[?,2,3]", "[d0_0,d0_1];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_OK(op, "[4,2,3]", "[d0_0,d0_1];[d0_0,d0_1,d0_1];[d0_0,d0_2,d0_2]");
  INFER_ERROR("Shape must be at least rank 2 but is rank 1", op, "[1]");
}

}  // end namespace tensorflow
