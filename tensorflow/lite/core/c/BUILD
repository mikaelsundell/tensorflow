load(
    "//tensorflow/lite:build_def.bzl",
    "tflite_cc_library_with_c_headers_test",
    "tflite_copts",
)
load("//tensorflow:tensorflow.default.bzl", "get_compatible_with_portable")
load(
    "//tensorflow/lite/core/c:special_rules.bzl",
    "c_api_experimental_visibility_allowlist",
    "c_api_visibility_allowlist",
    "common_header_visibility_allowlist",
)
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(
    # copybara:uncomment default_applicable_licenses = ["//tensorflow:license"],
    default_visibility = ["//tensorflow/lite:__subpackages__"],
    licenses = ["notice"],
)

exports_files(
    srcs = [
        "c_api.h",
        "c_api_experimental.h",
        "c_api_opaque.h",
        "c_api_types.h",
        "common.h",
        "registration_external.h",
    ],
    visibility = [
        "//tensorflow/lite:__subpackages__",
    ],
)

bzl_library(
    name = "special_rules_bzl",
    srcs = ["special_rules.bzl"],
    visibility = ["//tensorflow/lite:__subpackages__"],
)

filegroup(
    name = "headers_filegroup",
    srcs = [
        "c_api.h",
        "c_api_types.h",
        "common.h",
        "registration_external.h",
    ],
)

filegroup(
    name = "tflite_internal_cc_3p_api_deps_src",
    srcs = [
        "builtin_op_data.h",
        "common.cc",
        "common.h",
    ],
    visibility = [
        "//tensorflow/lite:__pkg__",
    ],
)

tflite_cc_library_with_c_headers_test(
    name = "c_api",
    hdrs = [
        "c_api.h",
    ],
    compatible_with = get_compatible_with_portable(),
    copts = tflite_copts(),
    visibility = [
        "//tensorflow/lite:__subpackages__",
    ] + c_api_visibility_allowlist(),
    deps = [
        ":c_api_types",
        ":c_api_without_op_resolver",
        ":registration_external",
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite:create_op_resolver_with_builtin_ops",
        "//tensorflow/lite/c:common",
        "//tensorflow/lite/core/async/c:types",
    ],
)

tflite_cc_library_with_c_headers_test(
    name = "c_api_without_op_resolver",
    srcs = ["c_api.cc"],
    hdrs = ["c_api.h"],
    compatible_with = get_compatible_with_portable(),
    copts = tflite_copts(),
    tags = ["allow_undefined_symbols"],  # For tflite::CreateOpResolver().
    visibility = [
        "//tensorflow/lite:__subpackages__",
    ],
    deps = [
        ":c_api_types",
        ":registration_external",
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite:version",
        "//tensorflow/lite/c:c_api_internal",
        "//tensorflow/lite/c:common_internal",
        "//tensorflow/lite/core:create_op_resolver_header",
        "//tensorflow/lite/core:framework",
        "//tensorflow/lite/core/async/c:types",
        "//tensorflow/lite/delegates:interpreter_utils",
        "//tensorflow/lite/delegates/nnapi:nnapi_delegate",
        "//tensorflow/lite/kernels/internal:compatibility",
    ],
    alwayslink = 1,  # Why?? TODO(b/161243354): eliminate this.
)

tflite_cc_library_with_c_headers_test(
    name = "private_c_api_without_op_resolver",
    hdrs = ["c_api.h"],
    copts = tflite_copts(),
    tags = [
        "allow_undefined_symbols",  # For tflite::CreateOpResolver().
        "avoid_dep",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":c_api_types",
        ":registration_external",
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite/core/async/c:types",
    ],
)

# Same as ":c_api_without_op_resolver", but without alwayslink=1.
tflite_cc_library_with_c_headers_test(
    name = "c_api_without_op_resolver_without_alwayslink",
    srcs = ["c_api.cc"],
    hdrs = ["c_api.h"],
    copts = tflite_copts(),
    tags = ["allow_undefined_symbols"],  # For tflite::CreateOpResolver().
    deps = [
        ":c_api_types",
        ":registration_external_without_alwayslink",
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite:framework",
        "//tensorflow/lite:version",
        "//tensorflow/lite/c:c_api_internal",
        "//tensorflow/lite/c:common_internal",
        "//tensorflow/lite/core:create_op_resolver_header",
        "//tensorflow/lite/core:framework",
        "//tensorflow/lite/core/async/c:types",
        "//tensorflow/lite/delegates:interpreter_utils",
        "//tensorflow/lite/delegates/nnapi:nnapi_delegate",
        "//tensorflow/lite/kernels:kernel_util",
        "//tensorflow/lite/kernels/internal:compatibility",
    ],
)

# This is a private target, its visibility is set to public only to be
# used by "tflite_custom_c_library".
# Do not use this target directly and don't consider it as a part of the public API.
alias(
    name = "private_c_api_without_op_resolver_without_alwayslink",
    actual = ":c_api_without_op_resolver_without_alwayslink",
    tags = ["avoid_dep"],
    visibility = [
        "//visibility:public",
    ],
)

cc_test(
    name = "c_api_test",
    size = "small",
    srcs = ["c_api_test.cc"],
    copts = tflite_copts(),
    data = [
        "//tensorflow/lite:testdata/2_subgraphs.bin",
        "//tensorflow/lite:testdata/add.bin",
        "//tensorflow/lite:testdata/add_quantized.bin",
        "//tensorflow/lite:testdata/custom_sinh.bin",
    ],
    deps = [
        ":c_api",
        ":c_api_experimental",
        ":c_api_types",
        ":common",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite/c:c_api_internal",
        "//tensorflow/lite/core:subgraph",
        "//tensorflow/lite/delegates:delegate_test_util",
        "//tensorflow/lite/schema:schema_fbs",
        "//tensorflow/lite/testing:util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "selectively_built_c_api_test",
    size = "small",
    srcs = ["c_api_test.cc"],
    copts = tflite_copts(),
    data = [
        "//tensorflow/lite:testdata/2_subgraphs.bin",
        "//tensorflow/lite:testdata/add.bin",
        "//tensorflow/lite:testdata/add_quantized.bin",
        "//tensorflow/lite:testdata/custom_sinh.bin",
    ],
    deps = [
        ":c_api_experimental_without_op_resolver_without_alwayslink",
        ":c_api_types",
        ":c_api_without_op_resolver_without_alwayslink",
        ":common",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite/c:c_api_internal",
        "//tensorflow/lite/c:selectively_built_c_api_test_lib",
        "//tensorflow/lite/core:subgraph",
        "//tensorflow/lite/delegates:delegate_test_util",
        "//tensorflow/lite/schema:schema_fbs",
        "//tensorflow/lite/testing:util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "builtin_op_data_test",
    size = "small",
    srcs = ["builtin_op_data_test.cc"],
    copts = ["-Wno-unused-variable"],
    deps = [
        ":common",
        "@com_google_googletest//:gtest_main",
    ],
)

# This is a private target, its visibility is set to public only to be
# used by "tflite_custom_c_library".
# Do not use this target directly and don't consider it as a part of the public API.
alias(
    name = "private_c_api_types",
    actual = ":c_api_types",
    tags = ["avoid_dep"],
    visibility = [
        "//visibility:public",
    ],
)

tflite_cc_library_with_c_headers_test(
    name = "c_api_types",
    hdrs = ["c_api_types.h"],
    compatible_with = get_compatible_with_portable(),
    copts = tflite_copts(),
    visibility = [
        "//tensorflow/lite:__subpackages__",
    ] + c_api_visibility_allowlist(),
)

# Test the C extension API code.
cc_test(
    name = "common_test",
    size = "small",
    srcs = ["common_test.cc"],
    deps = [
        ":c_api_types",
        ":common",
        "//tensorflow/lite:util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "c_api_opaque_test",
    size = "small",
    srcs = [
        "c_api_opaque.cc",
        "c_api_opaque.h",
        "c_api_opaque_test.cc",
    ],
    deps = [
        ":c_api",
        ":c_api_types",
        ":common",
        ":registration_external",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite:util",
        "//tensorflow/lite/c:c_api_opaque_internal",
        "//tensorflow/lite/c:common",
        "//tensorflow/lite/core:subgraph",
        "//tensorflow/lite/kernels:kernel_util",
        "@com_google_googletest//:gtest_main",
    ],
)

tflite_cc_library_with_c_headers_test(
    name = "common",
    srcs = ["common.cc"],
    hdrs = [
        "builtin_op_data.h",
        "common.h",
    ],
    compatible_with = get_compatible_with_portable(),
    copts = tflite_copts(),
    visibility = [
        "//tensorflow/lite:__subpackages__",
        "@org_tensorflow_lite_support//tensorflow_lite_support/custom_ops:__subpackages__",
    ] + common_header_visibility_allowlist(),
    deps = [
        ":c_api_types",
        "//tensorflow/lite:tflite_kernel_use_xnnpack_optional",
    ] + select({
        "//tensorflow/lite:tensorflow_profiler_config": [
            "//tensorflow/lite:macros",
            "//tensorflow/lite:tensorflow_profiler_logger_shim",
        ],
        "//conditions:default": [],
    }),
    alwayslink = 1,  # Why?? TODO(b/161243354): eliminate this.
)

# This is a private target, its visibility is set to public only to be
# used by "tflite_custom_c_library" and "tflite_flex_cc_library".
# Do not use this target directly and don't consider it as a part of the public API.
alias(
    name = "private_common",
    actual = ":common",
    tags = ["avoid_dep"],
    visibility = [
        "//visibility:public",
    ],
)

tflite_cc_library_with_c_headers_test(
    name = "c_api_experimental",
    hdrs = [
        "c_api_experimental.h",
        # DEPRECATED
        # ':c_api_opaque' was promoted to the 'mostly stable' API.
        # Please explicitly add a dependency on ':c_api_opaque' if your target
        # needs both ':c_api_experimental' and ':c_api_opaque' dependencies.
        # We plan to remove 'c_api_opaque.h' from the list of exposed headers and
        # will remove the dependency from this target on ':c_api_opaque' in the future.
        "c_api_opaque.h",
    ],
    copts = tflite_copts(),
    visibility = [
        "//tensorflow/lite:__subpackages__",
    ] + c_api_experimental_visibility_allowlist(),
    deps = [
        ":c_api",
        ":c_api_experimental_without_op_resolver",
        ":c_api_opaque",
        ":c_api_types",
        ":common",
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite:signature_runner",
        "//tensorflow/lite/c:c_api_internal",
        "//tensorflow/lite/c:c_api_opaque_internal",
        "//tensorflow/lite/core:framework",
        "//tensorflow/lite/kernels:kernel_util",
        "//tensorflow/lite/profiling/telemetry:profiler",
        "//tensorflow/lite/profiling/telemetry/c:profiler",
    ],
)

# Same as ":c_api_experimental", but without linking in the default CreateOpResolver implementation.
tflite_cc_library_with_c_headers_test(
    name = "c_api_experimental_without_op_resolver",
    srcs = [
        "c_api_experimental.cc",
    ],
    hdrs = [
        "c_api_experimental.h",
        # DEPRECATED
        # ':c_api_opaque_without_op_resolver' was promoted to the 'mostly stable' API.
        # Please explicitly add a dependency on ':c_api_opaque_without_op_resolver' if your target
        # needs both ':c_api_experimental_without_op_resolver' and ':c_api_opaque_without_op_resolver' dependencies.
        # We plan to remove 'c_api_opaque.h' from the list of exposed headers and
        # will remove the dependency from this target on ':c_api_opaque_without_op_resolver' in the future.
        "c_api_opaque.h",
    ],
    copts = tflite_copts(),
    tags = ["allow_undefined_symbols"],  # For tflite::CreateOpResolver().
    deps = [
        ":c_api_opaque_without_op_resolver",
        ":c_api_types",
        ":c_api_without_op_resolver",
        ":common",
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite:framework",
        "//tensorflow/lite:kernel_api",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite:util",
        "//tensorflow/lite/c:c_api_internal",
        "//tensorflow/lite/c:c_api_opaque_internal",
        "//tensorflow/lite/core:framework",
        "//tensorflow/lite/kernels:kernel_util",
        "//tensorflow/lite/profiling/telemetry:profiler",
        "//tensorflow/lite/profiling/telemetry/c:profiler",
    ],
    alwayslink = 1,  # Why?? TODO(b/161243354): eliminate this.
)

# This is a private target, its visibility is set to public only to be
# used by "custom_c_library_with_tflite".
# Do not use this target directly and don't consider it as a part of the public API.
alias(
    name = "private_c_api_experimental_without_op_resolver",
    actual = ":c_api_experimental_without_op_resolver",
    tags = ["avoid_dep"],
    visibility = [
        "//visibility:public",
    ],
)

# Same as ":c_api_experimental", but without linking in the default CreateOpResolver implementation.
tflite_cc_library_with_c_headers_test(
    name = "c_api_experimental_without_op_resolver_without_alwayslink",
    srcs = [
        "c_api_experimental.cc",
    ],
    hdrs = [
        "c_api_experimental.h",
        # DEPRECATED
        # ':c_api_opaque_without_op_resolver_without_alwayslink' was promoted to the 'mostly stable' API.
        # Please explicitly add a dependency on ':c_api_opaque_without_op_resolver_without_alwayslink'
        # if your target needs both ':c_api_experimental_without_op_resolver_without_alwayslink' and
        # ':c_api_opaque_without_op_resolver_without_alwayslink' dependencies.
        # We plan to remove 'c_api_opaque.h' from the list of exposed headers and
        # will remove the dependency from this target on
        # ':c_api_opaque_without_op_resolver_without_alwayslink' in the future.
        "c_api_opaque.h",
    ],
    copts = tflite_copts(),
    tags = ["allow_undefined_symbols"],  # For tflite::CreateOpResolver().
    deps = [
        ":c_api_opaque_without_op_resolver_without_alwayslink",
        ":c_api_types",
        ":c_api_without_op_resolver_without_alwayslink",
        ":common",
        "//tensorflow/lite:framework",
        "//tensorflow/lite:kernel_api",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite:util",
        "//tensorflow/lite/c:c_api_internal",
        "//tensorflow/lite/c:c_api_opaque_internal_without_alwayslink",
        "//tensorflow/lite/core:framework",
        "//tensorflow/lite/kernels:kernel_util",
        "//tensorflow/lite/profiling/telemetry:profiler",
        "//tensorflow/lite/profiling/telemetry/c:profiler",
    ],
)

# This is a private target, its visibility is set to public only to be
# used by "tflite_custom_c_library".
# Do not use this target directly and don't consider it as a part of the public API.
alias(
    name = "private_c_api_experimental_without_op_resolver_without_alwayslink",
    actual = ":c_api_experimental_without_op_resolver_without_alwayslink",
    tags = ["avoid_dep"],
    visibility = [
        "//visibility:public",
    ],
)

tflite_cc_library_with_c_headers_test(
    name = "c_api_opaque",
    hdrs = [
        "c_api_opaque.h",
    ],
    copts = tflite_copts(),
    visibility = [
        "//tensorflow/lite:__subpackages__",
    ] + c_api_experimental_visibility_allowlist(),
    deps = [
        ":c_api",
        ":c_api_opaque_without_op_resolver",
        ":c_api_types",
        ":common",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite:util",
        "//tensorflow/lite/c:c_api_opaque_internal",
        "//tensorflow/lite/core:framework",
        "//tensorflow/lite/kernels:kernel_util",
    ],
)

tflite_cc_library_with_c_headers_test(
    name = "c_api_opaque_without_op_resolver",
    srcs = [
        "c_api_opaque.cc",
    ],
    hdrs = [
        "c_api_opaque.h",
    ],
    copts = tflite_copts(),
    tags = ["allow_undefined_symbols"],  # For tflite::CreateOpResolver().
    deps = [
        ":c_api_types",
        ":c_api_without_op_resolver",
        ":common",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite:util",
        "//tensorflow/lite/c:c_api_opaque_internal",
        "//tensorflow/lite/core:framework",
        "//tensorflow/lite/kernels:kernel_util",
    ],
)

# This is a private target, its visibility is set to public only to be
# used by "custom_c_library_with_tflite".
# Do not use this target directly and don't consider it as a part of the public API.
alias(
    name = "private_c_api_opaque_without_op_resolver",
    actual = ":c_api_opaque_without_op_resolver",
    tags = ["avoid_dep"],
    visibility = [
        "//visibility:public",
    ],
)

# Same as ":c_api_opaque_without_op_resolver", but without alwayslink=1.
tflite_cc_library_with_c_headers_test(
    name = "c_api_opaque_without_op_resolver_without_alwayslink",
    srcs = [
        "c_api_opaque.cc",
    ],
    hdrs = [
        "c_api_opaque.h",
    ],
    copts = tflite_copts(),
    tags = ["allow_undefined_symbols"],  # For tflite::CreateOpResolver().
    deps = [
        ":c_api_types",
        ":c_api_without_op_resolver_without_alwayslink",
        ":common",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite:util",
        "//tensorflow/lite/c:c_api_opaque_internal_without_alwayslink",
        "//tensorflow/lite/core:framework",
        "//tensorflow/lite/kernels:kernel_util",
    ],
)

# This is a private target, its visibility is set to public only to be
# used by "tflite_custom_c_library".
# Do not use this target directly and don't consider it as a part of the public API.
alias(
    name = "private_c_api_opaque_without_op_resolver_without_alwayslink",
    actual = ":c_api_opaque_without_op_resolver_without_alwayslink",
    tags = ["avoid_dep"],
    visibility = [
        "//visibility:public",
    ],
)

cc_test(
    name = "c_api_experimental_test",
    size = "small",
    srcs = ["c_api_experimental_test.cc"],
    copts = tflite_copts(),
    data = [
        "//tensorflow/lite:testdata/add.bin",
        "//tensorflow/lite:testdata/custom_sinh.bin",
    ],
    deps = [
        ":c_api",
        ":c_api_experimental",
        ":common",
        "//tensorflow/lite:kernel_api",
        "//tensorflow/lite:util",
        "//tensorflow/lite/delegates:delegate_test_util",
        "//tensorflow/lite/testing:util",
        "@com_google_googletest//:gtest_main",
    ],
)

tflite_cc_library_with_c_headers_test(
    name = "registration_external",
    srcs = ["registration_external.cc"],
    hdrs = ["registration_external.h"],
    compatible_with = get_compatible_with_portable(),
    copts = tflite_copts(),
    visibility = [
        "//tensorflow/lite:__subpackages__",
    ] + c_api_visibility_allowlist(),
    deps = [
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite/c:common_internal",
        "//tensorflow/lite/core/async/c:types",
        "//tensorflow/lite/core/c:c_api_types",
        "//tensorflow/lite/core/c:common",
    ],
    # When building shared libraries, we need alwayslink here, since the functions
    # in this header are part of the public API.
    alwayslink = 1,
)

tflite_cc_library_with_c_headers_test(
    name = "registration_external_without_alwayslink",
    srcs = ["registration_external.cc"],
    hdrs = ["registration_external.h"],
    compatible_with = get_compatible_with_portable(),
    copts = tflite_copts(),
    deps = [
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite/c:common_internal",
        "//tensorflow/lite/core/async/c:types",
        "//tensorflow/lite/core/c:c_api_types",
        "//tensorflow/lite/core/c:common",
    ],
)
