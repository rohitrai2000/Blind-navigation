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
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_testutil.h"
#include "tensorflow/core/lib/core/status_test_util.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {

TEST(MathOpsTest, LookupTableFind) {
  ShapeInferenceTestOp op("LookupTableFind");
  INFER_OK(op, "?;?;?", "?");
  INFER_OK(op, "[];[];[]", "?");
  INFER_OK(op, "[];[1,2,3];[]", "?");

  // Dims 0 and 2 (table_handle and default_value must be scalars.
  INFER_ERROR("Shape must be rank 0 but is rank 1", op, "[1];[1,2,3];[]");
  INFER_ERROR("Shape must be rank 0 but is rank 2", op, "[];[1,2,3];[1,2]");
}

TEST(MathOpsTest, LookupTableInsert) {
  ShapeInferenceTestOp op("LookupTableInsert");
  INFER_OK(op, "?;?;?", "");
  INFER_OK(op, "[];[];[]", "");

  // Dim 0 (table handle) must be a scalar.
  INFER_ERROR("Shape must be rank 0 but is rank 1", op, "[1];[1,2,3];[]");

  // Dims 1 and 2 (keys and values) are the same.
  INFER_OK(op, "[];[1,?,3];[?,2,?]", "");
  INFER_ERROR("Dimension 2 in both shapes must be equal, but are 3 and 4", op,
              "[];[1,?,3];[?,2,4]");
}

TEST(MathOpsTest, LookupTableSize) {
  ShapeInferenceTestOp op("LookupTableSize");
  // Always scalar output.
  INFER_OK(op, "?", "[]");
  INFER_OK(op, "[]", "[]");

  // Dim 0 (table handle) must be a scalar.
  INFER_ERROR("Shape must be rank 0 but is rank 1", op, "[1]");
}

TEST(MathOpsTest, LookupTableExport) {
  ShapeInferenceTestOp op("LookupTableExport");
  // Always one vector output and one unknown size output.
  INFER_OK(op, "?", "[?];?");
  INFER_OK(op, "[]", "[?];?");

  // Dim 0 (table handle) must be a scalar.
  INFER_ERROR("Shape must be rank 0 but is rank 1", op, "[1]");
}

TEST(MathOpsTest, InitializeTable) {
  ShapeInferenceTestOp op("InitializeTable");
  // Always no output.
  INFER_OK(op, "?;?;?", "");

  // Dim 0 (table handle) must be a scalar.
  INFER_ERROR("Shape must be rank 0 but is rank 1", op, "[1];[];[]");

  // Dims 1 and 2 (keys and values) are the same size and must be vectors.
  INFER_ERROR("Dimension 0 in both shapes must be equal, but are 1 and 2", op,
              "?;[1];[2]");
  INFER_ERROR("Shape must be rank 1 but is rank 2", op, "?;[1,2];[1,2]");
}

TEST(MathOpsTest, InitializeTableFromTextFile) {
  ShapeInferenceTestOp op("InitializeTableFromTextFile");
  // Always no output.
  INFER_OK(op, "?;?", "");

  // Dim 0 (table handle) and Dim 1 (filename) must be scalars.
  INFER_ERROR("Shape must be rank 0 but is rank 1", op, "[1];[]");
  INFER_ERROR("Shape must be rank 0 but is rank 1", op, "[];[1]");
}

TEST(MathOpsTest, DynamicPartition) {
  ShapeInferenceTestOp op("DynamicPartition");
  TF_ASSERT_OK(NodeDefBuilder("test", "DynamicPartition")
                   .Input("data", 0, DT_FLOAT_REF)
                   .Input("indices", 0, DT_INT32)
                   .Attr("num_partitions", 4)
                   .Finalize(&op.node_def));

  // Unknown rank for indices, so unknown shape.
  INFER_OK(op, "?;?", "?;?;?;?");

  // 3 dimensional data, 2 dimensional indices.
  INFER_OK(op, "[3,4,5];[3,4]", "[?,d0_2];[?,d0_2];[?,d0_2];[?,d0_2]");

  TF_ASSERT_OK(NodeDefBuilder("test", "DynamicPartition")
                   .Input("data", 0, DT_FLOAT)
                   .Input("indices", 0, DT_INT32)
                   .Attr("num_partitions", 2)
                   .Finalize(&op.node_def));

  // Suffix after matching prefix is copied over.
  INFER_OK(op, "[3,4,5,6];[3,4]", "[?,d0_2,d0_3];[?,d0_2,d0_3]");

  // Does not start with proper prefix
  INFER_ERROR("Dimensions must be equal, but are 4 and 100", op,
              "[3,4,5];[3,100]");
}

TEST(MathOpsTest, DynamicStitch) {
  ShapeInferenceTestOp op("DynamicStitch");
  TF_ASSERT_OK(
      NodeDefBuilder("test", "DynamicStitch")
          .Input({{"indices", 0, DT_INT32}, {"indices_2", 1, DT_INT32}})
          .Input({{"data", 0, DT_FLOAT}, {"data_2", 1, DT_FLOAT}})
          .Attr("N", 2)
          .Finalize(&op.node_def));

  INFER_OK(op, "[2,3];[5,6];[2,3,4,5];[5,6,4,5]", "[?,d2_2,d2_3]");

  // Bad prefix for the second data input.
  INFER_ERROR("Dimensions must be equal, but are 10 and 5", op,
              "[2,3];[5,6];[2,3,4,5];[10,11,4,5]");

  // Inconsistent suffix dimensions
  INFER_ERROR("Dimension 0 in both shapes must be equal, but are 4 and 13", op,
              "[2,3];[5,6];[2,3,4,5];[5,6,13,14]");
}

}  // end namespace tensorflow
