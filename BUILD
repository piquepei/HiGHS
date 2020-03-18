load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "highs",
    # todo: separate ipx (.c and .cc belong there).
    srcs = glob([
        "src/*.cpp",
        "src/io/*.cpp",
        "src/lp_data/*.cpp",
        "src/mip/*.cpp",
        "src/presolve/*.cpp",
        "src/simplex/*.cpp",
        "src/test/*.cpp",
        "src/util/*.cpp",
        "src/**/*.cc",
        "src/**/*.c",
    ]),
    hdrs = glob([
        "**/*.h",
    ]),
    includes = [
        "build",
        "src",
        "src/io",
        "src/ipm",
        "src/ipm/basiclu/include",
        "src/ipm/ipx/include",
        "src/lp_data",
        "src/mip",
        "src/presolve",
        "src/simplex",
        "src/test",
        "src/util",
    ],
    linkopts = ["-fopenmp"],
    copts = ["-g3"],
    visibility = ["//visibility:public"],
)