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

#include "tensorflow/core/util/sparse/sparse_tensor.h"

#include <string>
#include <vector>

#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_types.h"
#include "tensorflow/core/lib/core/status_test_util.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {
namespace sparse {
namespace {

Eigen::Tensor<int64, 2, Eigen::RowMajor, Eigen::DenseIndex>
GetSimpleIndexTensor(int N, const int NDIM) {
  Eigen::Tensor<int64, 2, Eigen::RowMajor, Eigen::DenseIndex> ix(N, NDIM);
  ix(0, 0) = 0;
  ix(0, 1) = 0;
  ix(0, 2) = 0;

  ix(1, 0) = 3;
  ix(1, 1) = 0;
  ix(1, 2) = 0;

  ix(2, 0) = 2;
  ix(2, 1) = 0;
  ix(2, 2) = 0;

  ix(3, 0) = 0;
  ix(3, 1) = 1;
  ix(3, 2) = 0;

  ix(4, 0) = 0;
  ix(4, 1) = 0;
  ix(4, 2) = 2;
  return ix;
}

TEST(SparseTensorTest, DimComparatorSorts) {
  std::size_t N = 5;
  const int NDIM = 3;
  auto ix = GetSimpleIndexTensor(N, NDIM);
  TTypes<int64>::Matrix map(ix.data(), N, NDIM);

  std::vector<int64> sorting(N);
  for (std::size_t n = 0; n < N; ++n) sorting[n] = n;

  // new order should be: {0, 4, 3, 2, 1}
  std::vector<int64> order{0, 1, 2};
  DimComparator sorter(map, order, NDIM);
  std::sort(sorting.begin(), sorting.end(), sorter);

  EXPECT_EQ(sorting, std::vector<int64>({0, 4, 3, 2, 1}));

  // new order should be: {0, 3, 2, 1, 4}
  std::vector<int64> order1{2, 0, 1};
  DimComparator sorter1(map, order1, NDIM);
  for (std::size_t n = 0; n < N; ++n) sorting[n] = n;
  std::sort(sorting.begin(), sorting.end(), sorter1);

  EXPECT_EQ(sorting, std::vector<int64>({0, 3, 2, 1, 4}));
}

TEST(SparseTensorTest, SparseTensorInvalidIndicesType) {
  int N = 5;
  const int NDIM = 3;
  Tensor ix(DT_INT32, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  EXPECT_DEATH(SparseTensor(ix, vals, TensorShape({10, 10, 10}), {0, 1, 2}),
               "indices must be type int64");
}

TEST(SparseTensorTest, SparseTensorInvalidIndicesShape) {
  int N = 5;
  const int NDIM = 3;
  Tensor ix(DT_INT64, TensorShape({N, NDIM, 1}));
  Tensor vals(DT_STRING, TensorShape({N}));

  EXPECT_DEATH(SparseTensor(ix, vals, TensorShape({10, 10, 10}), {0, 1, 2}),
               "indices must be a matrix");
}

TEST(SparseTensorTest, SparseTensorInvalidValues) {
  int N = 5;
  const int NDIM = 3;
  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N, 1}));

  EXPECT_DEATH(SparseTensor(ix, vals, TensorShape({10, 10, 10}), {0, 1, 2}),
               "vals must be a vec");
}

TEST(SparseTensorTest, SparseTensorInvalidN) {
  int N = 5;
  const int NDIM = 3;
  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N - 1}));

  EXPECT_DEATH(SparseTensor(ix, vals, TensorShape({10, 10, 10}), {0, 1, 2}),
               "indices and values rows .* must match");
}

TEST(SparseTensorTest, SparseTensorInvalidOrder) {
  int N = 5;
  const int NDIM = 3;
  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  EXPECT_DEATH(SparseTensor(ix, vals, TensorShape({10, 10, 10}), {0, 1}),
               "Order length must be SparseTensor rank");
}
TEST(SparseTensorTest, SparseTensorInvalidShape) {
  int N = 5;
  const int NDIM = 3;
  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  EXPECT_DEATH(SparseTensor(ix, vals, TensorShape({10, 10}), {0, 1, 2}),
               "Shape rank must be SparseTensor rank");
}

TEST(SparseTensorTest, SparseTensorConstruction) {
  int N = 5;
  const int NDIM = 3;
  auto ix_c = GetSimpleIndexTensor(N, NDIM);
  Eigen::Tensor<string, 1, Eigen::RowMajor> vals_c(N);
  vals_c(0) = "hi0";
  vals_c(1) = "hi1";
  vals_c(2) = "hi2";
  vals_c(3) = "hi3";
  vals_c(4) = "hi4";

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  auto ix_t = ix.matrix<int64>();
  auto vals_t = vals.vec<string>();
  vals_t = vals_c;
  ix_t = ix_c;

  TensorShape shape({10, 10, 10});
  std::vector<int64> order{0, 1, 2};
  SparseTensor st(ix, vals, shape, order);
  Status st_indices_valid = st.IndicesValid();
  EXPECT_FALSE(st_indices_valid.ok());
  EXPECT_EQ("indices[2] = [2,0,0] is out of order",
            st_indices_valid.error_message());

  // Regardless of how order is updated; so long as there are no
  // duplicates, the resulting indices are valid.
  st.Reorder<string>({2, 0, 1});
  TF_EXPECT_OK(st.IndicesValid());
  EXPECT_EQ(vals_t(0), "hi0");
  EXPECT_EQ(vals_t(1), "hi3");
  EXPECT_EQ(vals_t(2), "hi2");
  EXPECT_EQ(vals_t(3), "hi1");
  EXPECT_EQ(vals_t(4), "hi4");

  ix_t = ix_c;
  vals_t = vals_c;
  st.Reorder<string>({0, 1, 2});
  TF_EXPECT_OK(st.IndicesValid());
  EXPECT_EQ(vals_t(0), "hi0");
  EXPECT_EQ(vals_t(1), "hi4");
  EXPECT_EQ(vals_t(2), "hi3");
  EXPECT_EQ(vals_t(3), "hi2");
  EXPECT_EQ(vals_t(4), "hi1");

  ix_t = ix_c;
  vals_t = vals_c;
  st.Reorder<string>({2, 1, 0});
  TF_EXPECT_OK(st.IndicesValid());
}

TEST(SparseTensorTest, EmptySparseTensorAllowed) {
  int N = 0;
  const int NDIM = 3;

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  TensorShape shape({10, 10, 10});
  std::vector<int64> order{0, 1, 2};
  SparseTensor st(ix, vals, shape, order);
  TF_EXPECT_OK(st.IndicesValid());
  EXPECT_EQ(st.order(), order);

  std::vector<int64> new_order{1, 0, 2};
  st.Reorder<string>(new_order);
  TF_EXPECT_OK(st.IndicesValid());
  EXPECT_EQ(st.order(), new_order);
}

TEST(SparseTensorTest, SortingWorksCorrectly) {
  int N = 30;
  const int NDIM = 4;

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));
  TensorShape shape({1000, 1000, 1000, 1000});
  SparseTensor st(ix, vals, shape);

  auto ix_t = ix.matrix<int64>();

  for (int n = 0; n < 100; ++n) {
    ix_t = ix_t.random(Eigen::internal::UniformRandomGenerator<int64>(n + 1));
    ix_t = ix_t.abs() % 1000;
    st.Reorder<string>({0, 1, 2, 3});
    TF_EXPECT_OK(st.IndicesValid());
    st.Reorder<string>({3, 2, 1, 0});
    TF_EXPECT_OK(st.IndicesValid());
    st.Reorder<string>({1, 0, 2, 3});
    TF_EXPECT_OK(st.IndicesValid());
    st.Reorder<string>({3, 0, 2, 1});
    TF_EXPECT_OK(st.IndicesValid());
  }
}

TEST(SparseTensorTest, ValidateIndicesFindsInvalid) {
  int N = 2;
  const int NDIM = 3;

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  Eigen::Tensor<int64, 2, Eigen::RowMajor> ix_orig(N, NDIM);
  ix_orig(0, 0) = 0;
  ix_orig(0, 1) = 0;
  ix_orig(0, 2) = 0;

  ix_orig(1, 0) = 0;
  ix_orig(1, 1) = 0;
  ix_orig(1, 2) = 0;

  auto ix_t = ix.matrix<int64>();
  ix_t = ix_orig;

  TensorShape shape({10, 10, 10});
  std::vector<int64> order{0, 1, 2};
  SparseTensor st(ix, vals, shape, order);

  st.Reorder<string>(order);
  Status st_indices_valid = st.IndicesValid();
  EXPECT_FALSE(st_indices_valid.ok());
  EXPECT_EQ("indices[1] = [0,0,0] is repeated",
            st_indices_valid.error_message());

  ix_orig(1, 2) = 1;
  ix_t = ix_orig;
  st.Reorder<string>(order);
  TF_EXPECT_OK(st.IndicesValid());  // second index now (0, 0, 1)

  ix_orig(0, 2) = 1;
  ix_t = ix_orig;
  st.Reorder<string>(order);
  st_indices_valid = st.IndicesValid();
  EXPECT_FALSE(st_indices_valid.ok());  // first index now (0, 0, 1)
  EXPECT_EQ("indices[1] = [0,0,1] is repeated",
            st_indices_valid.error_message());
}

TEST(SparseTensorTest, SparseTensorCheckBoundaries) {
  int N = 5;
  const int NDIM = 3;

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  auto ix_t = GetSimpleIndexTensor(N, NDIM);

  ix.matrix<int64>() = ix_t;

  TensorShape shape({10, 10, 10});
  std::vector<int64> order{0, 1, 2};

  SparseTensor st(ix, vals, shape, order);
  EXPECT_FALSE(st.IndicesValid().ok());

  st.Reorder<string>(order);
  TF_EXPECT_OK(st.IndicesValid());

  ix_t(0, 0) = 11;
  ix.matrix<int64>() = ix_t;
  st.Reorder<string>(order);
  Status st_indices_valid = st.IndicesValid();
  EXPECT_FALSE(st_indices_valid.ok());
  // Error message references index 4 because of the call to Reorder.
  EXPECT_EQ(
      "indices[4] = [11,0,0] is out of bounds: need 0 <= index < [10,10,10]",
      st_indices_valid.error_message());

  ix_t(0, 0) = -1;
  ix.matrix<int64>() = ix_t;
  st.Reorder<string>(order);
  st_indices_valid = st.IndicesValid();
  EXPECT_FALSE(st_indices_valid.ok());
  EXPECT_EQ(
      "indices[0] = [-1,0,0] is out of bounds: need 0 <= index < [10,10,10]",
      st_indices_valid.error_message());

  ix_t(0, 0) = 0;
  ix.matrix<int64>() = ix_t;
  st.Reorder<string>(order);
  TF_EXPECT_OK(st.IndicesValid());
}

TEST(SparseTensorTest, SparseTensorToDenseTensor) {
  int N = 5;
  const int NDIM = 3;

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  auto ix_t = GetSimpleIndexTensor(N, NDIM);
  auto vals_t = vals.vec<string>();

  ix.matrix<int64>() = ix_t;

  vals_t(0) = "hi0";
  vals_t(1) = "hi1";
  vals_t(2) = "hi2";
  vals_t(3) = "hi3";
  vals_t(4) = "hi4";

  TensorShape shape({4, 4, 5});
  std::vector<int64> order{0, 1, 2};
  SparseTensor st(ix, vals, shape, order);

  Tensor dense(DT_STRING, TensorShape({4, 4, 5}));
  st.ToDense<string>(&dense);

  auto dense_t = dense.tensor<string, 3>();
  Eigen::array<Eigen::DenseIndex, NDIM> ix_n;
  for (int n = 0; n < N; ++n) {
    for (int d = 0; d < NDIM; ++d) ix_n[d] = ix_t(n, d);
    EXPECT_EQ(dense_t(ix_n), vals_t(n));
  }

  // Spot checks on the others
  EXPECT_EQ(dense_t(0, 0, 1), "");
  EXPECT_EQ(dense_t(0, 0, 3), "");
  EXPECT_EQ(dense_t(3, 3, 3), "");
  EXPECT_EQ(dense_t(3, 3, 4), "");
}

TEST(SparseTensorTest, SparseTensorToLargerDenseTensor) {
  int N = 5;
  const int NDIM = 3;

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  auto ix_t = GetSimpleIndexTensor(N, NDIM);
  auto vals_t = vals.vec<string>();

  ix.matrix<int64>() = ix_t;

  vals_t(0) = "hi0";
  vals_t(1) = "hi1";
  vals_t(2) = "hi2";
  vals_t(3) = "hi3";
  vals_t(4) = "hi4";

  TensorShape shape({4, 4, 5});
  std::vector<int64> order{0, 1, 2};
  SparseTensor st(ix, vals, shape, order);

  Tensor dense(DT_STRING, TensorShape({10, 10, 10}));
  st.ToDense<string>(&dense);

  auto dense_t = dense.tensor<string, 3>();
  Eigen::array<Eigen::DenseIndex, NDIM> ix_n;
  for (int n = 0; n < N; ++n) {
    for (int d = 0; d < NDIM; ++d) ix_n[d] = ix_t(n, d);
    EXPECT_EQ(dense_t(ix_n), vals_t(n));
  }

  // Spot checks on the others
  EXPECT_EQ(dense_t(0, 0, 1), "");
  EXPECT_EQ(dense_t(0, 0, 3), "");
  EXPECT_EQ(dense_t(3, 3, 3), "");
  EXPECT_EQ(dense_t(3, 3, 4), "");
  EXPECT_EQ(dense_t(9, 0, 0), "");
  EXPECT_EQ(dense_t(9, 0, 9), "");
  EXPECT_EQ(dense_t(9, 9, 9), "");
}

TEST(SparseTensorTest, SparseTensorGroup) {
  int N = 5;
  const int NDIM = 3;

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_INT32, TensorShape({N}));

  auto ix_t = ix.matrix<int64>();
  auto vals_t = vals.vec<int32>();

  ix_t = GetSimpleIndexTensor(N, NDIM);

  vals_t(0) = 1;  // associated with ix (000)
  vals_t(1) = 2;  // associated with ix (300)
  vals_t(2) = 3;  // associated with ix (200)
  vals_t(3) = 4;  // associated with ix (010)
  vals_t(4) = 5;  // associated with ix (002)

  TensorShape shape({10, 10, 10});
  std::vector<int64> order{0, 1, 2};

  SparseTensor st(ix, vals, shape, order);
  st.Reorder<int32>(order);

  std::vector<std::vector<int64> > groups;
  std::vector<TTypes<int64>::UnalignedConstMatrix> grouped_indices;
  std::vector<TTypes<int32>::UnalignedVec> grouped_values;

  // Group by index 0
  auto gi = st.group({0});

  // All the hard work is right here!
  for (const auto& g : gi) {
    groups.push_back(g.group());
    VLOG(1) << "Group: " << str_util::Join(g.group(), ",");
    VLOG(1) << "Indices: " << g.indices();
    VLOG(1) << "Values: " << g.values<int32>();

    grouped_indices.push_back(g.indices());
    grouped_values.push_back(g.values<int32>());
  }

  // Group by dimension 0, we have groups: 0--, 2--, 3--
  EXPECT_EQ(groups.size(), 3);
  EXPECT_EQ(groups[0], std::vector<int64>({0}));
  EXPECT_EQ(groups[1], std::vector<int64>({2}));
  EXPECT_EQ(groups[2], std::vector<int64>({3}));

  std::vector<Eigen::Tensor<int64, 2, Eigen::RowMajor> > expected_indices;
  std::vector<Eigen::Tensor<int32, 1, Eigen::RowMajor> > expected_vals;

  // First group: 000, 002, 010
  expected_indices.emplace_back(3, NDIM);  // 3 x 3 tensor
  expected_vals.emplace_back(3);           // 3 x 5 x 1 x 1 tensor
  expected_indices[0].setZero();
  expected_indices[0](1, 2) = 2;  // 002
  expected_indices[0](2, 1) = 1;  // 010
  expected_vals[0].setConstant(-1);
  expected_vals[0](0) = 1;  // val associated with ix 000
  expected_vals[0](1) = 5;  // val associated with ix 002
  expected_vals[0](2) = 4;  // val associated with ix 010

  // Second group: 200
  expected_indices.emplace_back(1, NDIM);
  expected_vals.emplace_back(1);
  expected_indices[1].setZero();
  expected_indices[1](0, 0) = 2;  // 200
  expected_vals[1](0) = 3;        // val associated with ix 200

  // Third group: 300
  expected_indices.emplace_back(1, NDIM);
  expected_vals.emplace_back(1);
  expected_indices[2].setZero();
  expected_indices[2](0, 0) = 3;  // 300
  expected_vals[2](0) = 2;        // val associated with ix 300

  for (std::size_t gix = 0; gix < groups.size(); ++gix) {
    // Compare indices
    auto gi_t = grouped_indices[gix];
    Eigen::Tensor<bool, 0, Eigen::RowMajor> eval =
        (gi_t == expected_indices[gix]).all();
    EXPECT_TRUE(eval()) << gix << " indices: " << gi_t << " vs. "
                        << expected_indices[gix];

    // Compare values
    auto gv_t = grouped_values[gix];
    eval = (gv_t == expected_vals[gix]).all();
    EXPECT_TRUE(eval()) << gix << " values: " << gv_t << " vs. "
                        << expected_vals[gix];
  }
}

TEST(SparseTensorTest, Concat) {
  int N = 5;
  const int NDIM = 3;

  Tensor ix(DT_INT64, TensorShape({N, NDIM}));
  Tensor vals(DT_STRING, TensorShape({N}));

  auto ix_c = GetSimpleIndexTensor(N, NDIM);

  auto ix_t = ix.matrix<int64>();
  auto vals_t = vals.vec<string>();

  ix_t = ix_c;

  TensorShape shape({10, 10, 10});
  std::vector<int64> order{0, 1, 2};

  SparseTensor st(ix, vals, shape, order);
  EXPECT_FALSE(st.IndicesValid().ok());
  st.Reorder<string>(order);
  TF_EXPECT_OK(st.IndicesValid());

  SparseTensor concatted = SparseTensor::Concat<string>({st, st, st, st});
  EXPECT_EQ(concatted.order(), st.order());
  TensorShape expected_shape({40, 10, 10});
  EXPECT_EQ(concatted.shape(), expected_shape);
  EXPECT_EQ(concatted.num_entries(), 4 * N);
  TF_EXPECT_OK(concatted.IndicesValid());

  auto conc_ix_t = concatted.indices().matrix<int64>();
  auto conc_vals_t = concatted.values().vec<string>();

  for (int n = 0; n < 4; ++n) {
    for (int i = 0; i < N; ++i) {
      // Dimensions match except the primary dim, which is offset by
      // shape[order[0]]
      EXPECT_EQ(conc_ix_t(n * N + i, 0), 10 * n + ix_t(i, 0));
      EXPECT_EQ(conc_ix_t(n * N + i, 1), ix_t(i, 1));
      EXPECT_EQ(conc_ix_t(n * N + i, 1), ix_t(i, 1));

      // Values match
      EXPECT_EQ(conc_vals_t(n * N + i), vals_t(i));
    }
  }

  // Concat works if non-primary ix is out of order, but output order
  // is not defined
  SparseTensor st_ooo(ix, vals, shape, {0, 2, 1});  // non-primary ix OOO
  SparseTensor conc_ooo = SparseTensor::Concat<string>({st, st, st, st_ooo});
  std::vector<int64> expected_ooo{-1, -1, -1};
  EXPECT_EQ(conc_ooo.order(), expected_ooo);
  EXPECT_EQ(conc_ooo.shape(), expected_shape);
  EXPECT_EQ(conc_ooo.num_entries(), 4 * N);
}

// TODO(ebrevdo): ReduceToDense(R={dim1,dim2,...}, reduce_fn, &output)
// reduce_fn sees slices of resorted values based on generator (dim: DDIMS), and
// slices of resorted indices on generator.

TEST(SparseTensorTest, Split) {
  const int N = 4;
  const int DIM = 2;

  Tensor ids(DT_INT64, TensorShape({N, DIM}));
  Tensor vals(DT_INT64, TensorShape({N}));

  ids.matrix<int64>()(0, 0) = 0;
  ids.matrix<int64>()(0, 1) = 0;
  ids.matrix<int64>()(1, 0) = 1;
  ids.matrix<int64>()(1, 1) = 1;
  ids.matrix<int64>()(2, 0) = 1;
  ids.matrix<int64>()(2, 1) = 2;
  ids.matrix<int64>()(3, 0) = 3;
  ids.matrix<int64>()(3, 1) = 0;

  vals.vec<int64>()(0) = 1;
  vals.vec<int64>()(1) = 2;
  vals.vec<int64>()(2) = 3;
  vals.vec<int64>()(3) = 4;

  SparseTensor st(ids, vals, TensorShape({4, 3}));

  std::vector<SparseTensor> st_list = SparseTensor::Split<int64>(st, 0, 2);

  EXPECT_EQ(st_list.size(), 2);
  EXPECT_EQ(st_list[0].shape(), TensorShape({2, 3}));
  EXPECT_EQ(st_list[0].values().NumElements(), 3);
  EXPECT_EQ(st_list[0].values().vec<int64>()(0), 1);
  EXPECT_EQ(st_list[0].values().vec<int64>()(1), 2);
  EXPECT_EQ(st_list[0].values().vec<int64>()(2), 3);
  EXPECT_EQ(st_list[0].indices().NumElements(), 6);
  EXPECT_EQ(st_list[0].indices().matrix<int64>()(0, 0), 0);
  EXPECT_EQ(st_list[0].indices().matrix<int64>()(0, 1), 0);
  EXPECT_EQ(st_list[0].indices().matrix<int64>()(1, 0), 1);
  EXPECT_EQ(st_list[0].indices().matrix<int64>()(1, 1), 1);
  EXPECT_EQ(st_list[0].indices().matrix<int64>()(2, 0), 1);
  EXPECT_EQ(st_list[0].indices().matrix<int64>()(2, 1), 2);

  EXPECT_EQ(st_list[1].shape(), TensorShape({2, 3}));
  EXPECT_EQ(st_list[1].values().NumElements(), 1);
  EXPECT_EQ(st_list[1].values().vec<int64>()(0), 4);
  EXPECT_EQ(st_list[1].indices().NumElements(), 2);
  EXPECT_EQ(st_list[1].indices().matrix<int64>()(0, 0), 1);
  EXPECT_EQ(st_list[1].indices().matrix<int64>()(0, 1), 0);
}

}  // namespace
}  // namespace sparse
}  // namespace tensorflow
