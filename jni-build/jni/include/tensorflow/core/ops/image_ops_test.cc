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

#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/node_def_builder.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference_testutil.h"
#include "tensorflow/core/framework/tensor_testutil.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {

TEST(ImageOpsTest, SampleDistortedBoundingBox_ShapeFn) {
  ShapeInferenceTestOp op("SampleDistortedBoundingBox");
  INFER_OK(op, "?;?", "[3];[3];[1,1,4]");
}

TEST(ImageOpsTest, Resize_ShapeFn) {
  for (const char* op_name : {"ResizeArea", "ResizeBicubic", "ResizeBilinear",
                              "ResizeNearestNeighbor"}) {
    ShapeInferenceTestOp op(op_name);
    op.input_tensors.resize(2);

    // Inputs are images and size.

    // Rank and size checks.
    INFER_ERROR("Shape must be rank 4 but is rank 5", op, "[1,2,3,4,5];?");
    INFER_ERROR("Shape must be rank 4 but is rank 3", op, "[1,2,3];?");
    INFER_ERROR("Shape must be rank 1 but is rank 0", op, "?;[]");
    INFER_ERROR("Dimension must be 2 but is 3", op, "?;[3]");

    // When the size tensor is not a constant, the middle dims are unknown.
    INFER_OK(op, "[1,?,3,?];[2]", "[d0_0,?,?,d0_3]");

    Tensor size_tensor = test::AsTensor<int32>({20, 30});
    op.input_tensors[1] = &size_tensor;
    INFER_OK(op, "[1,?,3,?];[2]", "[d0_0,20,30,d0_3]");
  }
}

TEST(ImageOpsTest, DecodeImage_ShapeFn) {
  for (const char* op_name : {"DecodeJpeg", "DecodePng"}) {
    ShapeInferenceTestOp op(op_name);

    // Rank check.
    INFER_ERROR("Shape must be rank 0 but is rank 1", op, "[1]");

    // Channel not set - output is unknown.
    INFER_OK(op, "[]", "[?,?,?]");

    // Set the channel and so that part of output shape is known.
    TF_CHECK_OK(NodeDefBuilder("test", op_name)
                    .Input({"a", 0, DT_STRING})
                    .Attr("channels", 4)
                    .Finalize(&op.node_def));
    INFER_OK(op, "[]", "[?,?,4]");

    // Negative channel value is rejected.
    TF_CHECK_OK(NodeDefBuilder("test", op_name)
                    .Input({"a", 0, DT_STRING})
                    .Attr("channels", -1)
                    .Finalize(&op.node_def));
    INFER_ERROR("channels must be non-negative, got -1", op, "[]");
  }
}

TEST(ImageOpsTest, EncodeImage_ShapeFn) {
  for (const char* op_name : {"EncodeJpeg", "EncodePng"}) {
    ShapeInferenceTestOp op(op_name);

    // Rank check.
    INFER_ERROR("Shape must be rank 3 but is rank 2", op, "[1,2]");

    INFER_OK(op, "[1,?,3]", "[]");  // output is always scalar.
  }
}

TEST(ImageOpsTest, Colorspace_ShapeFn) {
  for (const char* op_name : {"HSVToRGB", "RGBToHSV"}) {
    ShapeInferenceTestOp op(op_name);

    // Rank check.
    INFER_ERROR("Shape must be at least rank 1 but is rank 0", op, "[]");

    // Input's last dim is required to be 3.
    INFER_ERROR("Dimension must be 3 but is 4", op, "[1,2,4]");
    INFER_OK(op, "[1,2,3]", "[d0_0,d0_1,d0_2]");
    INFER_OK(op, "[1,2,?]", "[d0_0,d0_1,3]");
    INFER_OK(op, "?", "?");
  }
}

TEST(ImageOpsTest, ExtractGlimpse_ShapeFn) {
  ShapeInferenceTestOp op("ExtractGlimpse");
  op.input_tensors.resize(2);

  // Inputs are input, size, offsets.

  // Rank and size checks.
  INFER_ERROR("Shape must be rank 4 but is rank 5", op, "[1,2,3,4,5];?;?");
  INFER_ERROR("Shape must be rank 4 but is rank 3", op, "[1,2,3];?;?");
  INFER_ERROR("Shape must be rank 1 but is rank 0", op, "?;[];?");
  INFER_ERROR("Dimension must be 2 but is 3", op, "?;[3];?");
  INFER_ERROR("Shape must be rank 2 but is rank 3", op, "?;?;[1,2,3]");

  // When the size tensor is not a constant, the middle dims are unknown.
  INFER_OK(op, "[1,?,3,?];[2];?", "[d0_0,?,?,d0_3]");

  Tensor size_tensor = test::AsTensor<int32>({20, 30});
  op.input_tensors[1] = &size_tensor;
  INFER_OK(op, "[1,?,3,?];[2];?", "[d0_0,20,30,d0_3]");

  // input.dim(0) and offsets.dim(0) are both the batch dimension.
  INFER_OK(op, "[?,?,3,?];[2];[1,?]", "[d2_0,20,30,d0_3]");
  INFER_OK(op, "[1,?,3,?];[2];[1,?]", "[d0_0|d2_0,20,30,d_0|d0_3]");
  INFER_ERROR("Dimensions must be equal, but are 10 and 1", op,
              "[10,?,?,?];?;[1,2]");
}

TEST(ImageOpsTest, CropAndResize_ShapeFn) {
  ShapeInferenceTestOp op("CropAndResize");
  op.input_tensors.resize(4);

  // Inputs are:  input, boxes, box_ind, and crop_size.

  // Rank and size checks.
  INFER_ERROR("Shape must be rank 4 but is rank 5", op, "[1,2,3,4,5];?;?;?");
  INFER_ERROR("Shape must be rank 4 but is rank 3", op, "[1,2,3];?;?;?");
  INFER_ERROR("Shape must be rank 2 but is rank 3", op, "?;[1,2,3];?;?");
  INFER_ERROR("Shape must be rank 1 but is rank 2", op, "?;?;[1,2];?");
  INFER_ERROR("Shape must be rank 1 but is rank 2", op, "?;?;?;[1,2]");
  INFER_ERROR("Dimension must be 2 but is 1", op, "?;?;?;[1]");

  // When the size tensor is not a constant, the middle dims are unknown.
  INFER_OK(op, "[1,?,3,?];?;?;[2]", "[?,?,?,d0_3]");

  Tensor size_tensor = test::AsTensor<int32>({20, 30});
  op.input_tensors[3] = &size_tensor;
  INFER_OK(op, "[1,?,3,?];?;?;[2]", "[?,20,30,d0_3]");

  // boxes.dim(0) and box_ind.dim(0) are both the num_boxes dim.
  INFER_OK(op, "[1,?,3,?];[2,4];?;[2]", "[d1_0,20,30,d0_3]");
  INFER_OK(op, "[1,?,3,?];?;[2];[2]", "[d2_0,20,30,d0_3]");
  INFER_OK(op, "[1,?,3,?];[?,4];[?];[2]", "[d1_0|d3_0,20,30,d0_3]");
  INFER_ERROR("Dimensions must be equal, but are 2 and 1", op, "?;[2,?];[1];?");

  // boxes.dim(1) must be 4.
  INFER_ERROR("Dimension must be 4 but is 3", op, "?;[?,3];?;?");
}

TEST(ImageOpsTest, ResizeNearestNeighborGrad_ShapeFn) {
  ShapeInferenceTestOp op("ResizeNearestNeighborGrad");
  op.input_tensors.resize(2);

  // Rank and size checks.
  INFER_ERROR("Shape must be rank 4 but is rank 3", op, "[1,2,3];?");
  INFER_ERROR("Shape must be rank 1 but is rank 2", op, "?;[1,2]")
  INFER_ERROR("Dimension must be 2 but is 1", op, "?;[1]");

  // When the size tensor is not a constant, the middle dims are unknown.
  INFER_OK(op, "[1,?,3,?];[2]", "[d0_0,?,?,d0_3]");

  Tensor size_tensor = test::AsTensor<int32>({20, 30});
  op.input_tensors[1] = &size_tensor;
  INFER_OK(op, "[1,?,3,?];[2]", "[d0_0,20,30,d0_3]");
}

TEST(ImageOpsTest, CropAndResizeGradImage_ShapeFn) {
  ShapeInferenceTestOp op("CropAndResizeGradImage");
  op.input_tensors.resize(4);

  // Rank checks.
  INFER_ERROR("Shape must be rank 1 but is rank 2", op, "?;?;?;[1,2]");

  // Unknown image_size should result in output of rank 4 with unknown dims.
  INFER_OK(op, "?;?;?;?", "[?,?,?,?]");

  // Known image_size should result in full shape information.
  Tensor image_size = test::AsTensor<int32>({10, 20, 30, 40});
  op.input_tensors[3] = &image_size;
  INFER_OK(op, "?;?;?;[1]", "[10, 20, 30, 40]");
}

}  // end namespace tensorflow
