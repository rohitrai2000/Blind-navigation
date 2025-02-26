/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

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

#define EIGEN_USE_THREADS

#include <limits>

#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/contrib/quantization/kernels/quantization_utils.h"
#include "tensorflow/core/common_runtime/eigen_thread_pool.h"
#include "tensorflow/core/framework/allocator.h"
#include "tensorflow/core/framework/tensor_testutil.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/random/simple_philox.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {

class QuantizationUtilsTest : public ::testing::Test {
 protected:
  void TestRequantizeMany(Eigen::ThreadPoolDevice* eigen_device,
                          float input_min, float input_max, float output_min,
                          float output_max,
                          const std::vector<qint32>& values_quantized,
                          int tolerance = 1) {
    const int values_count = values_quantized.size();
    std::vector<quint8> expected_values;
    for (int value_index = 0; value_index < values_count; ++value_index) {
      expected_values.push_back(FloatToQuantized<quint8>(
          QuantizedToFloat(values_quantized[value_index], input_min, input_max),
          output_min, output_max));
    }

    Tensor i_tensor =
        tensorflow::test::AsTensor(gtl::ArraySlice<qint32>(values_quantized));
    Tensor o_tensor(DT_QUINT8, TensorShape{values_count});
    auto output_values = o_tensor.flat<quint8>();

    if (eigen_device == nullptr) {
      auto input_array = i_tensor.flat<qint32>();
      RequantizeManyInNewRange(input_array.data(), input_array.size(),
                               input_min, input_max, output_min, output_max,
                               output_values.data());
    } else {
      RequantizeManyInNewRangeUsingEigen<qint32, quint8>(
          *eigen_device, i_tensor, input_min, input_max, output_min, output_max,
          &o_tensor);
    }

    const string tolerance_str = strings::StrCat("+-", tolerance);
    for (size_t value_index = 0; value_index < values_count; ++value_index) {
      int e = expected_values[value_index];
      int v = output_values(value_index);
      ASSERT_TRUE(std::abs(e - v) <= tolerance)
          << "actual=" << v << ", expected=" << e << tolerance_str
          << ", values_quantized[" << value_index
          << "]=" << values_quantized[value_index]
          << ", input_min=" << input_min << ", input_max=" << input_max
          << ", output_min=" << output_min << ", output_max=" << output_max
          << ", value_index=" << value_index;
    }
  }

  // If eigen_device is NULL, then the reference implementation is tested.
  void TestRequantizeManyInNewRange32To8Bit(
      Eigen::ThreadPoolDevice* eigen_device) {
    // These are the float values we're going to test the conversions on.
    const size_t values_count = 6;
    const float values[values_count] = {0.0f,  0.45f,  1.0f,
                                        -1.0f, 127.0f, 255.0f};
    // These are the input and output ranges we'll test.
    const size_t ranges_count = 6;
    const float ranges[ranges_count][4] = {
        {0.0f, 255.0f, 0.0f, 255.0f},    //
        {0.0f, 1.0f, 0.0f, 1.0f},        //
        {-1.0f, 1.0f, -1.0f, 1.0f},      //
        {-1.0f, 1.0f, -255.0f, 255.0f},  //
        {3.0f, 3.0f, 0.0f, 255.0f},      // input min == max
        {0.0f, 255.0f, 5.0f, 5.0f},      // output min == max
    };
    for (int i = 0; i < ranges_count; ++i) {
      const auto& r = ranges[i];
      std::vector<qint32> values_quantized;
      for (int value_index = 0; value_index < values_count; ++value_index) {
        const float v = values[value_index];
        values_quantized.push_back(FloatToQuantized<qint32>(v, r[0], r[1]));
      }
      TestRequantizeMany(eigen_device, r[0], r[1], r[2], r[3],
                         values_quantized);
    }

    // Test with many different values in the input quantized range.
    qint32 low = Eigen::NumTraits<qint32>::lowest();
    qint32 high = Eigen::NumTraits<qint32>::highest();
    std::vector<qint32> vals{low, high};
    int num_steps = 14419;
    qint32 step = static_cast<int32>((1L << 32) / num_steps);
    qint32 v = low + static_cast<qint32>(1);
    for (int i = 0; i < num_steps; ++i) {
      vals.push_back(v);
      v += step;
    }
    TestRequantizeMany(eigen_device, -1.0f, 1.0f, -1.0f, 1.0f, vals);
    TestRequantizeMany(eigen_device, -255.0f, 255.0f, -255.0f, 255.0f, vals);
    TestRequantizeMany(eigen_device, -1.0f, 1.0f, -12345678.0f, 12345678.0f,
                       vals);
    TestRequantizeMany(eigen_device, -1.0f, 12345678.0f, -12345678.0f,
                       12345678.0f, vals);

    // Test when the input range is large and output range is small.
    // Use all quantized values where the float is in the output range.
    const float out_min = -29.1234;
    const float out_max = 23.1234;
    const float in_min = -1e6;
    const float in_max = 1e6;

    low = FloatToQuantized<qint32>(out_min, in_min, in_max);
    high = FloatToQuantized<qint32>(out_max, in_min, in_max);
    vals.clear();
    for (int32 i = low; i <= high; ++i) vals.push_back(i);
    TestRequantizeMany(eigen_device, in_min, in_max, out_min, out_max, vals);
  }

  template <typename InputType, typename OutputType>
  void TestRequantizeManyInNewRangeEigenVsNonEigen() {
    thread::ThreadPool threadpool(Env::Default(), "test", 2 /* num_threads */);
    EigenThreadPoolWrapper wrapper(&threadpool);
    Eigen::ThreadPoolDevice eigen_device(&wrapper, 2 /* num_threads */);

    const size_t ranges_count = 6;
    const float ranges[ranges_count][4] = {
        {0.0f, 255.0f, 0.0f, 255.0f},    //
        {0.0f, 1.0f, 0.0f, 1.0f},        //
        {-1.0f, 1.0f, -1.0f, 1.0f},      //
        {-1.0f, 1.0f, -255.0f, 255.0f},  //
        {3.0f, 3.0f, 0.0f, 255.0f},      // input min == max
        {0.0f, 255.0f, 5.0f, 5.0f},      // output min == max
    };

    // Random values.
    for (size_t range_index = 0; range_index < ranges_count; ++range_index) {
      const float input_min = ranges[range_index][0];
      const float input_max = ranges[range_index][1];
      const float output_min = ranges[range_index][2];
      const float output_max = ranges[range_index][3];
      const int values_count = 10000;
      random::PhiloxRandom philox(testing::RandomSeed(), 17);
      random::SimplePhilox rnd(&philox);
      std::vector<InputType> values_quantized;
      for (int i = 0; i < values_count; ++i) {
        float v = (rnd.RandFloat() * (input_max - input_min)) + input_min;
        values_quantized.push_back(
            FloatToQuantized<InputType>(v, input_min, input_max));
      }

      Tensor i_tensor = tensorflow::test::AsTensor(
          gtl::ArraySlice<InputType>(values_quantized));
      const auto i_array = i_tensor.flat<InputType>();
      Tensor o_tensor_eigen(DataTypeToEnum<OutputType>::v(),
                            TensorShape{values_count});
      auto output_values_eigen = o_tensor_eigen.flat<OutputType>();
      Tensor o_tensor_ref(DataTypeToEnum<OutputType>::v(),
                          TensorShape{values_count});
      auto output_values_ref = o_tensor_ref.flat<OutputType>();

      RequantizeManyInNewRange(i_array.data(), i_array.size(), input_min,
                               input_max, output_min, output_max,
                               output_values_ref.data());
      RequantizeManyInNewRangeUsingEigen<InputType, OutputType>(
          eigen_device, i_tensor, input_min, input_max, output_min, output_max,
          &o_tensor_eigen);

      const int tolerance = 1;
      for (int i = 0; i < values_quantized.size(); ++i) {
        auto expected = output_values_ref(i);
        auto actual = output_values_eigen(i);
        // The eigen computation uses float for constants and computation
        // instead of doubles, so can be different by 1 or 2 in some cases
        // (e.g., input value 144.062744140625, min -1, max 255, type quint8).
        ASSERT_TRUE(std::abs(expected - actual) <= tolerance)
            << "expected=" << expected << " actual=" << actual
            << " tolerance=" << tolerance << " v=" << values_quantized[i]
            << " i=" << i << " input_min=" << input_min
            << " input_max=" << input_max
            << " input_type=" << DataTypeString(DataTypeToEnum<InputType>::v())
            << " output_type="
            << DataTypeString(DataTypeToEnum<OutputType>::v());
      }
    }
  }

  template <typename T>
  void TestFloatToQuantizedInPlaceUsingEigen(
      Eigen::ThreadPoolDevice* eigen_device) {
    // These are the float values we're going to test the conversions on.
    typedef std::pair<float, float> FPair;
    for (FPair min_and_max : std::vector<FPair>{FPair(-255.0f, 255.0f),  //
                                                FPair(-1.0f, 1.0f),      //
                                                FPair(-1.0f, 255.0f),    //
                                                FPair(0.0f, 1e6),        //
                                                FPair(0.0f, 1.0f),       //
                                                FPair(-31.0f, 13.0f)}) {
      const float f_min = min_and_max.first;
      const float f_max = min_and_max.second;
      const float f_range = f_max - f_min;
      const int values_count = 50000;
      Tensor input(DT_FLOAT, TensorShape{values_count});
      auto input_array = input.flat<float>();
      for (int i = 0; i < values_count; ++i) {
        input_array(i) = f_min + f_range * i / (values_count - 1);
      }

      Tensor output(DataTypeToEnum<T>::v(), TensorShape{values_count});
      FloatTensorToQuantizedInPlaceUsingEigen<T>(*eigen_device, input, f_min,
                                                 f_max, &output);
      auto output_array = output.flat<T>();

      const int tolerance = 1;
      for (int i = 0; i < values_count; ++i) {
        int32 expected = FloatToQuantized<T>(input_array(i), f_min, f_max);
        int32 actual = output_array(i);

        // The eigen computation uses float for constants and computation
        // instead
        // of doubles, so can be different by 1 or 2 in some cases (e.g., input
        // value 144.062744140625, min -1, max 255, type quint8).
        ASSERT_TRUE(std::abs(expected - actual) <= tolerance)
            << "expected=" << expected << " actual=" << actual
            << " tolerance=" << tolerance << " v=" << input_array(i)
            << " i=" << i << " f_min=" << f_min << " f_max=" << f_max
            << " type=" << DataTypeString(DataTypeToEnum<T>::v());
      }
    }
  }

  template <typename T>
  void TestQuantizedToFloatInPlaceUsingEigen(
      Eigen::ThreadPoolDevice* eigen_device) {
    // These are the float values we're going to test the conversions on.
    typedef std::pair<float, float> FPair;
    for (FPair min_and_max : std::vector<FPair>{FPair(-255.0f, 255.0f),  //
                                                FPair(-1.0f, 1.0f),      //
                                                FPair(-1.0f, 255.0f),    //
                                                FPair(0.0f, 1e6),        //
                                                FPair(0.0f, 1.0f),       //
                                                FPair(-31.0f, 13.0f)}) {
      const float f_min = min_and_max.first;
      const float f_max = min_and_max.second;
      const int values_count = sizeof(T) == 1 ? 256 : 50000;
      Tensor input(DataTypeToEnum<T>::v(), TensorShape{values_count});
      auto input_array = input.flat<T>();
      const double q_range =
          static_cast<double>(Eigen::NumTraits<T>::highest()) -
          Eigen::NumTraits<T>::lowest();
      for (int i = 0; i < values_count; ++i) {
        if (sizeof(T) == 1) {
          input_array(i) = Eigen::NumTraits<T>::lowest() + i;
        } else {
          int64 offset = static_cast<int64>(q_range / values_count * i);
          input_array(i) = static_cast<int32>(
              Eigen::NumTraits<T>::lowest() +
              std::min<int64>(Eigen::NumTraits<T>::highest(), offset));
        }
      }

      Tensor output(DT_FLOAT, TensorShape{values_count});
      QuantizedTensorToFloatInPlaceUsingEigen<T>(*eigen_device, input, f_min,
                                                 f_max, &output);
      auto output_array = output.flat<float>();
      const double range = static_cast<double>(f_max) - f_min;
      for (int i = 0; i < values_count; ++i) {
        float expected = QuantizedToFloat<T>(input_array(i), f_min, f_max);
        float actual = output_array(i);
        ASSERT_NEAR(expected, actual, range * 1e-6)
            << "expected=" << expected << " actual=" << actual
            << " v=" << input_array(i) << " i=" << i << " f_min=" << f_min
            << " f_max=" << f_max
            << " type=" << DataTypeString(DataTypeToEnum<T>::v());
      }
    }
  }
};

TEST_F(QuantizationUtilsTest, FloatToQuantized) {
  EXPECT_EQ(quint8(0), FloatToQuantized<quint8>(0.0f, 0.0f, 1.0f));
  EXPECT_EQ(quint8(0), FloatToQuantized<quint8>(0.0f, 0.0f, 2.0f));
  EXPECT_EQ(quint8(128), FloatToQuantized<quint8>(0.5f, 0.0f, 1.0f));
  EXPECT_EQ(quint8(128), FloatToQuantized<quint8>(1.0f, 0.0f, 2.0f));
  EXPECT_EQ(quint8(255), FloatToQuantized<quint8>(1.0f, 0.0f, 1.0f));
  EXPECT_EQ(quint8(255), FloatToQuantized<quint8>(2.0f, 0.0f, 2.0f));
  EXPECT_EQ(quint8(0), FloatToQuantized<quint8>(-128.0f, -128.0f, 127.0f));
  EXPECT_EQ(quint8(128), FloatToQuantized<quint8>(0.0f, -128.0f, 127.0f));
  EXPECT_EQ(quint8(255), FloatToQuantized<quint8>(127.0f, -128.0f, 127.0f));
  EXPECT_EQ(quint8(0), FloatToQuantized<quint8>(1.0f, 1.0f, 256.0f));
  EXPECT_EQ(quint8(127), FloatToQuantized<quint8>(128.0f, 1.0f, 256.0f));
  EXPECT_EQ(quint8(255), FloatToQuantized<quint8>(256.0f, 1.0f, 256.0f));

  const int int32_min = std::numeric_limits<int>::min();
  const int int32_max = std::numeric_limits<int>::max();

  EXPECT_EQ(qint32(int32_min),
            FloatToQuantized<qint32>(-128.0f, -128.0f, 128.0f));
  EXPECT_EQ(qint32(0), FloatToQuantized<qint32>(0.0f, -128.0f, 128.0f));
  EXPECT_EQ(qint32(int32_max),
            FloatToQuantized<qint32>(128.0f, -128.0f, 128.0f));
}

TEST_F(QuantizationUtilsTest, QuantizedToFloat) {
  EXPECT_LT(fabsf(0.0f - QuantizedToFloat<quint8>(0, 0.0f, 1.0f)), 1 / 255.0f);
  EXPECT_LT(fabsf(0.0f - QuantizedToFloat<quint8>(0, 0.0f, 2.0f)), 1 / 255.0f);
  EXPECT_LT(fabsf(0.5f - QuantizedToFloat<quint8>(127, 0.0f, 1.0f)),
            1 / 255.0f);
  EXPECT_LT(fabsf(1.0f - QuantizedToFloat<quint8>(127, 0.0f, 2.0f)),
            1 / 255.0f);
  EXPECT_LT(fabsf(1.0f - QuantizedToFloat<quint8>(255, 0.0f, 1.0f)),
            1 / 255.0f);
  EXPECT_LT(fabsf(2.0f - QuantizedToFloat<quint8>(255, 0.0f, 2.0f)),
            1 / 255.0f);
  EXPECT_LT(fabsf(1.0f - QuantizedToFloat<quint8>(0, 1.0f, 256.0f)),
            1 / 255.0f);
  EXPECT_LT(fabsf(128.0f - QuantizedToFloat<quint8>(127, 1.0f, 256.0f)),
            1 / 255.0f);
  EXPECT_LT(fabsf(256.0f - QuantizedToFloat<quint8>(255, 1.0f, 256.0f)),
            1 / 255.0f);

  const int int32_min = std::numeric_limits<int>::min();
  const int int32_max = std::numeric_limits<int>::max();

  EXPECT_LT(
      fabsf(-1.0f - QuantizedToFloat<qint32>(qint32(int32_min), -1.0f, 1.0f)),
      1e-5f);
  EXPECT_LT(fabsf(0.0f - QuantizedToFloat<qint32>(qint32(0), -1.0f, 1.0f)),
            1e-5f);
  EXPECT_LT(
      fabsf(1.0f - QuantizedToFloat<qint32>(qint32(int32_max), -1.0f, 1.0f)),
      1e-5f);
}

TEST_F(QuantizationUtilsTest, AvoidBias) {
  for (int i = 0; i < 256; ++i) {
    const float as_float = QuantizedToFloat<quint8>(i, 0.0f, 2.0f);
    const int back_to_int = FloatToQuantized<quint8>(as_float, 0.0f, 2.0f);
    EXPECT_EQ(i, back_to_int);
  }
}

TEST_F(QuantizationUtilsTest, RequantizeInNewRange) {
  // These are the float values we're going to test the conversions on.
  const size_t values_count = 6;
  const float values[values_count] = {0.0f, 0.5f, 1.0f, -1.0f, 127.0f, 255.0f};
  // These are the input and output ranges we'll test.
  const size_t ranges_count = 4;
  const float ranges[ranges_count][4] = {
      {0.0f, 255.0f, 0.0f, 255.0f},
      {0.0f, 1.0f, 0.0f, 1.0f},
      {-1.0f, 1.0f, -1.0f, 1.0f},
      {-1.0f, 1.0f, -255.0f, 255.0f},
  };
  for (size_t value_index = 0; value_index < values_count; ++value_index) {
    const float value_float = values[value_index];
    for (size_t range_index = 0; range_index < ranges_count; ++range_index) {
      const float input_min = ranges[range_index][0];
      const float input_max = ranges[range_index][1];
      const float output_min = ranges[range_index][2];
      const float output_max = ranges[range_index][3];
      const quint8 input_value =
          FloatToQuantized<quint8>(value_float, input_min, input_max);
      // Here we convert the quantized input value to what we expect
      // to get in the output range.
      const qint32 expected_value = FloatToQuantized<qint32>(
          QuantizedToFloat(input_value, input_min, input_max), output_min,
          output_max);
      EXPECT_EQ(expected_value,
                (RequantizeInNewRange<quint8, qint32>(
                    input_value, input_min, input_max, output_min, output_max)))
          << "value_float=" << value_float << ", input_min=" << input_min
          << ", input_max=" << input_max << ", output_min=" << output_min
          << ", output_max=" << output_max;
    }
  }
}

TEST_F(QuantizationUtilsTest, RequantizeInNewRangeRealData) {
  const float value_as_float = -0.290169f;
  const float input_min = -0.739539f;
  const float input_max = 0.641057f;
  const float output_min = -2381.49f;
  const float output_max = 2207.6f;
  const quint8 value_as_quint8 =
      FloatToQuantized<quint8>(value_as_float, input_min, input_max);
  EXPECT_EQ(quint8(83), value_as_quint8);
  const qint32 actual_output = RequantizeInNewRange<quint8, qint32>(
      value_as_quint8, input_min, input_max, output_min, output_max);
  const qint32 value_as_qint32 =
      FloatToQuantized<qint32>(value_as_float, output_min, output_max);
  EXPECT_LT(std::abs(value_as_qint32 - actual_output), 10);
}

TEST_F(QuantizationUtilsTest, RequantizeInNewRange32To8Bit) {
  // These are the float values we're going to test the conversions on.
  const size_t values_count = 6;
  const float values[values_count] = {0.0f, 0.45f, 1.0f, -1.0f, 127.0f, 255.0f};
  // These are the input and output ranges we'll test.
  const size_t ranges_count = 4;
  const float ranges[ranges_count][4] = {
      {0.0f, 255.0f, 0.0f, 255.0f},
      {0.0f, 1.0f, 0.0f, 1.0f},
      {-1.0f, 1.0f, -1.0f, 1.0f},
      {-1.0f, 1.0f, -255.0f, 255.0f},
  };
  for (size_t value_index = 0; value_index < values_count; ++value_index) {
    const float value_float = values[value_index];
    for (size_t range_index = 0; range_index < ranges_count; ++range_index) {
      const float input_min = ranges[range_index][0];
      const float input_max = ranges[range_index][1];
      const float output_min = ranges[range_index][2];
      const float output_max = ranges[range_index][3];
      const qint32 input_value =
          FloatToQuantized<qint32>(value_float, input_min, input_max);
      // Here we convert the quantized input value to what we expect
      // to get in the output range.
      const quint8 expected_value = FloatToQuantized<quint8>(
          QuantizedToFloat(input_value, input_min, input_max), output_min,
          output_max);
      EXPECT_EQ(expected_value,
                (RequantizeInNewRange<qint32, quint8>(
                    input_value, input_min, input_max, output_min, output_max)))
          << "input_value=" << input_value << ", value_float=" << value_float
          << ", input_min=" << input_min << ", input_max=" << input_max
          << ", output_min=" << output_min << ", output_max=" << output_max;
    }
  }
}

TEST_F(QuantizationUtilsTest, RequantizeManyInNewRange32To8Bit) {
  TestRequantizeManyInNewRange32To8Bit(nullptr /* eigen_device */);
}

TEST_F(QuantizationUtilsTest, RequantizeManyInNewRange32To8BitUsingEigen) {
  thread::ThreadPool threadpool(Env::Default(), "test", 2 /* num_threads */);
  EigenThreadPoolWrapper wrapper(&threadpool);
  Eigen::ThreadPoolDevice eigen_device(&wrapper, 2 /* num_threads */);
  TestRequantizeManyInNewRange32To8Bit(&eigen_device);
}

TEST_F(QuantizationUtilsTest, RequantizeManyInNewRange32To8BitEigenVsNonEigen) {
  TestRequantizeManyInNewRangeEigenVsNonEigen<qint32, quint8>();
}

TEST_F(QuantizationUtilsTest,
       RequantizeManyInNewRange32To8BitSignedEigenVsNonEigen) {
  TestRequantizeManyInNewRangeEigenVsNonEigen<qint32, qint8>();
}

TEST_F(QuantizationUtilsTest, FloatTensorToQuantized) {
  const int input_width = 3;
  const int input_height = 3;
  const float input_min = 0.0f;
  const float input_max = 255.0f;
  Tensor input(DT_FLOAT, TensorShape({input_height, input_width}));
  test::FillValues<float>(&input, {1.0f, -1.0f, 10.0f, 10.25f, 127.0f, 255.0f,
                                   512.0f, 0.0f, 23.0f});
  Tensor expected(DT_QUINT8, TensorShape({input_height, input_width}));
  test::FillValues<quint8>(&expected, {1, 0, 10, 10, 127, 255, 255, 0, 23});
  Tensor output = FloatTensorToQuantized<quint8>(input, input_min, input_max);
  test::ExpectTensorEqual<quint8>(expected, output);
}

// Verify that FloatToQuantizedInPlaceUsingEigen is same result as
// FloatToQuantized.
TEST_F(QuantizationUtilsTest, FloatToQuantizedInPlaceUsingEigen) {
  thread::ThreadPool threadpool(Env::Default(), "test", 2 /* num_threads */);
  EigenThreadPoolWrapper wrapper(&threadpool);
  Eigen::ThreadPoolDevice eigen_device(&wrapper, 2 /* num_threads */);

  TestFloatToQuantizedInPlaceUsingEigen<quint8>(&eigen_device);
  TestFloatToQuantizedInPlaceUsingEigen<qint8>(&eigen_device);
  TestFloatToQuantizedInPlaceUsingEigen<quint16>(&eigen_device);
  TestFloatToQuantizedInPlaceUsingEigen<qint16>(&eigen_device);
}

TEST_F(QuantizationUtilsTest, OverflowWithEigen) {
  thread::ThreadPool threadpool(Env::Default(), "test", 2 /* num_threads */);
  EigenThreadPoolWrapper wrapper(&threadpool);
  Eigen::ThreadPoolDevice eigen_device(&wrapper, 2 /* num_threads */);

  const int num_vals = 4;
  const float input_min = 0.0f;
  const float input_max = 2400.0f;
  TensorShape shape({num_vals});
  Tensor input(DT_FLOAT, shape);
  test::FillValues<float>(&input, {-100.f, 0.f, 2400.0f, 2400.0f});
  Tensor expected(DT_QINT32, shape);
  // Note that the positive expected values are not the highest int32 value,
  // because the implementation does a bounds check using float, not int32.
  test::FillValues<qint32>(
      &expected,
      {static_cast<int32>(-2147483648), static_cast<int32>(-2147483648),
       static_cast<int32>(2147483520), static_cast<int32>(2147483520)});

  FloatToQuantizedStruct<qint32> f2q(input_min, input_max);
  Tensor output(DT_QINT32, shape);
  auto input_array = input.flat<float>();
  output.flat<qint32>() = QUANTIZE_WITH_EIGEN(input_array, f2q, qint32);
  test::ExpectTensorEqual<qint32>(expected, output);
}

TEST_F(QuantizationUtilsTest, QuantizedTensorToFloat) {
  const int input_width = 3;
  const int input_height = 3;
  const float input_min = -128.0f;
  const float input_max = 127.0f;
  Tensor input(DT_QUINT8, TensorShape({input_height, input_width}));
  test::FillValues<quint8>(&input, {0, 128, 255, 23, 24, 25, 243, 244, 245});
  Tensor expected(DT_FLOAT, TensorShape({input_height, input_width}));
  test::FillValues<float>(&expected, {-128.0f, 0.0f, 127.0f, -105.0f, -104.0f,
                                      -103.0f, 115.0f, 116.0f, 117.0f});
  Tensor output = QuantizedTensorToFloat<quint8>(input, input_min, input_max);
  test::ExpectTensorEqual<float>(expected, output);
}

// Verify that QuantizedToFloatInPlaceUsingEigen is same result as
// QuantizedToFloat.
TEST_F(QuantizationUtilsTest, QuantizedToFloatInPlaceUsingEigen) {
  thread::ThreadPool threadpool(Env::Default(), "test", 2 /* num_threads */);
  EigenThreadPoolWrapper wrapper(&threadpool);
  Eigen::ThreadPoolDevice eigen_device(&wrapper, 2 /* num_threads */);

  TestQuantizedToFloatInPlaceUsingEigen<quint8>(&eigen_device);
  TestQuantizedToFloatInPlaceUsingEigen<qint8>(&eigen_device);
  TestQuantizedToFloatInPlaceUsingEigen<quint16>(&eigen_device);
  TestQuantizedToFloatInPlaceUsingEigen<qint16>(&eigen_device);
  TestQuantizedToFloatInPlaceUsingEigen<qint32>(&eigen_device);
}

}  // namespace tensorflow
