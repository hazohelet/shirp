filegroup(
    name = "tools",
    srcs = glob(
        [
            "*.c",
            "*.h",
        ],
    ),
    visibility = ["//test:__pkg__"],
)

cc_binary(
    name = "shirp",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    copts = [
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Werror",
    ],
)

cc_binary(
    name = "shirp_debug",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    copts = [
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Werror",
    ],
    defines = ["DEBUG"],
)
