# Description:
#   Benchmark utility that can run on desktop and Android.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

load(
    "//tensorflow:tensorflow.bzl",
    "tf_copts",
    "tf_cc_test",
)

exports_files(["LICENSE"])

cc_library(
    name = "benchmark_model_lib",
    testonly = 1,
    srcs = [
        "benchmark_model.cc",
    ],
    hdrs = [
        "benchmark_model.h",
    ],
    copts = tf_copts(),
    visibility = ["//visibility:public"],
    deps = select({
        "//tensorflow:android": [
            "//tensorflow/core:android_tensorflow_lib",
            "//tensorflow/core:android_tensorflow_test_lib",
        ],
        "//conditions:default": [
            "//tensorflow/core:core_cpu",
            "//tensorflow/core:lib",
            "//tensorflow/core:framework",
            "//tensorflow/core:framework_internal",
            "//tensorflow/core:protos_all_cc",
            "//tensorflow/core:tensorflow",
            "//tensorflow/core:test",
        ],
    }),
)

tf_cc_test(
    name = "benchmark_model_test",
    size = "medium",
    deps = [
        ":benchmark_model_lib",
        "//tensorflow/cc:cc_ops",
        "//tensorflow/core:core_cpu",
        "//tensorflow/core:framework",
        "//tensorflow/core:lib",
        "//tensorflow/core:test",
        "//tensorflow/core:test_main",
        "//tensorflow/core:testlib",
    ],
)

# This binary may be built for either desktop or Android.
# A typical Android build command will look like the following:
# bazel build -c opt tensorflow/core:android_tensorflow_lib \
# --crosstool_top=//external:android/crosstool \
# --cpu=armeabi-v7a \
# --host_crosstool_top=@bazel_tools//tools/cpp:toolchain
#
# NOTE: currently '-pthread' must be removed from the LINK_OPTS variable
# in @protobuf//:BUILD to sucessfully build for Android. This is temporary
# pending an update of the version of the protobuf library that TensorFlow
# uses.
cc_binary(
    name = "benchmark_model",
    testonly = 1,
    srcs = ["benchmark_model_main.cc"],
    copts = tf_copts(),
    linkopts = select({
        "//tensorflow:android": [
            "-pie",
            "-s",
            "-landroid",
            "-ljnigraphics",
            "-llog",
            "-lm",
            "-z defs",
            "-s",
            "-Wl,--icf=all",  # Identical Code Folding
            "-Wl,--exclude-libs,ALL",  # Exclude syms in all libs from auto export
        ],
        "//conditions:default": [],
    }),
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [":benchmark_model_lib"],
)
