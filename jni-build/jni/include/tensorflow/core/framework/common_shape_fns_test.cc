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
#include "tensorflow/core/framework/common_shape_fns.h"

#include "tensorflow/core/framework/node_def_builder.h"
#include "tensorflow/core/framework/op_def_builder.h"
#include "tensorflow/core/framework/shape_inference_testutil.h"
#include "tensorflow/core/framework/tensor_testutil.h"
#include "tensorflow/core/lib/core/status_test_util.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {
namespace shape_inference {

TEST(CommonShapeFnsTest, NoOutputShapeTest) {
  OpRegistrationData op_reg_data;
  TF_CHECK_OK(OpDefBuilder("Assert")
                  .Input("condition: bool")
                  .Input("data: float")
                  .Finalize(&op_reg_data));
  OpDef op_def = op_reg_data.op_def;

  NodeDef def;
  TF_CHECK_OK(NodeDefBuilder("test", "Assert")
                  .Input("condition", 0, DT_BOOL)
                  .Input({{"data", 0, DT_FLOAT}})
                  .Finalize(&def));

  InferenceContext c(&def, op_def, {"[]", "[10]"}, {});
  TF_EXPECT_OK(NoOutputs(&c));
  EXPECT_EQ(0, c.num_outputs());
}

TEST(CommonShapeFnsTest, ScalarShapeTest) {
  OpRegistrationData op_reg_data;
  TF_CHECK_OK(OpDefBuilder("L2Loss")
                  .Input("t: float")
                  .Output("t: float")
                  .Finalize(&op_reg_data));
  OpDef op_def = op_reg_data.op_def;

  NodeDef def;
  TF_CHECK_OK(
      NodeDefBuilder("test", "L2Loss").Input("t", 0, DT_FLOAT).Finalize(&def));

  {
    InferenceContext c(&def, op_def, {"[]"}, {});
    TF_EXPECT_OK(ScalarShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(0, c.Rank(output));
  }

  {
    InferenceContext c(&def, op_def, {"[1,23,4,4,2]"}, {});
    TF_EXPECT_OK(ScalarShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(0, c.Rank(output));
  }
}

TEST(CommonShapeFnsTest, MatMulShapeTest) {
  OpRegistrationData op_reg_data;
  TF_CHECK_OK(OpDefBuilder("MatMul")
                  .Input("a: float")
                  .Input("b: float")
                  .Output("c: float")
                  .Attr("transpose_a:bool=false")
                  .Attr("transpose_b:bool=false")
                  .Finalize(&op_reg_data));
  OpDef op_def = op_reg_data.op_def;

  NodeDef def;
  TF_CHECK_OK(NodeDefBuilder("test", "MatMul")
                  .Input("a", 0, DT_FLOAT)
                  .Input("b", 0, DT_FLOAT)
                  .Attr("transpose_a", false)
                  .Attr("transpose_b", false)
                  .Finalize(&def));

  {
    InferenceContext c(&def, op_def, {"[2,3]", "[3,4]"}, {});
    TF_EXPECT_OK(MatMulShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(2, c.Value(c.Dim(output, 0)));
    EXPECT_EQ(4, c.Value(c.Dim(output, 1)));
  }

  {
    // Unknown inner dimension for one
    InferenceContext c(&def, op_def, {"[2,?]", "[3,4]"}, {});
    TF_EXPECT_OK(MatMulShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(2, c.Value(c.Dim(output, 0)));
    EXPECT_EQ(4, c.Value(c.Dim(output, 1)));
  }

  {
    // Invalid rank.
    InferenceContext c(&def, op_def, {"[2]", "[3,4]"}, {});
    auto s = MatMulShape(&c);
    EXPECT_FALSE(s.ok());
    EXPECT_EQ("Invalid argument: Shape must be rank 2 but is rank 1",
              s.ToString());
  }

  {
    // Unknown outer dimension
    InferenceContext c(&def, op_def, {"[2,3]", "[3,?]"}, {});
    TF_EXPECT_OK(MatMulShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(2, c.Value(c.Dim(output, 0)));
    EXPECT_FALSE(c.ValueKnown(c.Dim(output, 1)));
  }

  {
    // Inner shapes not compatible
    InferenceContext c(&def, op_def, {"[2,5]", "[3,4]"}, {});
    auto s = MatMulShape(&c);
    EXPECT_FALSE(s.ok());
    EXPECT_EQ("Invalid argument: Dimensions must be equal, but are 5 and 3",
              s.ToString());
  }

  {
    // Inner shapes not compatible
    InferenceContext c(&def, op_def, {"[2,5,3]", "[3,5,4]"}, {});
    auto s = MatMulShape(&c);
    EXPECT_FALSE(s.ok());
    EXPECT_EQ("Invalid argument: Shape must be rank 2 but is rank 3",
              s.ToString());
  }

  {
    // transpose_a
    TF_CHECK_OK(NodeDefBuilder("test", "MatMul")
                    .Input("a", 0, DT_FLOAT)
                    .Input("b", 0, DT_FLOAT)
                    .Attr("transpose_a", true)
                    .Attr("transpose_b", false)
                    .Attr("type", DT_FLOAT)
                    .Finalize(&def));

    InferenceContext c(&def, op_def, {"[3,2]", "[3,4]"}, {});
    auto s = MatMulShape(&c);
    const Shape* output = c.output(0);
    EXPECT_EQ(2, c.Value(c.Dim(output, 0)));
    EXPECT_EQ(4, c.Value(c.Dim(output, 1)));
  }

  {
    // transpose_b
    TF_CHECK_OK(NodeDefBuilder("test", "MatMul")
                    .Input("a", 0, DT_FLOAT)
                    .Input("b", 0, DT_FLOAT)
                    .Attr("transpose_a", false)
                    .Attr("transpose_b", true)
                    .Attr("type", DT_FLOAT)
                    .Finalize(&def));

    InferenceContext c(&def, op_def, {"[2,3]", "[4,3]"}, {});
    auto s = MatMulShape(&c);
    const Shape* output = c.output(0);
    EXPECT_EQ(2, c.Value(c.Dim(output, 0)));
    EXPECT_EQ(4, c.Value(c.Dim(output, 1)));
  }
}

TEST(CommonShapeFnsTest, BiasAddShapeTest) {
  OpRegistrationData op_reg_data;
  TF_CHECK_OK(OpDefBuilder("BiasAdd")
                  .Input("a: float")
                  .Input("b: float")
                  .Output("c: float")
                  .Finalize(&op_reg_data));

  OpDef op_def = op_reg_data.op_def;
  NodeDef def;
  TF_CHECK_OK(NodeDefBuilder("test", "BiasAdd")
                  .Input("a", 0, DT_FLOAT)
                  .Input("b", 0, DT_FLOAT)
                  .Finalize(&def));

  {
    InferenceContext c(&def, op_def, {"[2,10]", "[10]"}, {});
    TF_EXPECT_OK(BiasAddShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(2, c.Value(c.Dim(output, 0)));
    EXPECT_EQ(10, c.Value(c.Dim(output, 1)));
  }

  {
    // Unknown ranks.
    InferenceContext c(&def, op_def, {"?", "?"}, {});
    TF_EXPECT_OK(BiasAddShape(&c));
    const Shape* output = c.output(0);
    EXPECT_FALSE(c.RankKnown(output));
  }

  {
    // Rank > 2
    InferenceContext c(&def, op_def, {"[4,3,4,2,15]", "[15]"}, {});
    TF_EXPECT_OK(BiasAddShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ("[4,3,4,2,15]", c.DebugString(output));
  }

  {
    // NCHW format
    TF_CHECK_OK(NodeDefBuilder("test", "BiasAdd")
                    .Input("a", 0, DT_FLOAT)
                    .Input("b", 0, DT_FLOAT)
                    .Attr("data_format", "NCHW")
                    .Finalize(&def));
    InferenceContext c(&def, op_def, {"[2,3,4,5]", "[3]"}, {});
    TF_EXPECT_OK(BiasAddShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ("[2,3,4,5]", c.DebugString(output));
  }

  {
    // NCHW format with high input rank
    TF_CHECK_OK(NodeDefBuilder("test", "BiasAdd")
                    .Input("a", 0, DT_FLOAT)
                    .Input("b", 0, DT_FLOAT)
                    .Attr("data_format", "NCHW")
                    .Finalize(&def));
    InferenceContext c(&def, op_def, {"[8,6,4,2,3,4,5]", "[3]"}, {});
    TF_EXPECT_OK(BiasAddShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ("[8,6,4,2,3,4,5]", c.DebugString(output));
  }

  {
    // Input rank not high enough
    InferenceContext c(&def, op_def, {"[3]", "[3]"}, {});
    EXPECT_FALSE(BiasAddShape(&c).ok());
  }

  {
    // NCHW rank not high enough
    TF_CHECK_OK(NodeDefBuilder("test", "BiasAdd")
                    .Input("a", 0, DT_FLOAT)
                    .Input("b", 0, DT_FLOAT)
                    .Attr("data_format", "NCHW")
                    .Finalize(&def));
    // NCHW format
    InferenceContext c(&def, op_def, {"[2,3,4]", "[3]"}, {});
    EXPECT_FALSE(BiasAddShape(&c).ok());
  }
}

TEST(CommonShapeFnsTest, BiasAddGradShapeTest) {
  OpRegistrationData op_reg_data;
  TF_CHECK_OK(OpDefBuilder("BiasAddGrad")
                  .Input("a: float")
                  .Output("b: float")
                  .Finalize(&op_reg_data));

  OpDef op_def = op_reg_data.op_def;
  NodeDef def;
  TF_CHECK_OK(NodeDefBuilder("test", "BiasAddGrad")
                  .Input("a", 0, DT_FLOAT)
                  .Finalize(&def));

  {
    InferenceContext c(&def, op_def, {"[2,10]"}, {});
    TF_EXPECT_OK(BiasAddGradShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(10, c.Value(c.Dim(output, 0)));
  }

  {
    // Rank > 2
    InferenceContext c(&def, op_def, {"[5,7,2,10]"}, {});
    TF_EXPECT_OK(BiasAddGradShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(10, c.Value(c.Dim(output, 0)));
  }

  {
    // NCHW format
    TF_CHECK_OK(NodeDefBuilder("test", "BiasAddGrad")
                    .Input("a", 0, DT_FLOAT)
                    .Attr("data_format", "NCHW")
                    .Finalize(&def));
    InferenceContext c(&def, op_def, {"[2,3,4,5]"}, {});
    TF_EXPECT_OK(BiasAddGradShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(3, c.Value(c.Dim(output, 0)));
  }

  {
    // NCHW format with high input rank
    TF_CHECK_OK(NodeDefBuilder("test", "BiasAddGrad")
                    .Input("a", 0, DT_FLOAT)
                    .Attr("data_format", "NCHW")
                    .Finalize(&def));
    InferenceContext c(&def, op_def, {"[8,6,4,2,3,4,5]"}, {});
    TF_EXPECT_OK(BiasAddGradShape(&c));
    const Shape* output = c.output(0);
    EXPECT_EQ(3, c.Value(c.Dim(output, 0)));
  }

  {
    // Input rank not high enough
    InferenceContext c(&def, op_def, {"[3]"}, {});
    EXPECT_FALSE(BiasAddGradShape(&c).ok());
  }

  {
    // NCHW rank not high enough
    TF_CHECK_OK(NodeDefBuilder("test", "BiasAddGrad")
                    .Input("a", 0, DT_FLOAT)
                    .Attr("data_format", "NCHW")
                    .Finalize(&def));
    // NCHW format
    InferenceContext c(&def, op_def, {"[2,3,4]"}, {});
    EXPECT_FALSE(BiasAddGradShape(&c).ok());
  }
}

TEST(CommonShapeFnsTest, Conv2DShapeTest) {
  ShapeInferenceTestOp op("Conv2D");
  auto set_op = [&op](const std::vector<int32>& strides, const string& padding,
                      const string& data_format) {
    TF_CHECK_OK(NodeDefBuilder("test", "Conv2D")
                    .Input("input", 0, DT_FLOAT)
                    .Input("filter", 0, DT_FLOAT)
                    .Attr("strides", strides)
                    .Attr("padding", padding)
                    .Attr("data_format", data_format)
                    .Finalize(&op.node_def));
  };

  // 1x1 filter
  set_op({{1, 1, 1, 1}}, "VALID", "NHWC");
  INFER_OK(op, "[1,2,2,1];[1,1,1,1]", "[d0_0,2,2,d1_3]");

  // 2x2 filter
  set_op({{1, 1, 1, 1}}, "VALID", "NHWC");
  INFER_OK(op, "[1,2,2,1];[2,2,1,1]", "[d0_0,1,1,d1_3]");

  // 3x3 input, 1x1 filter, 2x2 stride
  set_op({{1, 2, 2, 1}}, "VALID", "NHWC");
  INFER_OK(op, "[1,3,3,1];[1,1,1,1]", "[d0_0,2,2,d1_3]");

  // 3x3 input, 1x1 filter, 2x1 stride
  set_op({{1, 2, 1, 1}}, "VALID", "NHWC");
  INFER_OK(op, "[1,3,3,1];[1,1,1,1]", "[d0_0,2,3,d1_3]");

  // 4x4 input, 2x1 filter, 1x2 stride
  set_op({{1, 1, 2, 1}}, "VALID", "NHWC");
  INFER_OK(op, "[1,4,4,1];[2,1,1,1]", "[d0_0,3,2,d1_3]");

  // Invalid rank for input
  INFER_ERROR("must be rank 4", op, "[4,4];[2,1,1,1]");
  // Invalid rank for filter
  INFER_ERROR("must be rank 4", op, "[1,4,4,1];[2,1,1]");

  // No unknown dims in the critical fields.
  INFER_ERROR("is not known", op, "[1,?,2,1];[1,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,?,1];[1,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,2,1];[?,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,2,1];[1,?,1,1]");

  // input depths must match.
  INFER_ERROR("Dimensions must be equal, but are 10 and 10000", op,
              "[1,2,2,10];[1,1,10000,20]");

  // Tests for NCHW
  // 1x1 filter
  set_op({{1, 1, 1, 1}}, "VALID", "NCHW");
  INFER_OK(op, "[1,1,2,2];[1,1,1,1]", "[d0_0,d1_3,2,2]");

  // 2x2 filter
  set_op({{1, 1, 1, 1}}, "VALID", "NCHW");
  INFER_OK(op, "[1,1,2,2];[2,2,1,1]", "[d0_0,d1_3,1,1]");

  // 3x3 input, 1x1 filter, 2x2 stride
  set_op({{1, 1, 2, 2}}, "VALID", "NCHW");
  INFER_OK(op, "[1,1,3,3];[1,1,1,1]", "[d0_0,d1_3,2,2]");

  // 3x3 input, 1x1 filter, 2x1 stride
  set_op({{1, 1, 2, 1}}, "VALID", "NCHW");
  INFER_OK(op, "[1,1,3,3];[1,1,1,1]", "[d0_0,d1_3,2,3]");

  // 4x4 input, 2x1 filter, 1x2 stride
  set_op({{1, 1, 1, 2}}, "VALID", "NCHW");
  INFER_OK(op, "[1,1,4,4];[2,1,1,1]", "[d0_0,d1_3,3,2]");

  // Some tests for "SAME" padding

  // 4x4 input, 1x1 filter, 1x1 stride
  set_op({{1, 1, 1, 1}}, "SAME", "NHWC");
  INFER_OK(op, "[1,4,4,1];[1,1,1,1]", "[d0_0,4,4,d1_3]");

  // 3x3 input, 2x2 filter, 1x1 stride
  set_op({{1, 1, 1, 1}}, "SAME", "NHWC");
  INFER_OK(op, "[1,3,3,1];[2,2,1,1]", "[d0_0,3,3,d1_3]");

  // 4x4 input, 2x2 filter, 2x2 stride
  set_op({{1, 2, 2, 1}}, "SAME", "NHWC");
  INFER_OK(op, "[1,4,4,1];[2,2,1,1]", "[d0_0,2,2,d1_3]");

  // 4x4 input, 2x2 filter, 1x1 stride
  set_op({{1, 1, 1, 1}}, "SAME", "NHWC");
  INFER_OK(op, "[1,4,4,1];[2,2,1,1]", "[d0_0,4,4,d1_3]");
}

TEST(CommonShapeFnsTest, Conv3DShapeTest) {
  ShapeInferenceTestOp op("Conv3D");
  auto set_op = [&op](const std::vector<int32>& strides,
                      const string& padding) {
    TF_CHECK_OK(NodeDefBuilder("test", "Conv3D")
                    .Input("input", 0, DT_FLOAT)
                    .Input("filter", 0, DT_FLOAT)
                    .Attr("strides", strides)
                    .Attr("padding", padding)
                    .Finalize(&op.node_def));
  };

  // 1x1x1 filter
  set_op({{1, 1, 1, 1, 1}}, "VALID");
  INFER_OK(op, "[1,2,2,2,1];[1,1,1,1,1]", "[d0_0,2,2,2,d1_4]");

  // Invalid rank for input
  INFER_ERROR("must be rank 5", op, "[4,4];[2,1,1,1]");
  // Invalid rank for filter
  INFER_ERROR("must be rank 5", op, "[1,4,4,1];[2,1,1]");

  // No unknown dims in the critical fields.
  INFER_ERROR("is not known", op, "[1,?,2,2,1];[1,1,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,?,2,1];[1,1,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,2,?,1];[1,1,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,2,2,1];[?,1,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,2,2,1];[1,?,1,1,1]");

  // input depths must match.
  INFER_ERROR("Dimensions must be equal, but are 10 and 10000", op,
              "[1,2,2,2,10];[1,1,1,10000,20]");

  // 2x2x2 filter
  set_op({{1, 1, 1, 1, 1}}, "VALID");
  INFER_OK(op, "[1,2,2,2,1];[2,2,2,1,1]", "[d0_0,1,1,1,d1_4]");

  // 3x3 input, 1x1 filter, 2x2 stride
  set_op({{1, 2, 2, 2, 1}}, "VALID");
  INFER_OK(op, "[1,3,3,3,1];[1,1,1,1,1]", "[d0_0,2,2,2,d1_4]");

  // 3x3 input, 1x1 filter, 2x1x1 stride
  set_op({{1, 2, 1, 1, 1}}, "VALID");
  INFER_OK(op, "[1,3,3,3,1];[1,1,1,1,1]", "[d0_0,2,3,3,d1_4]");

  // 4x4 input, 2x2 filter, 1x1 stride
  set_op({{1, 1, 1, 1, 1}}, "SAME");
  INFER_OK(op, "[1,4,4,4,1];[2,2,2,1,1]", "[d0_0,4,4,4,d1_4]");
}

TEST(CommonShapeFnsTest, DepthwiseConv2DShapeTest) {
  ShapeInferenceTestOp op("DepthwiseConv2dNative");
  std::vector<int32> strides = {{1, 1, 1, 1}};
  TF_CHECK_OK(NodeDefBuilder("test", "DepthwiseConv2dNative")
                  .Input("input", 0, DT_FLOAT)
                  .Input("filter", 0, DT_FLOAT)
                  .Attr("strides", strides)
                  .Attr("padding", "VALID")
                  .Finalize(&op.node_def));

  // Most of DepthwiseConv2D is implicitly tested by Conv2D, so
  // we test only the very-specific differences here.

  // 1x1 filter, depth multiplication
  INFER_OK(op, "[1,2,2,3];[1,1,3,4]", "[d0_0,2,2,12]");

  // Input depths not compatible
  INFER_ERROR("Dimensions must be equal, but are 3 and 12", op,
              "[1,2,2,3];[1,1,12,4]");

  // No unknown dims in the critical fields.
  INFER_ERROR("is not known", op, "[1,?,2,1];[1,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,?,1];[1,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,2,1];[?,1,1,1]");
  INFER_ERROR("is not known", op, "[1,2,2,1];[1,?,1,1]");
  INFER_ERROR("is not known", op, "[1,2,2,1];[1,1,?,1]");
  INFER_ERROR("is not known", op, "[1,2,2,1];[1,1,1,?]");
}

TEST(CommonShapeFnsTest, AvgPool2DShapeTest) {
  ShapeInferenceTestOp op("AvgPool");
  auto set_op = [&op](const std::vector<int32>& strides,
                      const std::vector<int32>& ksizes, const string& padding,
                      const string& data_format) {
    TF_CHECK_OK(NodeDefBuilder("test", "AvgPool")
                    .Input("input", 0, DT_FLOAT)
                    .Attr("strides", strides)
                    .Attr("ksize", ksizes)
                    .Attr("padding", padding)
                    .Attr("data_format", data_format)
                    .Finalize(&op.node_def));
  };

  // Most of the functionality is tested by conv-like shapes,
  // so we check the very-specific avgpooling features here.

  // 1x1 filter, 1x1 stride
  set_op({1, 1, 1, 1}, {1, 1, 1, 1}, "VALID", "NHWC");
  INFER_OK(op, "[1,2,2,1]", "[d0_0,2,2,d0_3]");

  // 4x4 input, 2x1 ksize, 1x2 stride
  set_op({1, 1, 2, 1}, {1, 2, 1, 1}, "VALID", "NHWC");
  INFER_OK(op, "[1,4,4,1]", "[d0_0,3,2,d0_3]");

  // No unknown dims in the critical fields.  Assumes NHWC format.
  INFER_ERROR("is not known", op, "[1,?,2,1]");
  INFER_ERROR("is not known", op, "[1,2,?,1]");

  // 4x4 input, 2x1 ksize, 1x2 stride, NCHW format
  set_op({{1, 1, 1, 2}}, {1, 1, 2, 1}, "VALID", "NCHW");
  INFER_OK(op, "[1,1,4,4]", "[d0_0,d0_1,3,2]");

  // Invalid rank for input
  INFER_ERROR("must be rank 4", op, "[4,4]");
}

TEST(CommonShapeFnsTest, MaxPool2DShapeTest) {
  ShapeInferenceTestOp op("MaxPool");
  auto set_op = [&op](const std::vector<int32>& strides,
                      const std::vector<int32>& ksizes, const string& padding,
                      const string& data_format) {
    TF_CHECK_OK(NodeDefBuilder("test", "MaxPool")
                    .Input("input", 0, DT_FLOAT)
                    .Attr("strides", strides)
                    .Attr("ksize", ksizes)
                    .Attr("padding", padding)
                    .Attr("data_format", data_format)
                    .Finalize(&op.node_def));
  };

  // Most of the functionality is tested by conv-like shapes,
  // so we check the very-specific maxpooling features here,
  // namely depthwise kernel and striding.

  // all 1 strides, depth 2 filter
  set_op({1, 1, 1, 1}, {1, 1, 1, 2}, "VALID", "NHWC");
  INFER_OK(op, "[1,2,2,2]", "[d0_0,2,2,1]");

  // depth 3 stride, 1x1x1 filter, NCHW
  set_op({1, 3, 1, 1}, {1, 1, 1, 1}, "VALID", "NCHW");
  INFER_OK(op, "[1,7,5,5]", "[d0_0,3,5,5]");
}

TEST(CommonShapeFnsTest, Pool3DShapeTest) {
  ShapeInferenceTestOp op("MaxPool3D");
  auto set_op = [&op](const std::vector<int32>& strides,
                      const std::vector<int32>& ksizes, const string& padding) {
    TF_CHECK_OK(NodeDefBuilder("test", "MaxPool3D")
                    .Input("input", 0, DT_FLOAT)
                    .Attr("strides", strides)
                    .Attr("ksize", ksizes)
                    .Attr("padding", padding)
                    .Finalize(&op.node_def));
  };

  // Most of the functionality is tested by conv-like shapes,
  // so we check that we handle the extra dimension properly.

  // 2x3x4 stride, 1x1x1 filter.
  set_op({1, 2, 3, 4, 1}, {1, 1, 1, 1, 1}, "VALID");
  INFER_OK(op, "[1,24,24,24,1]", "[d0_0,12,8,6,d0_4]");
}

TEST(CommonShapeFnsTest, UnknownShapeTest) {
  {
    // Single output
    ShapeInferenceTestOp op("QueueDequeue");
    TF_CHECK_OK(NodeDefBuilder("test", "QueueDequeue")
                    .Input("handle", 0, DT_STRING_REF)
                    .Attr("component_types", {DT_FLOAT})
                    .Finalize(&op.node_def));
    INFER_OK(op, "[1]", "?");
  }

  {
    // Multiple outputs
    ShapeInferenceTestOp op("QueueDequeue");
    TF_CHECK_OK(NodeDefBuilder("test", "QueueDequeue")
                    .Input("handle", 0, DT_STRING_REF)
                    .Attr("component_types", {DT_FLOAT, DT_FLOAT, DT_STRING})
                    .Finalize(&op.node_def));
    INFER_OK(op, "[1]", "?;?;?");
  }
}

}  // namespace shape_inference
}  // namespace tensorflow
